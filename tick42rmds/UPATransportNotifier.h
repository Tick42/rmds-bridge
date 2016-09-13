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
#ifndef __UPATRANSPORTNOTIFIER_H__
#define __UPATRANSPORTNOTIFIER_H__

/** 
 * @brief Thin wrapper class over the transport callback notification  
 *
 */
class UPATransportNotifier
{
    mamaTransport transport;
public:
    UPATransportNotifier(mamaTransport  transport);
    ~UPATransportNotifier();
    // Notify the client that connection failed
    void onConnectionFailed(const char* extraInfo = "");
    // Notify the client that Connection failed
    void onLoginFailed(const char* extraInfo = "");
    // Notify the client the source directory request failed
    void onSourceDirectoryRequestFailed(const char* extraInfo = "");
    // Notify the client the source directory request succeed
    void onSourceDirectoryRequestConnect(const char* extraInfo = "");
    // Notify the dictionary request failed
    void onDictionaryRequestFailed(const char* extraInfo = "");
    // Notify the client the connection completed successfully
    void onConnectionComplete(const char* extraInfo = "");

    // Notify the client that connection disconnected
    void onConnectionDisconnect(const char* extraInfo);
    // Notify the client that connection reconnected
    void onConnectionReconnect(const char* extraInfo);

};

#endif //__UPATRANSPORTNOTIFIER_H__
