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
#ifndef __UPASUBSCRIPTION_H__
#define __UPASUBSCRIPTION_H__

#include <utils/thread/lock.h>

#include "UPAConsumer.h"
#include "UPAMamaFieldMap.h"
#include "UPABookMessage.h"

#include "RMDSBridgeSubscription.h"
#include "transportconfig.h"

class SubscriptionResponseListener;
class UPAFieldDecoder;

// manage the request / response  for a subscriptionon the UPA api
class UPASubscription : public boost::enable_shared_from_this<UPASubscription>
{
public:
    UPASubscription (const std::string&  sourceName, const std::string& symbol, bool logRmdsValues);
    virtual ~UPASubscription();

    const std::string& Symbol() const { return symbol_; }

    const std::string& SourceName() const { return sourceName_; }

    const RMDSSource_ptr_t& Source() const { return source_; }
    void Source(const RMDSSource_ptr_t& val) { source_ = val; }

    bool LogRmdsValues() const { return logRmdsValues_; }

    virtual bool Open(const UPAConsumer_ptr_t& consumer);
    virtual bool Close();
    // use this one for a standalone snapshot
    virtual bool Snapshot(const UPAConsumer_ptr_t& consumer, const RMDSBridgeSnapshot_ptr_t& snapRequest);

    // and this one to insert a refresh into an existing subscription stream
    virtual bool Refresh(const UPAConsumer_ptr_t& consumer, const RMDSBridgeSubscription_ptr_t& subRequest);

    void CompleteSnapshot();

    //  handle the incoming rssl messages from the consumer thread
    RsslRet ProcessMarketPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet ProcessMarketByOrderResponse(RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet ProcessMarketByPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter);

    // manage the state - these are set internally on item status messages from the consumer and also by the source
    // directory handler when it recieves changes to the source state
    bool SetStale(const char* msg);
    bool SetLive();

    enum UPASubscriptionState
    {
        SubscriptionStateInactive = 0,
        SubscriptionStateSubscribing,
        SubscriptionStateLive,
        SubscriptionStateStale
    };
    UPASubscriptionState GetSubscriptionState()
    {
        utils::thread::T42Lock l(&subscriptionLock_);
        return state_;
    }

    virtual bool ReSubscribe();


    // subscription type - which OMM domain to use
    enum UPASubscriptionType
    {
        SubscriptionTypeUnknown = 0,
        SubscriptionTypeMarketPrice,
        SubscriptionTypeMarketByPrice,
        SubscriptionTypeMarketByOrder

    };

    void SetDomain(UPASubscriptionType domain);

    // obtain the rssl stream id for this subscription
    RsslUInt32 StreamId() const { return streamId_; }

    // manage listeners
    void AddListener(const RMDSBridgeSubscription_ptr_t& listener);
    void RemoveListener(const RMDSBridgeSubscription_ptr_t& listener);
    void RemoveListener(RMDSBridgeSubscription* listener);
    bool FindListener(RMDSBridgeSubscription* listener, RMDSBridgeSubscription_ptr_t& listenerPtr);

    size_t ListenerCount() const
    {
        return listeners_.size();
    }

    // Request an image for this subscription
    void RequestImage(const RMDSBridgeSubscription_ptr_t& sub);

    void QueueSubscriptionDestroy(const RMDSBridgeSubscription_ptr_t& sub);

protected:

    // used by derived classes
    void NotifyListenersMessage(mamaMsg msg, mamaMsgType msgType);
    void NotifyListenersRefreshMessage(mamaMsg msg, const RMDSBridgeSubscription_ptr_t& sub, bool bookMessage = false);
    void NotifyListenersStatus(mamaMsgStatus statusCode);
    void NotifyListenersError(mama_status statusCode);
    void NotifyListenersQuality(mamaQuality quality, short cause);

    void NotifyListenersMessageSync(mamaMsg msg, mamaMsgType msgType) const;
    void NotifyListenersMessageAsync(mamaMsg msg, mamaMsgType msgType) const;

    // Protected so Marketfeed subclass can use them in diagnostics
    std::string sourceName_;
    std::string symbol_;

    // protected so derived enhanced classes can access
    UpaMamaFieldMap_ptr_t fieldmap_;
    typedef boost::shared_ptr<UPAFieldDecoder> UPAFieldDecoder_ptr_t;
    UPAFieldDecoder_ptr_t decoder_;

    UPAConsumer_ptr_t consumer_;

    // Re-usable message object
    mamaMsg msg_;
    mamaBridge bridge_;
    mamaPrice price_;
    mamaDateTime dateTime_;

    RsslRet InternalProcessMarketPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet InternalProcessMarketByOrderResponse(RsslMsg* msg, RsslDecodeIterator* dIter);
    RsslRet InternalProcessMarketByPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter);

    bool GotInitial() { return gotInitial_; }


private:
    // build and send a status message
    void SendStatusMsg(mamaMsgStatus status);

    // mama queue callback for the various requests
    static void MAMACALLTYPE SubscriptionOpenRequestCb(mamaQueue queue,void *closure);
    static void MAMACALLTYPE SubscriptionSnapshotRequestCb(mamaQueue queue,void *closure);
    static void MAMACALLTYPE SubscriptionRefreshRequestCb(mamaQueue queue,void *closure);
    static void MAMACALLTYPE SubscriptionCloseRequestCb(mamaQueue queue,void *closure);
    static void MAMACALLTYPE SubscriptionDestroyCb(mamaQueue queue,void *closure);

    // attributes
    UPASubscriptionType subscriptionType_;
    bool logRmdsValues_;

    //state - need to manage our internal representation of the state (which we use to map to mama state) and the rssl state
    // the subscription state needs to be thread safe as if it read and set on both the consumer thread and the bridge thread
    UPASubscriptionState state_;
    void SetSubscriptionState(UPASubscriptionState state)
    {
        utils::thread::T42Lock l(&subscriptionLock_);
        t42log_debug("Set subscription state to %d for %s on stream %d \n", state, symbol_.c_str(), streamId_);
        state_ = state;
    }

    // rssl state - this persists state data delivered by upa
    void LogRsslState(RsslState* state);
    bool SetRsslState(RsslState* state);
    RsslState itemState_;

    // rssl streamid
    RsslUInt32 streamId_;

    // need to keep track of whether we have had a response to the initial open request, so we can track the pending opens correctly
    bool gotInitial_;

    // keep track of opens and closes
    int openCloseCount_;

    // we hold a reference to the source object as it carries the serviceID we need to for requests to the rmds
    RMDSSource_ptr_t source_;

    // make an open request
    void QueueOpenRequest();
    bool SendOpenRequest(bool isSnapshot = false);

    // make a close request
    void QueueCloseRequest();
    bool SendCloseRequest();

    void QueueSnapRequest();
    void QueueRefreshRequest();

    // request item
    RsslRet EncodeItemRequest(RsslChannel* chnl, RsslBuffer* msgBuf, RsslInt32 streamId, bool isSnapshot = false);

    // close stream
    RsslRet CloseStream(RsslChannel* chnl, RsslInt32 streamId);
    RsslRet EncodeItemClose(RsslChannel* chnl, RsslBuffer* msgBuf, RsslInt32 streamId);


    // This is supported only in the Tick42 enhanced package - contact support@tick42.com for details
    // marketfeed
    virtual void ProcessMFeedMessage(RsslMsg* msg);
    // ansi
    virtual void ProcessAnsiMessage(RsslMsg* msg);

    // flags to ensure not uspported is only logged once
    bool reportedMFeedNotSupported_;
    bool reportedAnsiNotSupported_;


    // utilities to build the mama message
    void dumpHexBuffer(const RsslBuffer * buffer);
    mamaMsgType SetMessageType(RsslUInt8 messageClass, RsslUInt8 updateType, bool bookMessage);
    void setMsgNum(bool IsStatusMsg);
    void setMsgNumBook();
    void setLineTime();

    int msgTotal_;
    int msgNum_;
    int64_t msgSeqNum_;

    // notify listeners
    typedef std::vector<RMDSBridgeSubscription_ptr_t> SubscriptionResponseListenersVector_t;
    SubscriptionResponseListenersVector_t listeners_;

    mutable utils::thread::lock_t subscriptionLock_;

    // internal message cache for book handling
    UPABookByOrderMessage bookByOrderMessage_;
    UPABookByPriceMessage bookByPriceMessage_;

    // price point map for book handling
    PricePointMap_t PPMap_;

    char ExtractSideCode(RsslBuffer &mapKey);


    // handle snapshots
    bool isSnapshot_;
    bool isRefresh_;
    RMDSBridgeSnapshot_ptr_t snapShot_;
    RMDSBridgeSubscription_ptr_t subscription_;

    // For dup symbol send recap to all or just new subscriber
    bool sendRecap_;
    bool useCallbacks_;
    bool sendAckMessages_;
    bool asyncMessaging_;

    // report rssl message decode failure
    // we dont know if this happens often so report first and then throttle reporting to once per minute
    RsslUInt64 numDecodeFailures_;
    RsslUInt64 numDecodeFailuresLast_;
    RsslUInt32 timeLastReport_;
    void ReportDecodeFailure(const char * location, RsslRet errCode);

    // wrap a upa subscription shared pointer for use as a mama void * closure;
    // If we use a pointer to one of these objects as a mama closure then shared pointer (and the underlying object
    // is guaranteed to stay alive while the object is on the queue
    class UPASubscriptionClosure
    {
    public:
        UPASubscriptionClosure(const UPASubscription_ptr_t& s)
            :sub_(s)
        {}

        const UPASubscription_ptr_t& GetPtr() const
        {
            return sub_;
        }
    private:
        UPASubscription_ptr_t sub_;
    };

    // wrap a rmds bridge subscription shared pointer for use as a mama void * closure;
    // If we use a pointer to one of these objects as a mama closure then shared pointer (and the underlying object
    // is guaranteed to stay alive while the object is on the queue
    public:
    class RMDSBridgeSubscriptionClosure
    {
    public:
        RMDSBridgeSubscriptionClosure(const RMDSBridgeSubscription_ptr_t& s)
            :sub_(s)
        {}

        const RMDSBridgeSubscription_ptr_t& GetPtr() const
        {
            return sub_;
        }

    private:
        RMDSBridgeSubscription_ptr_t sub_;
    };

};


#endif //__UPASUBSCRIPTION_H__
