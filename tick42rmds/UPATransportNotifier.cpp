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
#include "UPATransportNotifier.h"
#include <utils/t42log.h>

UPATransportNotifier::UPATransportNotifier( mamaTransport transport )
    : transport(transport)
{

}


// Notify the client that connection failed
void UPATransportNotifier::onConnectionFailed(const char* extraInfo)
{
    t42log_error("Connection failed: %s \n", extraInfo);
    // TODO This change is to work around a bug in OpenMAMA C++ code where the MAMA_TRANSPORT_CONNECT_FAILED code is missing
    //   from the MamaTransportCallback class.
    // mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_CONNECT_FAILED, 0, extraInfo.c_str());
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_DISCONNECT, 0, extraInfo);
}


// Notify the client that connection disconnected
void UPATransportNotifier::onConnectionDisconnect(const char* extraInfo)
{

    t42log_error("Connection disconnected: %s \n", extraInfo);
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_DISCONNECT, 0, extraInfo);
}


// Notify the client that connection reconnected
void UPATransportNotifier::onConnectionReconnect(const char* extraInfo)
{

    t42log_info("Connection recconnected: %s \n", extraInfo);
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_RECONNECT, 0, extraInfo);
}



// Notify the client that Connection failed
void UPATransportNotifier::onLoginFailed(const char* extraInfo)
{

    t42log_error("login failed: %s \n", extraInfo);
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_CONNECT_FAILED, 0, extraInfo);
}


// Notify the client the source directory request failed
void UPATransportNotifier::onSourceDirectoryRequestFailed(const char* extraInfo)
{

    t42log_error("Source directory Request failed: %s \n", extraInfo);
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_NAMING_SERVICE_DISCONNECT, 0, extraInfo);
}

// Notify the client the source directory request succeed
void UPATransportNotifier::onSourceDirectoryRequestConnect(const char* extraInfo)
{

    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_NAMING_SERVICE_CONNECT, 0, extraInfo);
}

// Notify the dictionary request failed
void UPATransportNotifier::onDictionaryRequestFailed(const char* extraInfo)
{
    const char* default_message = "Dictionary request failed!";
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_CONNECT_FAILED, 0, strlen(extraInfo) == 0 ? default_message : extraInfo);
}


// Notify the client the connection completed successfully
void UPATransportNotifier::onConnectionComplete(const char* extraInfo)
{
    mamaTransportImpl_invokeTransportCallback(transport, MAMA_TRANSPORT_CONNECT, 0, extraInfo);
    
}
