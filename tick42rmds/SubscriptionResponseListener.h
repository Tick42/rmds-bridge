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
#ifndef __SUBSCRIPTIONRESPONSELISTENER_H__
#define __SUBSCRIPTIONRESPONSELISTENER_H__

#include "mama/quality.h"


// abstract base class for responses
class SubscriptionResponseListener
{
public:
    // send a message on the mama subscription
    virtual void OnMessage(mamaMsg msg, mamaMsgType msgType, bool async) = 0;

    // send a status message on the mama subcription
    virtual void OnStatusMessage(mamaMsgStatus statusCode) = 0;

    // raise an error on the mama subscription
    virtual void OnError(mama_status statusCode) = 0;

    // call OpenMAMA onQuality callback
    virtual void OnQuality(mamaQuality quality, short cause) = 0;
};

#endif //__SUBSCRIPTIONRESPONSELISTENER_H__

