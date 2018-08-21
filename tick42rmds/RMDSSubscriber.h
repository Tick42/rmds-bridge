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
#ifndef __RMDS_SUBSCRIBER_H__
#define __RMDS_SUBSCRIBER_H__

#include "UPASourceDirectory.h"
#include "SourceDirectoryResponseListener.h"

#include "tick42rmdsbridgefunctions.h"

#include "UPALogin.h"
#include "UPAConsumer.h"

#include "UPADictionary.h"
#include "UPATransportNotifier.h"

#include "UPAMamaFieldMap.h"

#include "RMDSSources.h"
#include "DictionaryReply.h"

#include "UPASubscription.h"
#include "RMDSBridgeSubscription.h"
#include "rmdsBridgeTypes.h"
#include "transportconfig.h"
#include "utils/thread/lock.h"
#include "inbox.h"

class UPALogin;
class UPABridgePoster;


// Implements the mama subscriber

class RMDSSubscriber : public LoginResponseListener, public ConnectionListener, public SourceDirectoryResponseListener, public DictionaryResponseListener
{
public:
    RMDSSubscriber(UPATransportNotifier &notify);
    virtual ~RMDSSubscriber();

    bool Initialize(mamaBridge bridge, mamaTransport transport, const std::string &transport_name);

    bool Start(const char* interfaceName, RsslConnectionTypes connType );
    bool Stop();
    bool Done();

    // accessors
    const std::string& GetTransportName() const {return transport_name_;}
    char* InterfaceName() const {return interfaceName_;}
    RsslConnectionTypes ConnType() const {return connType_;}

    mamaQueue GetRequestQueue() const {return upaRequestQueue_;}

    const TransportConfig_ptr_t& Config() const {return config_;}

    UPAConsumer_ptr_t Consumer() const { return consumer_; }
    void Consumer(UPAConsumer_ptr_t val) { consumer_ = val; }

    // subscription & snapshots
    virtual bool AddSubscription(subscriptionBridge* subscriber, const char* source, const char* symbol, mamaTransport transport, mamaQueue queue, mamaMsgCallbacks callback, mamaSubscription subscription, void* closure);
    bool RemoveSubscription(RMDSBridgeSubscription * pSubscription);
    void SendSnapshotRequest(SnapshotReply_ptr_t snap);
    bool FindSubscription(const std::string & source, const std::string & symbol, UPASubscription_ptr_t & sub);

    // dictionary reply
    void SetDictionaryReply(boost::shared_ptr<DictionaryReply_t> dictionaryReply);

    static void loginSuccessCallBack(RsslChannel* chnl);

    // connection state
    typedef enum
    {
        unconnected = 0,
        connecting,
        loggingin,
        requestingSourceDirectory,
        requestingDictionary,
        live,
        reconnecting

    } SubscriberState_t;
    SubscriberState_t subscriberState_;

    SubscriberState_t GetState() const
    {
        return subscriberState_;
    }

    // do the stuff that needs to be done when the connection comes live
    void SetLive();

    // Sources

    RsslUInt32 GetServiceId(const std::string & sourceName) const;

    // Listener functions
    virtual void LoginResponse(UPALogin::RsslLoginResponseInfo * pResponseInfo, bool loginSucceeded, const char* extraInfo);
    virtual void ConnectionNotification(bool connected, const char* extraInfo);
    virtual void SourceDirectoryUpdate(RsslSourceDirectoryResponseInfo * pResponseInfo, bool isRefresh);
    virtual void SourceDirectoryRefreshComplete(bool succeeded);
    virtual void DictionaryUpdate(bool dictionaryComplete);

    //Dictionaries related (RMDS and UPA to MAMA)
    const UPADictionary *GetUpaDictionary() const {return consumer_ ? (consumer_->UpaDictionary()) : NULL;}
    UPADictionary *GetUpaDictionary() {return consumer_ ? (consumer_->UpaDictionary()) : NULL;}
    bool UpdateUpaMamaFieldMap();

    const UpaMamaFieldMap_ptr_t& FieldMap() const
    {
        return UpaMamaFieldMap_;
    }

    bool CreateUpaMamaFieldMap();

    // get the new item subscription for the specified source - used by interactive publishing
    bool GetNewItemSubscription(const std::string& sourceName, mamaSubscription* sub);

    // need to license the enhanced version of the bridge to accept sink inserts
    // get the Insert subscription for the specified source
    virtual bool GetInsertSubscription(const std::string& sourceName, mamaSubscription* sub);
    virtual bool SendInsertMessage(const std::string& sourceName, mamaMsg msg);


   // We need to stop updates being sent while closing down
   void PauseUpdates();
   void ResumeUpdates();

   // deal with any pending subscriptions
   void ProcessPendingSubcriptions();

protected:
    UPAConsumer_ptr_t consumer_;

private:
    static void *threadFunc(void *state);
    UPATransportNotifier notify_;
    UPALogin::RsslLoginResponseInfo responseInfo_;
    boost::shared_ptr<DictionaryReply_t> dictionaryReply_;
    bool sentDictionary_;

    // access to mama
    mamaBridge bridge_;
    mamaTransport transport_;
    std::string transport_name_;

    // connection configuration
    char* interfaceName_;
    RsslConnectionTypes connType_;

    bool recovering_;
    bool connected_;

    // upa consumer thread
    wthread_t hConsumerThread_;
    //mutable utils::thread::lock_t cs_;

    mutable utils::thread::lock_t pendingListLock_;


    mamaQueue upaRequestQueue_;

    boost::shared_ptr<RMDSSources> sources_;

    //map for "special" subscribers for publisher new item requests
    typedef std::map<std::string, mamaSubscription> PublisherRequestSubMap_t;
    PublisherRequestSubMap_t publisherRequestSubMap_;

    TransportConfig_ptr_t config_;

    UpaMamaFieldMap_ptr_t UpaMamaFieldMap_;

    // list of pending subscritpions / snapshots.  If requests are made before connection completes
    // they are added to the pending list and actioned when the connection state changes
    typedef std::list<RMDSBridgeSubscription_ptr_t> SubscriptionList_t;
    SubscriptionList_t pendingSubscriptions_;

    typedef std::list<RMDSBridgeSnapshot_ptr_t> SnapshotList_t;
    SnapshotList_t pendingSnapshots_;

    // need to keep a map of subscriptions that are created on an invalid source. Although the subscription failure sends an error code to the caller, there is nothing to prevent
    // calling unsubscribe later. This holds onto the boost pointer
    typedef std::map<RMDSBridgeSubscription *, RMDSBridgeSubscription_ptr_t> BadSourceFailuresMap_t;
    BadSourceFailuresMap_t badSourceFailures_;

    bool AddSubscriptionToSource(RMDSBridgeSubscription_ptr_t sub);
    bool AddSnaphotToSource(RMDSBridgeSnapshot_ptr_t snap);


};


class SnapshotReply
{
    //bound variables from publisher

    mamaMsg reply;
    publisherBridge publisher;
    const char* replyAddr;
    mamaInbox inbox;
    bool queuedictionary_;
    std::string sourceName_;
    std::string symbol_;


public:
    SnapshotReply ( publisherBridge mamaPublisher, const char* replyAddr, mamaInbox inbox, std::string sourceName, std::string symbol)
        : publisher(mamaPublisher)
        , replyAddr(replyAddr)
        , inbox(inbox)
        , sourceName_(sourceName)
        , symbol_(symbol)
    {
    }

    std::string Symbol() const { return symbol_; }
    void Symbol(std::string val) { symbol_ = val; }

    std::string SourceName() const { return sourceName_; }
    void SourceName(std::string val) { sourceName_ = val; }

    void OnMsg(mamaMsg msg)
    {
        rmdsMamaInbox_send( inbox,  msg );
    }

};



#endif //__RMDS_SUBSCRIBER_H__

