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

#pragma once
#include "UPATransportNotifier.h"
#include "rmdsBridgeTypes.h"
#include "RMDSPublisherSource.h"
#include "RMDSPublisherBase.h"

class RMDSPublisherSource;

//
// Implements the Non-Interactive provider in the bridge

class RMDSPublisher : public RMDSPublisherBase, public ConnectionListener
{
public:
    RMDSPublisher(UPATransportNotifier &notify);
    ~RMDSPublisher(void);

    bool Initialize(mamaBridge bridge, mamaTransport transport, const std::string &transport_name);

    virtual bool Start();
    const char *PortNumber()
    {
        return portNumber_.c_str();
    }

    typedef enum
    {
        unconnected = 0,
        connecting,
        live,
        reconnecting

    } PublisherState_t;
    
    UPAProvider_ptr_t Provider()
    {
        return provider_;
    }

    void SentLoginResponse();

    bool RequestItem(std::string source, std::string symbol, bool refresh);

    bool CloseItem(std::string source, std::string symbol);

    // get hold of the subscriber. This is where new item messages get sent to and where we get dictionaries and field map from
    RMDSSubscriber_ptr_t Subscriber() const { return subscriber_; }

    // interactive publisher doesn't really need to do this
    virtual RsslChannel *GetChannel()
    {
        return 0;
    }

    virtual bool AcceptPosts() const
    {
        return false;
    }

    virtual bool SendInsertMessage(mamaMsg msg);

protected:
    // keep a ref to the bridge impl
    mamaBridgeImpl* bridgeImpl_;

private:
    std::string portNumber_;

    // transport notification callbacks
    //UPATransportNotifier notify_;

    // state
    bool connected_;
    PublisherState_t publisherState_;

    // The transport implementing the subscriber that will be listening for newItem requests
    std::string subscriberTransportName_;

    UPAProvider_ptr_t provider_;

    // listeners
    virtual void ConnectionNotification(bool connected, const char* extraInfo);

    // UPA provider thread
    wthread_t hProviderThread_; 

    // sources
    int InitialiseSource(const std::string sourceList);

    // this is where recap messages get sent
    static void MAMACALLTYPE InboxOnMessageCB(mamaMsg msg, void *closure);
    void InboxOnMessage(mamaMsg msg);
    mamaInbox inbox_;
};

