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

#include "UPATransportNotifier.h"
#include "rmdsBridgeTypes.h"
#include "RMDSPublisherSource.h"
#include "RMDSPublisherBase.h"
#include "UPANIProvider.h"

//
// Implements the Non-Interactive provider in the bridge

class RMDSNIPublisher: public RMDSPublisherBase, public LoginResponseListener, public ConnectionListener
{
public:
    RMDSNIPublisher(UPATransportNotifier &notify);
    virtual ~RMDSNIPublisher();

    bool Initialize(mamaBridge bridge, mamaTransport transport, const std::string &transport_name);
    bool Start();

    boost::shared_ptr<TransportConfig_t> Config(void)
    {
        return config_;
    }

    char * InterfaceName() {return interfaceName_;}
    RsslConnectionTypes ConnType() {return connType_;}

    std::string GetTransportName() {return transportName_;}
    
    // Listener functions
    virtual void LoginResponse(UPALogin::RsslLoginResponseInfo * pResponseInfo, bool loginSucceeded, const char* extraInfo);

    UPANIProvider_ptr_t NIProvider() const { return niProvider_; }

    virtual  RsslChannel * GetChannel()
    {
        return niProvider_->RsslNIProviderChannel();
    }

    // unsolicited messages for NI publishers
    virtual bool SolicitedMessages()
    {
        return false;
    }

private:
    // keep a ref to the bridge impl
    mamaBridgeImpl* bridgeImpl_;

    // source
    void InitialiseSource(const std::string source);
    boost::shared_ptr<TransportConfig_t> config_;

    // connection configuration
    char* interfaceName_;
    RsslConnectionTypes connType_;

    // login
    UPALogin::RsslLoginResponseInfo responseInfo_;
    // log the login details
    void LogResponseInfo(const UPALogin::RsslLoginResponseInfo &responseInfo);

    // listeners
    virtual void ConnectionNotification(bool connected, const char* extraInfo);

    // UPA Thread
    // upa consumer thread
    wthread_t NIProviderThread_; 

    UPANIProvider_ptr_t niProvider_;

    typedef enum
    {
        unconnected = 0,
        connecting,
        loggingin,
        sendingsourcedirectory,
        live,
        reconnecting

    } NIPublisherState_t;

    NIPublisherState_t publisherState_;

    // connection state
    bool recovering_;
    bool connected_;
};

