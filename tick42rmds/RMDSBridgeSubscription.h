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
#ifndef __RMDS_BRIDGESUBSCRIPTION_H__
#define __RMDS_BRIDGESUBSCRIPTION_H__

#include "UPASubscription.h"
#include "SubscriptionResponseListener.h"
#include "utils/thread/lock.h"

class SubscriptionResponseListener;

// The RMDSBridgeSubscription maps the mama subscription object onto an underlying UPA subscription. Mama subscriptions to the same source+symbol are mapped onto the same 
// underlying platform subscription and the updates, status etc multiplexed through the subscription response listener interface methods

class RMDSBridgeSubscription : public SubscriptionResponseListener
{
public:
    RMDSBridgeSubscription(void);

    RMDSBridgeSubscription(const std::string&  sourceName, const std::string& symbol, mamaTransport transport, mamaQueue queue, mamaMsgCallbacks callback, 
        mamaSubscription subscription, void* closure, bool logRmdsValues);

    virtual ~RMDSBridgeSubscription(void);

    void Shutdown();

    // accessors
    std::string SourceName() const { return sourceName_; }
    mamaMsgCallbacks Callback() const { return callback_; }
    mamaSubscription Subscription() const { return subscription_; }
    void* Closure() const { return closure_; }
    std::string Symbol() const { return symbol_; }
    bool LogRmdsValues() const { return logRmdsValues_; }
    mamaTransport Transport() const { return transport_; }

    RMDSSource_ptr_t Source() const { return source_; }
    void Source(RMDSSource_ptr_t val) { source_ = val; }

    UPASubscription_ptr_t UpaSubscription() const { return UpaSubscription_; }
    void UpaSubscription(UPASubscription_ptr_t val) { UpaSubscription_ = val; }

    // mnethods
    void SendStatusMessage(mamaMsgStatus status);

    bool Open(UPAConsumer_ptr_t consumer);
    mamaQueue Queue() const { return queue_; }



    //
    // SubscriptionResponseListener implementation
    //
    // send a message on the mama subscription
    virtual void OnMessage(mamaMsg msg, mamaMsgType msgType);

    // raise an error on the mama subscription
    virtual void OnError(mama_status statusCode);

    // send a status message on the mama subcription
    virtual void OnStatusMessage(mamaMsgStatus statusCode);

    // call OpenMAMA onQuality callback
    virtual void OnQuality(mamaQuality quality, short cause);

    static void MAMACALLTYPE SubscriptionDestroyCb(mamaQueue queue,void *closure);


private:
    mamaTransport      transport_;
    mamaQueue         queue_;
    mamaMsgCallbacks  callback_;
    mamaSubscription  subscription_;
    void*              closure_;
    bool              logRmdsValues_;

    std::string sourceName_;
    RMDSSource_ptr_t source_;

    std::string symbol_;

    // the underlying platform subscription
    UPASubscription_ptr_t UpaSubscription_;

    // need to know to drop updates if no image
    bool gotImage_;

    // block updates when set
    bool isShutdown_;

};


// The RMDSBridgeSnapshot maps the mama subscription object onto an underlying UPA subscription. Unlike the RMDSBridgeSubscription it does not multiples onto the underlying platform subscription
// Thats doesn't really make any sense for a snapshot. Also unlike a subscription the message carrying the data is sent to an inbox rather that through the OnMsg callback
class RMDSBridgeSnapshot 
{
public:
    RMDSBridgeSnapshot(SnapshotReply_ptr_t snap, bool logRMDSValues);
    ~RMDSBridgeSnapshot()
    {
    }

    std::string SourceName() const;
    std::string Symbol() const;

    UPASubscription_ptr_t Subscription() const { return subscription_; }
    void Subscription(UPASubscription_ptr_t val) { subscription_ = val; }

    bool LogRMDSValues() const { return logRMDSValues_; }

public:
    // send a message on the mama subscription
    virtual void OnMessage(mamaMsg msg);

    // raise an error on the mama subscription
    virtual void OnError(mama_status statusCode);

    // send a status message on the mama subscription
    virtual void OnStatusMessage(mamaMsgStatus statusCode);

private:

    SnapshotReply_ptr_t reply_;

    //enable logging of incoming data values
    bool logRMDSValues_;

    UPASubscription_ptr_t subscription_;
};



#endif //__RMDS_BRIDGESUBSCRIPTION_H__

