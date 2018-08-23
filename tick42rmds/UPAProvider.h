

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

#pragma once

#include <vector>

#include "rmdsBridgeTypes.h"
#include "UPALogin.h"
#include "ConnectionListener.h"

class RMDSPublisher;

// Implements thread for Interactive provider - accepts a sokect connection from the ADH
//
// implements responses to login and source directory requests

class UPAProvider
{
public:
    UPAProvider(RMDSPublisher * owner);
    virtual ~UPAProvider();

    void Run();

    void Shutdown();


    mamaQueue RequestQueue() const { return requestQueue_; }

    //RsslChannel * RsslProviderChannel() const { return rsslConsumerChannel_; }

    // connection notifications
    void AddListener( ConnectionListener * pListener );

protected:
    virtual RsslRet ProcessPost(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet SendAck(RsslChannel *chnl, RsslPostMsg *postMsg, RsslUInt8 nakCode, char *errText);

    UPALogin login_;


    // We need to keep a record of channel / streams ids to items so that we can handle close requests (these contain no identity)

    // keep this separate from the UPAClientConnection_t structure because it really is unrelated;
    // We can reasonably use a nested map here because this is only accessed on new item and item close events, which are relatively infrequent
    //
    // these are also now used by on-stream posts so may want to consider a more efficient lookup
    typedef utils::collection::unordered_map<RsslUInt32, UPAPublisherItem_ptr_t> ChannelStreamIdMap_t;
    typedef struct
    {
        RsslChannel * chnl_;
        ChannelStreamIdMap_t streamIdMap_;
    } ChannelDictionaryItem_t;

    typedef utils::collection::unordered_map<RsslChannel *, ChannelDictionaryItem_t *> ChannelDictionary_t;

    ChannelDictionary_t channelDictionary_;

    RMDSPublisher * owner_;

private:
        // rssl connection
    fd_set    readfds_;
    fd_set    exceptfds_;
    fd_set    wrtfds_;


    // Socket connection
    // Rssl structure that is bound to the listen socket
    RsslServer * rsslServer_;

    // the listen port
    char *portno_;
    bool Bind(char* portno, RsslError* error);

    bool runThread_;

    // notify listeners
    std::vector<ConnectionListener*> listeners_;
    void NotifyListeners(bool connected, const char* extraInfo);

    // connection management
    //
    // client connection information - required per connection
    typedef struct
    {
        RsslChannel *clientChannel;
        time_t nextReceivePingTime;
        time_t nextSendPingTime;
        RsslBool receivedClientMsg;
        RsslBool pingsInitialized;
    } UPAClientConnection_t;

    UPAClientConnection_t * clientConnections_;

    // clear the connection struct
    void ResetClientConnection(UPAClientConnection_t * connection);

    // Close the client connection
    void ShutdownClientConnection(UPAClientConnection_t * connection);

    // close the rssl channel
    void ShutdownChannel(RsslChannel * chnl);

    // remove ths connection on this channel
    void RemoveChannelConnection(RsslChannel * chnl);




    // handle incoming on the read socket
    void ReadFromChannel(RsslChannel* chnl);
    void CreateNewClientConnection();

    RsslRet ProcessRequest(RsslChannel* chnl, RsslBuffer* buffer);

    RsslRet ProcessLoginRequest(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet ProcessSourceDirectoryRequest(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet ProcessDictionaryRequest(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);

    RsslRet ProcessItemRequest(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);



    // ping management
    void InitChannelPingTimes(UPAClientConnection_t * clientConnection);

    // send and received pings
    uint32_t lastPingCheck_;
    void HandlePings();

    RsslRet sendPing(RsslChannel* chnl);

    void SetChannelReceivedClientMsg(RsslChannel *chnl);

    // queue for dispatching requests onto provider thread
    mamaQueue requestQueue_;




    bool acceptsPosts_;

    RsslRet EncodeAck(RsslChannel* chnl, RsslBuffer* ackBuf, RsslPostMsg *postMsg, RsslUInt8 nakCode, char *text);

};

