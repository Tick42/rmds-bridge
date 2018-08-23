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


#include "UPAMessage.h"
#include "RMDSConnectionConfig.h"
#include "ConnectionListener.h"

class RMDSNIPublisher;
class UPALogin;

// Implements a provider thread for non-interactive poublishing - mounts to an ADH
class UPANIProvider
{
public:
    UPANIProvider(RMDSNIPublisher *);
    virtual ~UPANIProvider(void);


    void Run();

    bool IsConnectionConfigValid();

    // connection notifications
    void AddListener( ConnectionListener * pListener );

    static void MAMACALLTYPE LoginRequestCb(mamaQueue queue,void *closure);
    bool RequestLogin(mamaQueue requestQueue);

    static void MAMACALLTYPE SendSourceDirectoryCb(mamaQueue queue,void *closure);
    bool SendSourceDirectory(mamaQueue requestQueue);

    RsslChannel * RsslNIProviderChannel() const { return rsslNIProviderChannel_; }

private:
    RMDSNIPublisher * owner_;

    // sockets
    // rssl connection
    fd_set    readfds_;
    fd_set    exceptfds_;
    fd_set    wrtfds_;

    // manage connection
    RsslChannel* ConnectToRsslServer(const std::string &hostname, const std::string &port, const char* interfaceName, RsslConnectionTypes connType, RsslError* error);

    void RecoverConnection();
    void RemoveChannel(RsslChannel* chnl);

    RMDSConnectionConfig connectionConfig_;
    RsslBool shouldRecoverConnection_;
    RsslBool receivedServerMsg_;
    void WaitReconnectionDelay();
    void LogReconnection();

    void InitPingHandler(RsslChannel* chnl);

    RsslUInt32 pingTimeoutServer_;
    RsslUInt32 pingTimeoutClient_;
    time_t nextReceivePingTime_;
    time_t nextSendPingTime_;

    RsslChannel *rsslNIProviderChannel_;

    RsslRet ReadFromChannel(RsslChannel* chnl);
    void ProcessPings(RsslChannel* chnl);

    RsslRet ProcessResponse(RsslChannel* chnl, RsslBuffer* buffer);

    // notify listeners
    std::vector<ConnectionListener*> listeners_;
    void NotifyListeners(bool connected, const char* extraInfo);

    // keep the thread spinning
    bool runThread_;

    RsslBool isInLoginSuspectState_;

    char* interfaceName_;
    RsslConnectionTypes connType_;

    mamaQueue requestQueue_;

    UPALogin * login_;

    // manage reconnection


    // pump incoming events from mama queue
    void PumpQueueEvents();

    void ExitThread();
};


