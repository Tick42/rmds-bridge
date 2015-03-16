/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2014 Tick42 Ltd.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*Distributed under the Boost Software License, Version 1.0.
*    (See accompanying file LICENSE_1_0.txt or copy at
*         http://www.boost.org/LICENSE_1_0.txt)
*
*/
#include "stdafx.h"

#include <utils/t42log.h>
#include <utils/time.h>

#include "RMDSPublisher.h"
#include "UPAProvider.h"

#include "UPALogin.h"
#include "UPASourceDirectory.h"

#include "UPAPublisherItem.h"
#include "RMDSPublisherSource.h"
#include "UPAMessage.h"

// this is a timout for the select loop 
// In the reuters provider sample they use this to limit the update rate - wait for this period
// to see if anything to read then if something read or timeout expires publish some stuff
// Thats too crude for what we want to do here. we will be queing updates from the mama thread and
// applying a throttle to that. So, for the moment just hard code a 1ms timeout but consider making it 
// configurable
const int selTimeoutSec = 0;
const int selTimeoutUSec = 1000;

// The UPA sdk provider sample checks pings on every loop of the socket thread but that is run on a 1s timeout. We are running ours a lot faster
// because we are not using the loop to directly throttle the output rate. So use this interval to control how often we check for pings
// Could make it configurable but 100 ms is about right
const int pingCheckInterval  = 1000;

// todo make this configurable
const int maxClientConnections = 10;

UPAProvider::UPAProvider(RMDSPublisher * owner)
    : owner_(owner), login_(true)
{

    rsslServer_ = 0;
    runThread_ = true;
    portno_ = ::strdup(owner->PortNumber());

    clientConnections_ = new UPAClientConnection_t[maxClientConnections];
    for(int index = 0; index < maxClientConnections; index ++)
    {
        ResetClientConnection(&clientConnections_[index]);
    }

    requestQueue_ = owner->RequestQueue();

    acceptsPosts_ = owner->AcceptPosts();

}


UPAProvider::~UPAProvider()
{
}


void UPAProvider::Shutdown()
{
    // todo per - channel shutdown
    rsslUninitialize();
    runThread_ = false;

    return;
}

// Bind to listen socket
bool UPAProvider::Bind(char* portno, RsslError* error)
{
    RsslBindOptions sopts = RSSL_INIT_BIND_OPTS;

    sopts.guaranteedOutputBuffers = 500;
    sopts.serviceName = portno_;
    sopts.majorVersion = RSSL_RWF_MAJOR_VERSION;
    sopts.minorVersion = RSSL_RWF_MINOR_VERSION;
    sopts.protocolType = RSSL_RWF_PROTOCOL_TYPE;

    if ((rsslServer_ = rsslBind(&sopts, error)) != 0)
    {
        t42log_info("Publisher Socket id = %d bound on port %d\n", rsslServer_->socketId, rsslServer_->portNumber);
        FD_SET(rsslServer_->socketId,&readfds_);
        FD_SET(rsslServer_->socketId,&exceptfds_);
    }

    return rsslServer_ != 0;
}

void UPAProvider::Run()
{

    struct timeval time_interval;
    RsslRet	retval = 0;

    fd_set useRead;
    fd_set useExcept;
    fd_set useWrt;


    // prime the counter for ping checking
    lastPingCheck_ = utils::time::GetMilliCount();

    FD_ZERO(&readfds_);
    FD_ZERO(&exceptfds_);
    FD_ZERO(&wrtfds_);

    RsslError error;
    if (!Bind(portno_, &error))
    {
        t42log_error("Unable to start provider thread, bind return error %d ('%s') \n",error.sysError, error.text);
        return;
    }

    while(runThread_ == true)
    {

        useRead = readfds_;
        useExcept = exceptfds_;
        useWrt = wrtfds_;

        time_interval.tv_sec = selTimeoutSec;
        time_interval.tv_usec = selTimeoutUSec;

        // now process any queued publish events

        size_t numEvents = 0;

        size_t maxDispatchesPerCycle = 50;
        size_t maxPendingOpens = 500;
        mama_status status = mamaQueue_getEventCount(requestQueue_, &numEvents);
        if ((status == MAMA_STATUS_OK) && (numEvents > 0))
        {
            // although we only really want to throttle subscriptions, for simplicity throttle everything here.  
            size_t eventsDispatched = 0;

            // so keep dispatching while there are events on the queue, we haven t hit the mac per cycle and we havent hit the max pending limit
            while (numEvents > eventsDispatched && eventsDispatched < maxDispatchesPerCycle )
            {
                mamaQueue_dispatchEvent(requestQueue_);	
                ++eventsDispatched;
            }

            t42log_info("dispatched %d requests \n", eventsDispatched);
        }

        int selRet = select(FD_SETSIZE,&useRead, &useWrt,&useExcept,&time_interval);

        if (selRet == 0)
        {
            // timed out, no messages received so publish from queue
        }
        else if ( selRet > 0)
        {
            // read from the socket
            if ((rsslServer_ != NULL) && (rsslServer_->socketId != -1) && (FD_ISSET(rsslServer_->socketId,&useRead)))
            {
                // this is on the listening socket (rsslServer_->socketId)
                CreateNewClientConnection();
            }

            // so now read on all the channels
            for( int index = 0; index < maxClientConnections; index ++)
            {
                if ((clientConnections_[index].clientChannel != NULL) && (clientConnections_[index].clientChannel->socketId != -1))
                {
                    // theres a connection on this index so check the FDs
                    if ((FD_ISSET(clientConnections_[index].clientChannel->socketId, &useRead)) || (FD_ISSET(clientConnections_[index].clientChannel->socketId, &useExcept)))
                    {
                        // Theres something to read
                        ReadFromChannel(clientConnections_[index].clientChannel);
                    }

                    // The UPA provider sample ties these 2 conditions together but the problem then is that the
                    // ping initialization does happen until there is something to write (the write fd is set) and 
                    // there is no guarantee when that will happen. So here we check for active state on the channel 
                    // and whether the ping settings have been initialized before we check the write fd. Seems safer.
                    if (clientConnections_[index].clientChannel != NULL && clientConnections_[index].clientChannel->state == RSSL_CH_STATE_ACTIVE)
                    {
                        // if we havent set the ping times already (and the channel is now active) set them from the channel settings
                        if (clientConnections_[index].pingsInitialized != RSSL_TRUE)
                        {
                            InitChannelPingTimes(&clientConnections_[index]);
                            t42log_debug("Using %d as pingTimeout for Channel %d\n", clientConnections_[index].clientChannel->pingTimeout,clientConnections_[index].clientChannel->socketId);
                        }						
                        // if there is something to write and the channel is active thne flush the write socket
                        if (clientConnections_[index].clientChannel != NULL &&	FD_ISSET(clientConnections_[index].clientChannel->socketId, &useWrt))
                        {
                            if ((retval = rsslFlush(clientConnections_[index].clientChannel, &error)) < RSSL_RET_SUCCESS)
                            {
                                // the write failed. Kill the connection
                                t42log_error("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
                                RemoveChannelConnection(clientConnections_[index].clientChannel);
                            }
                            else if (retval == RSSL_RET_SUCCESS)
                            {
                                // clear the write fd
                                FD_CLR(clientConnections_[index].clientChannel->socketId, &wrtfds_);
                            }
                        }
                    }
                    // if there is something to write and the channel is active thne flush the write socket
                    if (clientConnections_[index].clientChannel != NULL &&	FD_ISSET(clientConnections_[index].clientChannel->socketId, &useWrt) &&
                        clientConnections_[index].clientChannel->state == RSSL_CH_STATE_ACTIVE)
                    {
                        // if we havent set the ping times already (and the channel is now active) set them from the channel settings
                        if (clientConnections_[index].pingsInitialized != RSSL_TRUE)
                        {
                            InitChannelPingTimes(&clientConnections_[index]);
                            t42log_debug("Using %d as pingTimeout for Channel %d\n", clientConnections_[index].clientChannel->pingTimeout,clientConnections_[index].clientChannel->socketId);
                        }


                        if ((retval = rsslFlush(clientConnections_[index].clientChannel, &error)) < RSSL_RET_SUCCESS)
                        {
                            // the write failed. Kill the connection
                            t42log_error("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
                            RemoveChannelConnection(clientConnections_[index].clientChannel);
                        }
                        else if (retval == RSSL_RET_SUCCESS)
                        {
                            // clear the write fd
                            FD_CLR(clientConnections_[index].clientChannel->socketId, &wrtfds_);
                        }
                    }
                }
            }
        }
        else
        {
            // handle error

#ifdef _WIN32
            if (WSAGetLastError() == WSAEINTR)
            {
                continue;
            }
            else
            {
                t42log_error("select() failed with error code %d\n", WSAGetLastError());
                break;
            }
#else
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                t42log_error("select() failed with error code %d\n", errno);
                break;
            }
#endif
        }


        HandlePings();

    }
}


void UPAProvider::CreateNewClientConnection()
{
    // find the first free client session 
    int nextIndex = -1;
    for(int index = 0; index < maxClientConnections; index ++)
    {
        if (clientConnections_[index].clientChannel == 0)
        {
            nextIndex = index;
            break;
        }
    }

    RsslAcceptOptions acceptOpts = RSSL_INIT_ACCEPT_OPTS;
    if (nextIndex == -1)
    {
        // need to fails the connection request
        acceptOpts.nakMount = RSSL_TRUE;
    }
    else
    {
        acceptOpts.nakMount = RSSL_FALSE;
    }

    RsslChannel * chnl;
    RsslError error;
    if ((chnl = rsslAccept(rsslServer_, &acceptOpts, &error)) == 0)
    {
        // if the accpet fails on the socket, just return
        t42log_error("rsslAccept: failed <%s>\n",error.text);
        return;
    }

    // set the channel
    clientConnections_[nextIndex].clientChannel = chnl;

    // and insert the channel into the dictionary
    ChannelDictionaryItem_t * dictItem = new ChannelDictionaryItem_t;
    dictItem->chnl_ = chnl;
    channelDictionary_[chnl] = dictItem;

    //printf("create client connection at index %d\n", nextIndex);

    t42log_info("Server fd=%d: New client on Channel fd=%d\n",
        rsslServer_->socketId,chnl->socketId);

    // and add this socket to the set if read fds
    FD_SET(chnl->socketId,&readfds_);
    FD_SET(chnl->socketId,&exceptfds_);


}


// read from an rssl channel

void UPAProvider::ReadFromChannel(RsslChannel* chnl)
{
    // first, handle state change
    int			retval;
    RsslError	error;

    if (chnl->socketId != -1 && chnl->state == RSSL_CH_STATE_INITIALIZING)
    {
        RsslInProgInfo inProg = RSSL_INIT_IN_PROG_INFO;
        // its initialising
        if ((retval = rsslInitChannel(chnl, &inProg, &error)) < RSSL_RET_SUCCESS)
        {
            // we fail to init the channel so just shut it down
            t42log_warn("sessionInactive fd=%d <%s>\n", chnl->socketId,error.text);
            RemoveChannelConnection(chnl);
        }
        else 
        {
            switch (retval)
            {
            case RSSL_RET_CHAN_INIT_IN_PROGRESS:
                if (inProg.flags & RSSL_IP_FD_CHANGE)
                {
                    // the FDs have changed
                    t42log_info("Channel In Progress - New FD: %d  Old FD: %d\n",chnl->socketId, inProg.oldSocket );

                    FD_CLR(inProg.oldSocket,&readfds_);
                    FD_CLR(inProg.oldSocket,&exceptfds_);
                    FD_SET(chnl->socketId,&readfds_);
                    FD_SET(chnl->socketId,&exceptfds_);
                }
                else
                {
                    t42log_debug("Channel %d connection in progress\n", chnl->socketId);
                }
                break;

            case RSSL_RET_SUCCESS:
                {
                    // initialization is complete

                    t42log_info("Client Channel fd=%d is now ACTIVE\n" ,chnl->socketId);
#ifdef _WIN32
                    // WINDOWS: change size of send/receive buffer since it's small by default 
                    int rcvBfrSize = 65535;
                    int sendBfrSize = 65535;

                    if (rsslIoctl(chnl, RSSL_SYSTEM_WRITE_BUFFERS, &sendBfrSize, &error) != RSSL_RET_SUCCESS)
                    {
                        t42log_warn("rsslIoctl(): failed <%s>\n", error.text);
                    }
                    if (rsslIoctl(chnl, RSSL_SYSTEM_READ_BUFFERS, &rcvBfrSize, &error) != RSSL_RET_SUCCESS)
                    {
                        t42log_warn("rsslIoctl(): failed <%s>\n", error.text);
                    }
#endif

                    /* if device we connect to supports connected component versioning, 
                    * also display the product version of what this connection is to */
                    RsslChannelInfo chanInfo;	
                    if ((retval = rsslGetChannelInfo(chnl, &chanInfo, &error)) >= RSSL_RET_SUCCESS)
                    {
                        RsslUInt32 i;
                        for (i = 0; i < chanInfo.componentInfoCount; i++)
                        {
                            t42log_info("Connection is from %s device.\n", chanInfo.componentInfo[i]->componentVersion.data);
                        }
                    }
                }
                break;

            default:
                t42log_error("Bad return value fd=%d <%s>\n", chnl->socketId,error.text);
                RemoveChannelConnection(chnl);
                break;
            }
        }
    }

    // the channel is active so read whatver there is to read
    if (chnl->socketId != -1 && chnl->state == RSSL_CH_STATE_ACTIVE)
    {
        RsslRet	readret = 1;

        while (readret > 0) /* read until no more to read */
        {
            RsslBuffer *msgBuf = 0 ;
            if ((msgBuf = rsslRead(chnl,&readret,&error)) != 0)
            {
                if (ProcessRequest(chnl, msgBuf) == RSSL_RET_SUCCESS)
                {
                    /* set flag for client message received */
                    SetChannelReceivedClientMsg(chnl);
                }
            }
            if (msgBuf == 0)
            {
                switch (readret)
                {
                case RSSL_RET_CONGESTION_DETECTED:
                case RSSL_RET_SLOW_READER:
                case RSSL_RET_PACKET_GAP_DETECTED:
                    if (chnl->state != RSSL_CH_STATE_CLOSED)
                    {
                        // disconnectOnGaps must be false.  Connection is not closed 
                        t42log_warn("Read Error: %s <%d>\n", error.text, readret);
                        /* break out of switch */
                        break;
                    }
                    /// if channel is closed, we want to fall through 
                case RSSL_RET_FAILURE:
                    {
                        t42log_info("channelInactive fd=%d <%s>\n",
                            chnl->socketId,error.text);
                        RemoveChannelConnection(chnl);

                    }
                    break;
                case RSSL_RET_READ_FD_CHANGE:
                    {
                        t42log_info("rsslRead() FD Change - Old FD: %d New FD: %d\n", chnl->oldSocketId, chnl->socketId);
                        FD_CLR(chnl->oldSocketId, &readfds_);
                        FD_CLR(chnl->oldSocketId, &exceptfds_);
                        FD_SET(chnl->socketId, &readfds_);
                        FD_SET(chnl->socketId, &exceptfds_);
                    }
                    break;
                case RSSL_RET_READ_PING: 
                    {
                        /* set flag for client message received */
                        SetChannelReceivedClientMsg(chnl);
                    }
                    break;
                default:
                    if (readret < 0 && readret != RSSL_RET_READ_WOULD_BLOCK)
                    {
                        t42log_warn("Read Error: %s <%d>\n", error.text, readret);

                    }
                    break;
                }
            }
        }
    }
    else if (chnl->state == RSSL_CH_STATE_CLOSED)
    {
        t42log_info("Channel fd=%d Closed.\n", chnl->socketId);
        RemoveChannelConnection(chnl);
    }



}


void UPAProvider::InitChannelPingTimes(UPAClientConnection_t * clientConnection)
{
    time_t currentTime = 0;

    /* get current time */
    time(&currentTime);

    // The ping timeout is obtained from the channel structure given to us by the server
    // we set the send ping much shorter than the requested timout (upa sdk sample uses factor of 3 - cant see any reason to do anything different here)
    clientConnection->nextSendPingTime = currentTime + (time_t)(clientConnection->clientChannel->pingTimeout/3);
    clientConnection->nextReceivePingTime = currentTime + (time_t)(clientConnection->clientChannel->pingTimeout);

    clientConnection->pingsInitialized = RSSL_TRUE;
}


void UPAProvider::ResetClientConnection(UPAClientConnection_t * connection)
{
    connection->clientChannel = 0;
    connection->nextReceivePingTime = 0;
    connection->nextSendPingTime = 0;
    connection->receivedClientMsg = RSSL_FALSE;
    connection->pingsInitialized = RSSL_FALSE;
}

void UPAProvider::ShutdownChannel(RsslChannel * chnl)
{
    RsslError error;
    RsslRet ret;

    // clean up the socket fds 
    FD_CLR(chnl->socketId, &readfds_);
    FD_CLR(chnl->socketId, &exceptfds_);
    if (FD_ISSET(chnl->socketId, &wrtfds_))
        FD_CLR(chnl->socketId, &wrtfds_);

    // and close the connection
    if ((ret = rsslCloseChannel(chnl, &error)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslCloseChannel() failed with return code: %d ('%s')\n", ret, error.text);
    }
}

void UPAProvider::ShutdownClientConnection(UPAClientConnection_t * connection)
{
    ShutdownChannel(connection->clientChannel);

    // and remove the channel from the dictionary
    RsslChannel * chnl = connection->clientChannel;
    ChannelDictionary_t::iterator it = channelDictionary_.find(chnl);
    if (it != channelDictionary_.end())
    {
        // shutdown all the channel related stuff
        ChannelDictionaryItem_t * dictItem = it->second;

        // now walk the streamID map for the channel
        // this will hold all the open streams on the channel which in turn will provide a reference to the publisher item that is
        // publishing on the stream
        ChannelStreamIdMap_t::iterator streamIt = dictItem->streamIdMap_.begin();

        while (streamIt != dictItem->streamIdMap_.end())
        {

            UPAPublisherItem_ptr_t item = streamIt->second;
            RsslUInt32 streamId = streamIt->first;
            if (!item->RemoveChannel(chnl, streamId))
            {
                // this will send a message to the mama client to tell it we dont want this any more
                owner_->CloseItem(item->Source(), item->Symbol());

            }
            ++streamIt;


        }

        // clear out the map
        dictItem->streamIdMap_.clear();

        // todo cleanup any remaining items when the channel is closed
        channelDictionary_.erase(it);

        delete dictItem;
    }

    ResetClientConnection(connection);
}

void UPAProvider::RemoveChannelConnection(RsslChannel * chnl)
{
    // find the channel
    for ( int index = 0; index < maxClientConnections; index ++)
    {
        if (clientConnections_[index].clientChannel == chnl)
        {
            // found it, shut it down
            ShutdownClientConnection(&clientConnections_[index]);
            break;
        }
    }
}


void UPAProvider::HandlePings()
{
    uint32_t now = utils::time::GetMilliCount();
    if ( (now > lastPingCheck_) && (now - lastPingCheck_ > pingCheckInterval))
    {

        lastPingCheck_ = now;

        // millicount hasnt rolled (linux) and we have exceeded the pin check interval, so do the stuff
        time_t currentTime = 0;

        // get the current time
        time(&currentTime);

        // now walk the array of connections  and check each for whether we need to do anything 
        for( int index = 0; index < maxClientConnections; index++)
        {
            if ((clientConnections_[index].clientChannel != NULL) && (clientConnections_[index].clientChannel->socketId != -1))
            {
                // we have a connection at this slot

                // check if we need to send a ping

                if ((clientConnections_[index].pingsInitialized == RSSL_TRUE) && (currentTime >= clientConnections_[index].nextSendPingTime))
                {
                    // send the ping
                    if (sendPing(clientConnections_[index].clientChannel) != RSSL_RET_SUCCESS)
                    {
                        // really need some logic to see if this is a persistent condition
                        // for the moment just let it try again
                    }
                    else
                    {
                        t42log_info("sent ping on channel %d\n", clientConnections_[index].clientChannel->socketId);
                        // was successfull so set time for next
                        clientConnections_[index].nextSendPingTime = currentTime + (time_t)clientConnections_[index].clientChannel->pingTimeout/10;
                    }
                }

                // received pings
                if ((clientConnections_[index].pingsInitialized == RSSL_TRUE) && (currentTime >= clientConnections_[index].nextReceivePingTime))
                {
                    // set this flag whenever we get a message in
                    if (clientConnections_[index].receivedClientMsg)
                    {
                        /* reset flag for client message received */
                        clientConnections_[index].receivedClientMsg = RSSL_FALSE;

                        /* set time server should receive next message/ping from client */
                        clientConnections_[index].nextReceivePingTime = currentTime + (time_t)(clientConnections_[index].clientChannel->pingTimeout);
                    }
                    else /* lost contact with client */
                    {
                        t42log_info("Lost contact with client fd=%d\n", clientConnections_[index].clientChannel->socketId);
                        ShutdownClientConnection(&clientConnections_[index]);
                    }
                }
            }
        }
    }
}



RsslRet UPAProvider::sendPing(RsslChannel* chnl)
{
    RsslError error;
    RsslRet ret = 0;

    if ((ret = rsslPing(chnl, &error)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslPing(): Failed on fd=%d with code %d\n", chnl->socketId, ret);
        return ret;
    }
    else if (ret > RSSL_RET_SUCCESS)
    {
        // rsslPing only writes the ping message, not anything else. It will return >1 if the is more stuff to write
        // in which case set the write socket
        FD_SET(chnl->socketId, &wrtfds_);
    }

    return RSSL_RET_SUCCESS;
}



RsslRet UPAProvider::ProcessRequest(RsslChannel* chnl, RsslBuffer* buffer)
{
    RsslRet ret = 0;
    RsslMsg msg = RSSL_INIT_MSG;
    RsslDecodeIterator dIter;

    // set up an iterator to decode the message
    rsslClearDecodeIterator(&dIter);
    rsslSetDecodeIteratorRWFVersion(&dIter, chnl->majorVersion, chnl->minorVersion);
    // and attach it top the message buffer
    rsslSetDecodeIteratorBuffer(&dIter, buffer);

    ret = rsslDecodeMsg(&dIter, &msg);				
    if (ret != RSSL_RET_SUCCESS)
    {
        t42log_error("UPAProvider::ProcessRequest - rsslDecodeMsg(): Error %d on channel fd=%d  Size %d \n", ret, chnl->socketId, buffer->length);
        RemoveChannelConnection(chnl);
        return RSSL_RET_FAILURE;
    }

    switch ( msg.msgBase.domainType )
    {
    case RSSL_DMT_LOGIN:
        if (ProcessLoginRequest(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
        {

            RemoveChannelConnection(chnl);
            return RSSL_RET_FAILURE;
        }
        break;
    case RSSL_DMT_SOURCE:

        if (ProcessSourceDirectoryRequest(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
        {
            RemoveChannelConnection(chnl);
            return RSSL_RET_FAILURE;
        }
        break;
    case RSSL_DMT_DICTIONARY:
        if (ProcessDictionaryRequest(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
        {
            RemoveChannelConnection(chnl);
            return RSSL_RET_FAILURE;
        }
        break;
    case RSSL_DMT_MARKET_PRICE:
    case RSSL_DMT_MARKET_BY_ORDER:
    case RSSL_DMT_SYMBOL_LIST:
    case RSSL_DMT_MARKET_BY_PRICE:
    case RSSL_DMT_YIELD_CURVE:
        if (ProcessItemRequest(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
        {
            RemoveChannelConnection(chnl);
            return RSSL_RET_FAILURE;
        }
        break;
    default:
          break;
    }


    return RSSL_RET_SUCCESS;
}


void UPAProvider::SetChannelReceivedClientMsg(RsslChannel *chnl)
{
    // find the connection and set the flag
    for (int index = 0; index < maxClientConnections; index ++)
    {
        if (clientConnections_[index].clientChannel == chnl)
        {
            clientConnections_[index].receivedClientMsg = RSSL_TRUE;
            break;
        }
    }
}

RsslRet UPAProvider::ProcessLoginRequest( RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter )
{

    RsslState *pState = 0;
    RsslMsgKey* key = 0;

    // shouldnt need to persist this
    login_.UPAChannel(chnl);
    UPALogin::RsslLoginRequestInfo loginRequestInfo;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REQUEST:
         //get key 
        key = (RsslMsgKey *)rsslGetMsgKey(msg);

         //check if key has user name 
         //user name is only login user type accepted by this application (user name is the default type) 
        if (!(key->flags & RSSL_MKF_HAS_NAME) || ((key->flags & RSSL_MKF_HAS_NAME_TYPE) && (key->nameType != RDM_LOGIN_USER_NAME)))
        {
            login_.SendLoginRequestReject(chnl, msg->msgBase.streamId, UPALogin::NoUserName);
            break;
        }


        // decode the login request 
        if (login_.DecodeLoginRequest(&loginRequestInfo, msg, dIter, key) != RSSL_RET_SUCCESS)
        {
            t42log_warn("UPAProvider::ProcessLoginRequest - decodeLoginRequest() failed\n");
            return RSSL_RET_FAILURE;
        }


        // at this point we can do whatever cheking we want and reject the login

        t42log_info("Received Login Request for Username: %.*s\n", strlen(loginRequestInfo.Username), loginRequestInfo.Username);


        // in the reuters UPA sample they build the login response info from the requestInfo inside the send function
        // We want to allow a space to set values from config  so build the structure here
        UPALogin::RsslLoginResponseInfo loginRespInfo;
        memset((void*)&loginRespInfo, 0, sizeof(UPALogin::RsslLoginResponseInfo));

        loginRespInfo.StreamId = loginRequestInfo.StreamId;
         //Username 
        snprintf(loginRespInfo.Username, 128, "%s", loginRequestInfo.Username);
         //ApplicationId 
        snprintf(loginRespInfo.ApplicationId, 128, "%s", "255");
         //ApplicationName 
        snprintf(loginRespInfo.ApplicationName, 128, "%s", "Bridge");
         //Position 
        snprintf(loginRespInfo.Position, 128, "%s", loginRequestInfo.Position);

        loginRespInfo.SingleOpen = 0;				/* this provider does not support SingleOpen behavior */
        loginRespInfo.SupportBatchRequests = 0;		/* this provider supports batch requests */
        loginRespInfo.SupportOMMPost = acceptsPosts_ ? 1 : 0;

        // send login response 
        if (login_.SendLoginResponse(chnl, &loginRespInfo) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;



        owner_->SentLoginResponse();


        break;

    case RSSL_MC_CLOSE:
        t42log_info("Received Login Close for StreamId %d\n", msg->msgBase.streamId);

        // dont actually need to do anything as we are not persisting login information

        break;

    default:
        t42log_warn("Received unhandled Login Msg Class: %d\n", msg->msgBase.msgClass);
        return RSSL_RET_FAILURE;

    }



    return RSSL_RET_SUCCESS;
}

RsslRet UPAProvider::ProcessSourceDirectoryRequest( RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter )
{

    // need to (a) check the filter flags - The rssl UPA provider sample does this. Not sure why its completely necessary - in any case we will send all the elements
    // (b) get the streamID - this get encodeded into the response
    // (c) add a map entry for each of the services - the upa sample only supports a single service but we potentially support many


    UPASourceDirectory srcDir;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REQUEST:
        {
            RsslMsgKey* key = 0;
            key = (RsslMsgKey *)rsslGetMsgKey(msg);
            if (!((key->flags & RSSL_MKF_HAS_FILTER) && (key->filter & RDM_DIRECTORY_SERVICE_INFO_FILTER) 
                &&	(key->filter & RDM_DIRECTORY_SERVICE_STATE_FILTER) && (key->filter & RDM_DIRECTORY_SERVICE_GROUP_FILTER)))
            {
                if (srcDir.SendSrcDirectoryRequestReject(chnl, msg->msgBase.streamId, UPASourceDirectory::IncorrectFilterFlags) != RSSL_RET_SUCCESS)
                {
                        return RSSL_RET_FAILURE;
                }
                break;
            }

            RsslInt32 streamId = msg->requestMsg.msgBase.streamId;
            srcDir.SendSourceDirectoryResponse(chnl, streamId, key, owner_->GetSource());
        }
        break;

    case RSSL_MC_CLOSE:
        t42log_debug("Received Source Directory Close for StreamId %d\n", msg->msgBase.streamId);

        // dont actually need to do anything as we dont persist the info at the moment. If mama allowed us to report and publisjh source state then we would close the persistsed source data here

        break;

    default:
        t42log_warn("Received Unhandled Source Directory Msg Class: %d\n", msg->msgBase.msgClass);
        return RSSL_RET_FAILURE;

        break;

    }



    return RSSL_RET_SUCCESS;
}



RsslRet UPAProvider::ProcessDictionaryRequest( RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter )
{

    return RSSL_RET_SUCCESS;
}

RsslRet UPAProvider::ProcessItemRequest(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter)
{

    RsslMsgKey* key = 0;

    RsslUInt8 domainType = msg->msgBase.domainType;

    RsslInt32 streamId = msg->msgBase.streamId;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REQUEST:
        // get key 
        {
            key = (RsslMsgKey *)rsslGetMsgKey(msg);

            bool isPrivateStream = (msg->requestMsg.flags & RSSL_RQMF_PRIVATE_STREAM) > 0 ? RSSL_TRUE : RSSL_FALSE;
            RMDSPublisherSource *source = owner_->GetSource();

            // insert the sourcename into the new item request as we cant get it from the servioce id
            if ( source == 0)
            {
                if (UPAPublisherItem::SendItemRequestReject(chnl, streamId, domainType, RSSL_SC_USAGE_ERROR, "Invalid service id",  isPrivateStream) != RSSL_RET_SUCCESS)
                    return RSSL_RET_FAILURE;
                break;
            }


            bool isNew;

            UPAPublisherItem_ptr_t newPubItem = source->AddItem(chnl, streamId, source->Name(), string(key->name.data, key->name.length), key->serviceId, owner_, isNew);

            // now need to add this item to our channel dictionary so we can find it again when we get a close
            ChannelDictionaryItem_t * dictItem = 0;
            ChannelDictionary_t::iterator it = channelDictionary_.find(chnl);
            if (it == channelDictionary_.end())
            {
                t42log_error("received item request for %s : %s on unknown channel (%d)\n", source->Name().c_str(), string(key->name.data, key->name.length).c_str(), chnl);
                break;
            }

            // so now we can insert the stream it in the dictionary item
            dictItem = it->second;
            dictItem->streamIdMap_[streamId] = newPubItem;

            // request a new item. Send a message to the client
            //
            // if its not a new item we want to request a recap, that will be send on the new channel / stream
            owner_->RequestItem(source->Name(), string(key->name.data, key->name.length), !isNew);
        } 
        break;



    case RSSL_MC_CLOSE:

        {
            t42log_debug("Received Item Close for StreamId %d\n", streamId);

            // find the item in the channel dictionary
            ChannelDictionary_t::iterator it = channelDictionary_.find(chnl);

            if (it == channelDictionary_.end())
            {
                t42log_error("received close item request for stream %d on unknown channel (%d)\n", streamId, chnl);
                break;
            }

            ChannelDictionaryItem_t * dictItem = it->second;

            // now look up the stream id
            ChannelStreamIdMap_t::iterator streamIt = dictItem->streamIdMap_.find(streamId);

            if (streamIt == dictItem->streamIdMap_.end())
            {
                t42log_warn("received item close for unknown stream id %d on channel %d \n", streamId, chnl);
                break;
            }

            UPAPublisherItem_ptr_t item = streamIt->second;
            dictItem->streamIdMap_.erase(streamIt);
            if (!item->RemoveChannel(chnl, streamId))
            {
                // this will send a message to the mama client to tell iut we dont want this any more
                owner_->CloseItem(item->Source(), item->Symbol());

            }

        }



        break;

    case RSSL_MC_POST:
        if (ProcessPost(chnl, msg, dIter) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
        break;

    default:
        printf("Received Unhandled Item Msg Class: %d\n", msg->msgBase.msgClass);
        break;
    }

    return RSSL_RET_SUCCESS;

}

RsslRet UPAProvider::ProcessPost( RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter )
{

    RsslPostMsg	*postMsg = &msg->postMsg;
    SendAck(chnl, postMsg, RSSL_NAKC_DENIED_BY_SRC, "Provider doesnt accept posts");
    t42log_warn("Accepting posted messages requires the enhanced version of the RMDS bridge\n");
    return RSSL_RET_SUCCESS;
}

RsslRet UPAProvider::SendAck(RsslChannel *chnl, RsslPostMsg *postMsg, RsslUInt8 nakCode, char *errText)
{
    RsslBuffer *ackBuf;
    RsslError error;
    RsslRet ret;

    // send an ack if it was requested
    if (postMsg->flags & RSSL_PSMF_ACK)
    {
        if ((ackBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error)) == NULL)
        {
            t42log_error(" UPAProvider::SendAc - rsslGetBuffer() Failed (rsslErrorId = %d)\n", error.rsslErrorId);
            return RSSL_RET_FAILURE;
        }

        if ((ret = EncodeAck(chnl, ackBuf, postMsg, nakCode, errText)) < RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(ackBuf, &error); 
            t42log_error(" UPAProvider::SendAck - encodeAck() Failed (ret = %d)\n", ret);
            return RSSL_RET_FAILURE;
        }

        if (SendUPAMessage(chnl, ackBuf) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}

RsslRet UPAProvider::EncodeAck(RsslChannel* chnl, RsslBuffer* ackBuf, RsslPostMsg *postMsg, RsslUInt8 nakCode, char *text)
{
    RsslRet ret = 0;
    RsslAckMsg ackMsg = RSSL_INIT_ACK_MSG;
    RsslEncodeIterator encodeIter;

    /* clear encode iterator */
    rsslClearEncodeIterator(&encodeIter);

    /* set-up message */
    ackMsg.msgBase.msgClass = RSSL_MC_ACK;
    ackMsg.msgBase.streamId = postMsg->msgBase.streamId;
    ackMsg.msgBase.domainType = postMsg->msgBase.domainType;
    ackMsg.msgBase.containerType = RSSL_DT_NO_DATA;
    ackMsg.flags = RSSL_AKMF_NONE;
    ackMsg.nakCode = nakCode;
    ackMsg.ackId = postMsg->postId;
    ackMsg.seqNum = postMsg->seqNum;

    if (nakCode != RSSL_NAKC_NONE)
        ackMsg.flags |= RSSL_AKMF_HAS_NAK_CODE;

    if (postMsg->flags & RSSL_PSMF_HAS_SEQ_NUM)
        ackMsg.flags |= RSSL_AKMF_HAS_SEQ_NUM;

    if (text != NULL) 
    {
        ackMsg.flags |= RSSL_AKMF_HAS_TEXT;
        ackMsg.text.data = text;
        ackMsg.text.length = (RsslUInt32)strlen(text);
    }

    /* encode message */
    rsslSetEncodeIteratorBuffer(&encodeIter, ackBuf);
    rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
    if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&ackMsg)) < RSSL_RET_SUCCESS)
    {
        t42log_error("UPAProvider::EncodeAck - rsslEncodeMsg() of ackMsg failed with return code: %d\n", ret);
        return ret;
    }

    ackBuf->length = rsslGetEncodedBufferLength(&encodeIter);
    return RSSL_RET_SUCCESS;
}


