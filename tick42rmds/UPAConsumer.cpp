/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2015 Tick42 Ltd.
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

#include "RMDSSubscriber.h"
#include "UPALogin.h"
#include "UPASourceDirectory.h"
#include "UPADictionary.h"
#include "UPABridgePoster.h"
#include "UPAConsumer.h"
#include "transportconfig.h"

#include <utils/HiResTime.h>
#include <utils/t42log.h>
#include <utils/os.h>
#include <utils/time.h>

const int32_t StatsSampleInterval = 10000;

UPAConsumer::UPAConsumer(RMDSSubscriber * pOwner)
   : shouldRecoverConnection_(RSSL_TRUE)
   , rsslConsumerChannel_(NULL)
   , receivedServerMsg_ (RSSL_FALSE)
   , connectionConfig_(pOwner->Config()->getString("hosts"), pOwner->Config()->getString("retrysched", Default_retrysched))
   ,requiresConnection_(true)
{
   isInLoginSuspectState_ = RSSL_FALSE;
   owner_ = pOwner;
   if (pOwner->InterfaceName())
   {
      interfaceName_ = ::strdup(pOwner->InterfaceName());
   }
   else
   {
      interfaceName_ = NULL;
   }

   // if no hosts list dont try and connect
   // this lets us run a subscriber that is not connected - allows mama to subscribe for special topis such as new item request that are created by the publisher
   // then marshalled into a subscriber

	if (connectionConfig_.NumHosts() == 0)
	{
		t42log_warn("No hosts specified for transport %s - WILL NOT ATTEMPT CONNECTION\n", pOwner->GetTransportName().c_str());
		shouldRecoverConnection_ = RSSL_FALSE;
		requiresConnection_ = false;

	}

    connType_ = pOwner->ConnType();
	requestQueue_ = pOwner->GetRequestQueue();

	TransportConfig_t config(pOwner->GetTransportName());

	login_ = new UPALogin(false);
	login_->ConfigureEntitlements(&config);       // Use the DACS settings in the configuration to login

	// config for throttle
	maxDispatchesPerCycle_ = config.getInt("maxdisp", Default_maxdisp);
	maxPendingOpens_ = config.getInt("maxPending", Default_maxPending);
	t42log_info("Consumer thread request throttle parameters - max dispatches = %d max pending = %d\n", maxDispatchesPerCycle_, maxPendingOpens_);

	bool configDisableDataConversion = config.getBool("disabledataconversion",false);

	// initialise the source directory and dictionary management components
	login_->AddListener(pOwner);
	login_->DisableDataConversion(configDisableDataConversion);
	sourceDirectory_ = new UPASourceDirectory();
	sourceDirectory_->AddListener(pOwner);
	upaDictionary_ = boost::make_shared<UPADictionary>(pOwner->GetTransportName());
	upaDictionary_->AddListener(pOwner);


	// init statistics
	incomingMessageCount_ = 0;
	lastMessageCount_ = 0;
	lastSampleTime_ = utils::time::GetMilliCount();
	totalSubscriptions_ = totalSubscriptionsSucceeded_ = totalSubscriptionsFailed_ = 0;


	// create the shared mama message
	mamaMsg_createForPayload(&msg_, MAMA_PAYLOAD_TICK42RMDS);


   runThread_ = true;

}

UPAConsumer::~UPAConsumer(void)
{
   delete login_;
   delete sourceDirectory_;
}



void UPAConsumer::Run()
{
   RsslError error;
   struct timeval time_interval;
   RsslRet	retval = 0;

   fd_set useRead;
   fd_set useExcept;
   fd_set useWrt;

   // get hold of the statistics logger
   statsLogger_ = StatisticsLogger::GetStatisticsLogger();

   // todo might want to make these configurable
#ifdef _WIN32
   int rcvBfrSize = 65535;
   int sendBfrSize = 65535;
#endif

   int selRet;

   FD_ZERO(&readfds_);
   FD_ZERO(&exceptfds_);
   FD_ZERO(&wrtfds_);

   // The outer thread loop attempts to make / remake a connection to the rssl server
   //
   // Inner loop dispatches requests from the mama queue and reads the socket for incoming responses
   while (runThread_)
   {
      // attempt to make connection
      while(shouldRecoverConnection_ && runThread_)
      {
         login_->UPAChannel(0);
         sourceDirectory_->UPAChannel(0);
         upaDictionary_->UPAChannel(0);
         // connect to server
         t42log_info("Attempting to connect to server %s:%s...\n", connectionConfig_.Host().c_str(), connectionConfig_.Port().c_str());

         if ((rsslConsumerChannel_ = ConnectToRsslServer(connectionConfig_.Host(), connectionConfig_.Port(), interfaceName_, connType_, &error)) == NULL)
         {
            t42log_error("Unable to connect to RSSL server: <%s>\n",error.text);
         }
         else
         {
            if (rsslConsumerChannel_)
            {
               // connected - set up socket fds
               RsslSocket s = rsslConsumerChannel_->socketId;
               FD_SET(rsslConsumerChannel_->socketId, &readfds_);
               FD_SET(rsslConsumerChannel_->socketId, &wrtfds_);
               FD_SET(rsslConsumerChannel_->socketId, &exceptfds_);
            }
         }

         if (rsslConsumerChannel_ != NULL && rsslConsumerChannel_->state == RSSL_CH_STATE_ACTIVE)
            shouldRecoverConnection_ = RSSL_FALSE;

         //Wait for channel to become active. 
         while (rsslConsumerChannel_ != NULL && rsslConsumerChannel_->state != RSSL_CH_STATE_ACTIVE && runThread_)
         {
            useRead = readfds_;
            useWrt = wrtfds_;
            useExcept = exceptfds_;

            /* Set a timeout value if the provider accepts the connection, but does not initialize it */
            time_interval.tv_sec = 60;
            time_interval.tv_usec = 0;

            selRet = select(FD_SETSIZE, &useRead, &useWrt, &useExcept, &time_interval);

            // select has timed out, close the channel and attempt to reconnect 
            if (selRet == 0)
            {
               t42log_warn("Channel initialization has timed out, attempting to reconnect...\n");

               if (rsslConsumerChannel_)
               {
                  FD_CLR(rsslConsumerChannel_->socketId, &readfds_);
                  FD_CLR(rsslConsumerChannel_->socketId, &exceptfds_);
                  if (FD_ISSET(rsslConsumerChannel_->socketId, &wrtfds_))
                     FD_CLR(rsslConsumerChannel_->socketId, &wrtfds_);
               }
               RecoverConnection();
            }
            else
               // Received a response from the provider. 
               if (rsslConsumerChannel_ && selRet > 0 && (FD_ISSET(rsslConsumerChannel_->socketId, &useRead) || FD_ISSET(rsslConsumerChannel_->socketId, &useWrt) || FD_ISSET(rsslConsumerChannel_->socketId, &useExcept)))
               {
                  if (rsslConsumerChannel_->state == RSSL_CH_STATE_INITIALIZING)
                  {
                     RsslInProgInfo inProg = RSSL_INIT_IN_PROG_INFO;
                     FD_CLR(rsslConsumerChannel_->socketId,&wrtfds_);
                     if ((retval = rsslInitChannel(rsslConsumerChannel_, &inProg, &error)) < RSSL_RET_SUCCESS)
                     {
                        // channel init failed, try again
                        t42log_warn("channelInactive fd=%d <%s>\n",
                           rsslConsumerChannel_->socketId,error.text);
                        RecoverConnection();
                        break; 
                     }
                     else 
                     {
                        switch ((int)retval)
                        {
                        case RSSL_RET_CHAN_INIT_IN_PROGRESS:
                           if (inProg.flags & RSSL_IP_FD_CHANGE)
                           {
                              t42log_info("Channel In Progress - New FD: %d  Old FD: %d\n",rsslConsumerChannel_->socketId, inProg.oldSocket );

                              FD_CLR(inProg.oldSocket,&readfds_);
                              FD_CLR(inProg.oldSocket,&exceptfds_);
                              FD_SET(rsslConsumerChannel_->socketId,&readfds_);
                              FD_SET(rsslConsumerChannel_->socketId,&exceptfds_);
                              FD_SET(rsslConsumerChannel_->socketId,&wrtfds_);
                           }
                           else
                           {
                              t42log_info("Channel %d In Progress...\n", rsslConsumerChannel_->socketId);
                           }
                           break;
                        case RSSL_RET_SUCCESS:
                           {
                              // connected
                              t42log_info("Channel %d Is Active\n" ,rsslConsumerChannel_->socketId);
                              shouldRecoverConnection_ = RSSL_FALSE;
                              RsslChannelInfo chanInfo;

                              // log channel info
                              if ((retval = rsslGetChannelInfo(rsslConsumerChannel_, &chanInfo, &error)) >= RSSL_RET_SUCCESS)
                              {
                                 RsslUInt32 i;
                                 for (i = 0; i < chanInfo.componentInfoCount; i++)
                                 {
                                    t42log_info("Connected to %s device.\n", chanInfo.componentInfo[i]->componentVersion.data);
                                 }
                              }


                              login_->UPAChannel(rsslConsumerChannel_);
                              sourceDirectory_->UPAChannel(rsslConsumerChannel_);
                              upaDictionary_->UPAChannel(rsslConsumerChannel_);
                              mama_log (MAMA_LOG_LEVEL_FINEST, "Provider returned \"Connection Successful\"");
                              NotifyListeners(true, "Provider returned \"Connection Successful\"");

                           }
                           break;
                        default:
                           {
                              // Connection has failed
                              char buff[256]={0};
                              memset(buff,0,sizeof(buff));
                              sprintf(buff, "Bad return value on connection  fd=%d <%s>\n", rsslConsumerChannel_->socketId,error.text);
                              t42log_error("%s\n", buff);
                              NotifyListeners(false, buff);
                              runThread_ = false;
                           }
                           break;
                        }
                     }
                  }
               }
               else
                  if (selRet < 0)
                  {
                     t42log_error("Select error.\n");
                     runThread_ = false;
                     break;
                  }

         }

         // wait a while before retry
         if (shouldRecoverConnection_)
         {
            LogReconnection();
            WaitReconnectionDelay();

         }
      }

      //  WINDOWS: change size of send/receive buffer since it's small by default 
#ifdef _WIN32
      if (rsslConsumerChannel_ && rsslIoctl(rsslConsumerChannel_, RSSL_SYSTEM_WRITE_BUFFERS, &sendBfrSize, &error) != RSSL_RET_SUCCESS)
      {
         t42log_error("rsslIoctl(): failed <%s>\n", error.text);
      }
      if (rsslConsumerChannel_ && rsslIoctl(rsslConsumerChannel_, RSSL_SYSTEM_READ_BUFFERS, &rcvBfrSize, &error) != RSSL_RET_SUCCESS)
      {
         t42log_error("rsslIoctl(): failed <%s>\n", error.text);
      }
#endif

      /* Initialize ping handler */
      if (rsslConsumerChannel_) 
         InitPingHandler(rsslConsumerChannel_);

      int64_t queueCount = 0;
      /* this is the message processing loop */
      while (runThread_)
      {

         // first dispatch some events off the event queue
         if (!PumpQueueEvents())
         {
            break;
         }

		 if ((rsslConsumerChannel_ != NULL) && (rsslConsumerChannel_->socketId != -1))
		 {
			 // if we have a connection
			 useRead = readfds_;
			 useExcept = exceptfds_;
			 useWrt = wrtfds_;
			 time_interval.tv_sec = 0;
			 time_interval.tv_usec = 100000;

			 // look at the socket state

			 selRet = select(FD_SETSIZE, &useRead, &useWrt, &useExcept, &time_interval);
		 }
		 else
		 {
			 // no connection, just sleep for 100 millis
			 sleep(100);
			 continue;
		 }


         if (!runThread_)
         {
			 // thread is stopped
            break;
         }

         if (selRet < 0) // no messages received, continue 
         {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEINTR)
               continue;
#else
            if (errno == EINTR)
            {
               continue;
            }
#endif
         }
         else if (selRet > 0) // messages received
         {
            if ((rsslConsumerChannel_ != NULL) && (rsslConsumerChannel_->socketId != -1))
            {
               if ((FD_ISSET(rsslConsumerChannel_->socketId, &useRead)) ||
                  (FD_ISSET(rsslConsumerChannel_->socketId, &useExcept)))
               {
				   // This will empty the read buffer and dispatch incoming events
                  if (ReadFromChannel(rsslConsumerChannel_) != RSSL_RET_SUCCESS)
                  {
                     // the read failed so attempt to recover if required
					 if(RSSL_TRUE == shouldRecoverConnection_)
					 {
						RecoverConnection();
					 }
					 else
					 {
						 // otherwise just run out of the thread
						 runThread_ = false;
					 }
                  }
               }

               // If there's anything to be written flush the write socket
               if (rsslConsumerChannel_ != NULL &&
                  FD_ISSET(rsslConsumerChannel_->socketId, &useWrt) &&
                  rsslConsumerChannel_->state == RSSL_CH_STATE_ACTIVE)
               {
                  if ((retval = rsslFlush(rsslConsumerChannel_, &error)) < RSSL_RET_SUCCESS)
                  {
                     t42log_error("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
                  }
                  else if (retval == RSSL_RET_SUCCESS)
                  {
                     // and clear the fd
                     FD_CLR(rsslConsumerChannel_->socketId, &wrtfds_);
                  }
               }
            }
         }

         // break out of message processing loop if should recover connection 
         if (shouldRecoverConnection_ == RSSL_TRUE)
         {
            LogReconnection();
            WaitReconnectionDelay();
            break;
         }

         // check if its time to process pings
         if (rsslConsumerChannel_)
         {
            ProcessPings(rsslConsumerChannel_);
         }
      }
   }

   	  // thread has stopped
   RemoveChannel(rsslConsumerChannel_);

   t42log_debug("Exit UPAConsumer thread\n");
}

RsslChannel* UPAConsumer::ConnectToRsslServer(const std::string &hostname, const std::string &port, char* interfaceName, RsslConnectionTypes connType, RsslError* error)
{
   RsslChannel* chnl;
   RsslConnectOptions connectOpts;// = RSSL_INIT_CONNECT_OPTS;		
   //manual initialization for connectOpts based on RSSL_INIT_CONNECT_OPTS - only values that are different than zero
   memset(&connectOpts,0,sizeof(connectOpts));
   connectOpts.pingTimeout=60;
   connectOpts.guaranteedOutputBuffers=50;
   connectOpts.numInputBuffers=10;
   connectOpts.multicastOpts.packetTTL=5;


   connectOpts.guaranteedOutputBuffers = 500;

   connectOpts.connectionInfo.unified.address = const_cast<char *>(hostname.c_str());
   connectOpts.connectionInfo.unified.serviceName = const_cast<char *>(port.c_str());
   connectOpts.connectionInfo.unified.interfaceName = interfaceName;
   connectOpts.connectionType = connType;
   connectOpts.majorVersion = RSSL_RWF_MAJOR_VERSION;
   connectOpts.minorVersion = RSSL_RWF_MINOR_VERSION;
   connectOpts.protocolType = RSSL_RWF_PROTOCOL_TYPE;

   if ( (chnl = rsslConnect(&connectOpts,error)) != 0)
   {
      FD_SET(chnl->socketId,&readfds_);
      FD_SET(chnl->socketId,&exceptfds_);

      t42log_info("Channel IPC descriptor = %d\n", chnl->socketId);
      if (!connectOpts.blocking)
      {	
         if (!FD_ISSET(chnl->socketId,&wrtfds_))
            FD_SET(chnl->socketId,&wrtfds_);
      }
   }

   return chnl;
}



void UPAConsumer::RecoverConnection()
{

   // notify listeners that we are not connected
   mama_log (MAMA_LOG_LEVEL_FINE, "Recovering Connection");
   NotifyListeners(false, "Recovering Connection");

   // reset the channel
   if ((rsslConsumerChannel_ != NULL) && (rsslConsumerChannel_->socketId != -1))
   {
      RemoveChannel(rsslConsumerChannel_);
      rsslConsumerChannel_ = NULL;
   }
   // and flag a reconnect
   shouldRecoverConnection_ = RSSL_TRUE;

}

void UPAConsumer::RemoveChannel(RsslChannel* chnl)
{

	// its already been shutdown
	if(0 == chnl)
	{
		return;
	} 
   RsslError error = { 0, 0, 0, { '\0' } };
   RsslRet ret;

   // tidy up the fds and close the channel
   FD_CLR(chnl->socketId, &readfds_);
   FD_CLR(chnl->socketId, &exceptfds_);
   if (FD_ISSET(chnl->socketId, &wrtfds_))
   {
      FD_CLR(chnl->socketId, &wrtfds_);
   }

   if ((ret = rsslCloseChannel(chnl, &error)) < RSSL_RET_SUCCESS)
   {
      t42log_error("rsslCloseChannel() failed with return code: %d\n", ret);
   }
}

// Stop the UPAConsumer thread, which will also shut down RSSL
void UPAConsumer::JoinThread(wthread_t thread)
{
   if (0 != statsLogger_)
   {      
      statsLogger_->Stop();
   }

   t42log_info("stopping UPAConsumer thread\n");
   runThread_ = false;

   if (0 != thread)
   {
      // And wait for the thread to exit
      wthread_join(thread, NULL);
      wthread_destroy(thread);
   }
}

// initialise the ping timer with current times
void UPAConsumer::InitPingHandler(RsslChannel* chnl)
{
   time_t currentTime = 0;

   // get the current time 
   time(&currentTime);

   // set the ping timeout for client and server 
   if ((chnl != NULL) && (chnl->socketId != -1))
   {
      pingTimeoutClient_ = chnl->pingTimeout/3;
      pingTimeoutServer_ = chnl->pingTimeout;
   }
   else
   {
      pingTimeoutClient_ = 20;
      pingTimeoutServer_ = 60;
   }

   // set time to send next ping from client 
   nextSendPingTime_ = currentTime + (time_t)pingTimeoutClient_;

   // set time client should receive next message/ping from server 
   nextReceivePingTime_ = currentTime + (time_t)pingTimeoutServer_;
}


// Read data from the rssl channel and process

RsslRet UPAConsumer::ReadFromChannel(RsslChannel* chnl)
{
   RsslError		error;
   RsslBuffer *msgBuf=0;
   RsslRet	readret;

   int maxReadTime = (pingTimeoutClient_*1000)/2;
   if (chnl->socketId != -1 && chnl->state == RSSL_CH_STATE_ACTIVE)
   {

	int32_t start = utils::time::GetMilliCount();
      readret = 1;

	  // keep reading 'til nothing left or we have used half the ping interval
	  // we put int he second condition in order that when the input rate is high we still get the opportunity to send pings and other outgoing data

      while (readret > 0 || utils::time::GetMilliSpan(start) > maxReadTime )  
      {

         if ((msgBuf = rsslRead(chnl,&readret,&error)) != 0)
         {
            if (ProcessResponse(chnl, msgBuf) == RSSL_RET_SUCCESS)	
            {
               /* set flag for server message received */
               receivedServerMsg_ = RSSL_TRUE;
            }
            else
            {
               // this will trigger a reconnection so process response needs to be circumspect about return value
               return RSSL_RET_FAILURE;
            }
         }
         else
         {
            switch (readret)
            {
            case RSSL_RET_CONGESTION_DETECTED:
            case RSSL_RET_SLOW_READER:
            case RSSL_RET_PACKET_GAP_DETECTED:
               {
                  if (chnl->state != RSSL_CH_STATE_CLOSED)
                  {
                     // disconnectOnGaps must be false.  Connection is not closed 
                     t42log_error("Read Error: %s <%d>\n", error.text, readret);
                     break;
                  }
                  // if channel is closed, we want to fall through to force a reconnect
               }
            case RSSL_RET_FAILURE:
               {
                  t42log_warn("channelInactive fd=%d <%s>\n",
                     chnl->socketId,error.text);
                  RecoverConnection();
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
                  //set flag for server message received 
                  receivedServerMsg_ = RSSL_TRUE;
               }
               break;
            default:
               if (readret < 0 && readret != RSSL_RET_READ_WOULD_BLOCK)
               {
                  t42log_error("Read Error: %s <%d>\n", error.text, readret);

               }
               break;
            }
         }
      }
   }
   else if (chnl->state == RSSL_CH_STATE_CLOSED)
   {
      t42log_warn("Channel fd=%d Closed.\n", chnl->socketId);
      RecoverConnection();
   }

   return RSSL_RET_SUCCESS;
}


// check ping times and handle pings as required
void UPAConsumer::ProcessPings(RsslChannel* chnl)
{
   time_t currentTime = 0;

   // get current time 
   time(&currentTime);

   // time to send a ping
   if (currentTime >= nextSendPingTime_)
   {
      /* send ping to server */
      if (chnl && SendPing(chnl) != RSSL_RET_SUCCESS)
      {
		RecoverConnection();
         return;
      }

      // schedule next
      nextSendPingTime_ = currentTime + (time_t)pingTimeoutClient_;
   }

   // time to check if we received anything
   if (currentTime >= nextReceivePingTime_)
   {
      //  check if client received message from server since last time 
      if (receivedServerMsg_)
      {
         // reset flag/
         receivedServerMsg_ = RSSL_FALSE;

         // schedule next time

         nextReceivePingTime_ = currentTime + (time_t)pingTimeoutServer_;
      }
      else // lost contact
      {
         t42log_error("Lost contact with server...\n");
         RecoverConnection();
         return;
      }
   }
}


// Process a channel response
// Decode the higher level message elements that when we have determined the message type
// pass to the appropriate handler
RsslRet UPAConsumer::ProcessResponse(RsslChannel* chnl, RsslBuffer* buffer)
{
   RsslRet ret = 0;
   RsslMsg msg = RSSL_INIT_MSG;
   RsslDecodeIterator dIter;
   UPALogin::RsslLoginResponseInfo *loginRespInfo = NULL;

   // bump counter
   //++incomingMessageCount_;
   statsLogger_->IncIncomingMessageCount();

   // reset the decode iterator
   rsslClearDecodeIterator(&dIter);

   // set version info 
   rsslSetDecodeIteratorRWFVersion(&dIter, chnl->majorVersion, chnl->minorVersion);

   if ((ret = rsslSetDecodeIteratorBuffer(&dIter, buffer)) != RSSL_RET_SUCCESS)
   {
      t42log_error("rsslSetDecodeIteratorBuffer() failed with return code: %d\n", ret);
      return RSSL_RET_FAILURE;
   }

   // Decode the message - note this only decode the outer parts of the message
   ret = rsslDecodeMsg(&dIter, &msg);				
   if (ret != RSSL_RET_SUCCESS)
   {
      t42log_error("rsslDecodeMsg(): Error %d on SessionData fd=%d  Size %d \n", ret, chnl->socketId, buffer->length);
      return RSSL_RET_FAILURE;
   }

   switch ( msg.msgBase.domainType )
   {
   case RSSL_DMT_LOGIN:
      if (login_->processLoginResponse(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
      {
		 t42log_info("ProcessResponse: cl=%d, rec=%d sus=%d notentitled=%d",
		 	login_->IsClosed(), login_->IsClosedRecoverable(), login_->IsSuspect(), login_->IsNotEntitled());
         if (login_->IsNotEntitled())
         {
		 	shouldRecoverConnection_ = RSSL_FALSE;
            return RSSL_RET_FAILURE;
         }
         else if (login_->IsClosed())
         {
            return RSSL_RET_FAILURE;
         }
         else if (login_->IsClosedRecoverable())
         {
            RecoverConnection();
         }
         else if (login_->IsSuspect())
         {
            isInLoginSuspectState_ = RSSL_TRUE;
         }
      }
      else
      {
         if (isInLoginSuspectState_)
         {
            isInLoginSuspectState_ = RSSL_FALSE;
         }
      }
      break;
   case RSSL_DMT_SOURCE:
      //			if (processSourceDirectoryResponse(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
      if (sourceDirectory_->ProcessSourceDirectoryResponse( &msg, &dIter) != RSSL_RET_SUCCESS)
         return RSSL_RET_FAILURE;

      break;
   case RSSL_DMT_DICTIONARY:
      if (upaDictionary_->ProcessDictionaryResponse(&msg, &dIter) != RSSL_RET_SUCCESS)
         return RSSL_RET_FAILURE;

      //TODO check that dictionary processing failure results in connection failure notification



      break;
   case RSSL_DMT_MARKET_PRICE:
      if (!isInLoginSuspectState_)
      {

         // lookup the subscription from the stream id
         RsslUInt32 streamId = msg.msgBase.streamId;


         if (streamId >= 16)
            // its either a regular mp update or an onstream ack
         {
            UPAItem_ptr_t item = streamManager_.GetItem(streamId); 	
            if (item.get() == 0)
            {
               // the item has been released - just ignore the update
               return RSSL_RET_SUCCESS;
            }

            if (item->Subscription()->ProcessMarketPriceResponse(&msg, &dIter) != RSSL_RET_SUCCESS)

               return RSSL_RET_FAILURE;
         }

         // otherwise is an ack or nak to an offstream post
         if (streamId == login_->StreamId())
         {
            // this isnt associated with a subscription so just have to process it here
            if (ProcessOffStreamResponse(&msg, &dIter) != RSSL_RET_SUCCESS)
            {
               return RSSL_RET_FAILURE;
            }
         }


      }
      break;
   case RSSL_DMT_MARKET_BY_ORDER:
      if (!isInLoginSuspectState_)
      {
         // lookup the subscription from the stream id
         RsslUInt32 streamId = msg.msgBase.streamId;

         UPAItem_ptr_t item = streamManager_.GetItem(streamId); 


         if (item.get() == 0)
         {
            // the item has been released - just ignore the update
            return RSSL_RET_SUCCESS;
         }

         if (item->Subscription()->ProcessMarketByOrderResponse(&msg, &dIter) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
      }
      break;
   case RSSL_DMT_MARKET_BY_PRICE:
      if (!isInLoginSuspectState_)
      {
         // lookup the subscription from the stream id
         RsslUInt32 streamId = msg.msgBase.streamId;

         UPAItem_ptr_t item = streamManager_.GetItem(streamId); 


         if (item.get() == 0)
         {
            // the item has been released - just ignore the update
            return RSSL_RET_SUCCESS;
         }

         if (item->Subscription()->ProcessMarketByPriceResponse(&msg, &dIter) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
      }
      break;
   case RSSL_DMT_YIELD_CURVE:

      break;
   case RSSL_DMT_SYMBOL_LIST:

      break;
   default:
      t42log_warn("Unhandled Domain Type: %d\n", msg.msgBase.domainType);
      break;
   }

   return RSSL_RET_SUCCESS;
}


// login request

void MAMACALLTYPE UPAConsumer::LoginRequestCb(mamaQueue queue,void *closure)
{
   UPALogin * pLogin = (UPALogin*)closure;
   pLogin->SendLoginRequest();

}

bool UPAConsumer::RequestLogin( mamaQueue requestQueue )
{
   // the UPALogin::QueueLogin function reports any errors

   // use the function that specifies the cb
   return login_->QueueLogin(requestQueue, UPAConsumer::LoginRequestCb);
}


// source directory request

void MAMACALLTYPE UPAConsumer::SourceDirectoryRequestCb(mamaQueue queue,void *closure)
{
   UPASourceDirectory * pSrcDirectory = (UPASourceDirectory*)closure;

   pSrcDirectory->SendRequest();

}

bool UPAConsumer::RequestSourceDirectory( mamaQueue requestQueue)
{
   //	sourceDirectory_->QueueRequest(requestQueue);
   sourceDirectory_->QueueRequest(requestQueue, UPAConsumer::SourceDirectoryRequestCb);

   return true;
}


// dictionary request

void MAMACALLTYPE UPAConsumer::DictionaryRequestCb(mamaQueue queue,void *closure)
{
   UPADictionary * pDictionary = (UPADictionary*)closure;

   // send the request to the rmds
   pDictionary->SendRequest();

}


//client  dictionary request

bool UPAConsumer::ClientRequestDictionary(mamaQueue requestQueue)
{
   return upaDictionary_->QueueMamaClientRequest(requestQueue);
}

void MAMACALLTYPE UPAConsumer::ClientDictionaryRequestCb(mamaQueue queue,void *closure)
{
   UPADictionary * pDictionary = (UPADictionary*)closure;

   // so if we already have a dictionary we can process and send to the client
   // if not, then do nothing as the dictionary will appear anyway as part of the bridge initialisation / connection
   if (pDictionary->IsComplete())
   {
      t42log_debug("Notify Dictionary complete\n");
      pDictionary->NotifyComplete();
   }
}

bool UPAConsumer::RequestDictionary( mamaQueue requestQueue )
{

   // if the dictionary is loaded from the source files already then we can 
   // notify now

   if (upaDictionary_->IsComplete())
   {
      t42log_debug("Notify Dictionary complete\n");
      upaDictionary_->NotifyComplete();
   }
   else
   {
      upaDictionary_->QueueRequest(requestQueue);
   }

   // todo check status
   return true;
}

// connection notification

void UPAConsumer::AddListener( ConnectionListener * pListener )
{
   listeners_.push_back(pListener);
}


void UPAConsumer::NotifyListeners( bool connected, std::string extraInfo )
{
   vector<ConnectionListener*>::iterator it = listeners_.begin();

   while(it != listeners_.end() )
   {
      (*it)->ConnectionNotification(connected, extraInfo);
      it++;
   }

}



RsslUInt32 UPAConsumer::LoginStreamId() const
{
   return login_->StreamId();
}

RsslRet UPAConsumer::ProcessOffStreamResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
   // handle ACK / NAK for off-stream posts

   if (msg->msgBase.msgClass == RSSL_MC_ACK)
   {
      t42log_debug("Received AckMsg for stream #%d", msg->msgBase.streamId);

      // get key 
      RsslMsgKey *key = 0;
      key = (RsslMsgKey *)rsslGetMsgKey(msg);

      UPABridgePoster_ptr_t poster;
      PublisherPostMessageReply *reply = 0;

      RsslUInt32 id = msg->ackMsg.ackId;
      if (postManager_.RemovePost(id, poster, reply))
      {
         poster->ProcessAck(msg, dIter, reply);
      }
      else
      {
         t42log_warn("received response for off-stream post with unknown id %d", id); 
      }
   }
   return RSSL_RET_SUCCESS;
}

bool UPAConsumer::PumpQueueEvents()
{
	// before we do anything else, process any pending subscriptions
   owner_->ProcessPendingSubcriptions();

   // Now dispatch any incoming events from the mama queue
   size_t numEvents = 0;

  mama_status status = mamaQueue_getEventCount(requestQueue_, &numEvents);
   if ((status == MAMA_STATUS_OK) && (numEvents > 0))
   {
      // although we only really want to throttle subscriptions, for simplicity throttle everything here.  
      size_t eventsDispatched = 0;

      // so keep dispatching while there are events on the queue, we haven't hit the max per cycle
      //and we haven't hit the max pending limit
      while ((numEvents > eventsDispatched)
         && (eventsDispatched < maxDispatchesPerCycle_)
         && (StreamManager().countPendingItems() < maxPendingOpens_))
      {
         if (!runThread_)
         {
            return false;
         }

         mamaQueue_dispatchEvent(requestQueue_);	
         ++eventsDispatched;
      }

      t42log_debug("dispatched %d requests \n", eventsDispatched);
   }

   // This MUST be outside the loop as there may be no further events when the final items
   // are opened
   statsLogger_->SetPendingOpens((int)StreamManager().countPendingItems());
   statsLogger_->SetPendingCloses((int)StreamManager().PendingCloses());
   statsLogger_->SetOpenItems((int)StreamManager().OpenItems());
   statsLogger_->SetRequestQueueLength((int)numEvents);
   return true;
}

void UPAConsumer::WaitReconnectionDelay()
{
   // wait for the specified time in the reconnection delay sequence.
   // but check every second to see if the thread has been shutdown
   for(unsigned  int i = 0; i < connectionConfig_.Delay(); ++i)
   {
      if (!runThread_)
         return;
#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
   }
   connectionConfig_.Next();
}

bool UPAConsumer::IsConnectionConfigValid()
{
   return connectionConfig_.Valid();
}

void UPAConsumer::LogReconnection()
{
   t42log_warn("Connection failed will retry in %d second(s)\n", connectionConfig_.Delay());
}

string UPAConsumer::getTransportName()
{
   return GetOwner()->GetTransportName();
}
