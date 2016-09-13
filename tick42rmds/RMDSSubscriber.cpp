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

#include "tick42rmdsbridgefunctions.h"
#include "RMDSBridgeSubscription.h"
#include "UPABridgePoster.h"
#include "RMDSSubscriber.h"
#include "UPASubscription.h"
#include "UPAConsumer.h"
#include "UPALogin.h"

#include "transportconfig.h"
#include <utils/thread/interlockedInt.h>
#include <utils/os.h>
#include <utils/t42log.h>
#include <utils/threadMonitor.h>

extern "C"
{
   fd_set    wrtfds;
}

RMDSSubscriber::RMDSSubscriber(UPATransportNotifier &notify)
   : notify_(notify)
   , interfaceName_(NULL), hConsumerThread_(0)

{
   sources_ = boost::make_shared<RMDSSources>();

   connected_ = false;
   recovering_ = false;
   sentDictionary_ = false;

   subscriberState_ = unconnected;
}

RMDSSubscriber::~RMDSSubscriber()
{
   if (interfaceName_)
      free(interfaceName_);
}

bool RMDSSubscriber::Initialize( mamaBridge bridgeImpl, mamaTransport transport, const std::string &transport_name )
{
   bridge_ = bridgeImpl;
   transport_ = transport;
   transport_name_ = transport_name;
   config_ = boost::make_shared<TransportConfig_t>(this->transport_name_);

   CreateUpaMamaFieldMap();

   mama_status status = MAMA_STATUS_OK;

   t42log_debug( "RMDSSubscriber::Initialize(): Entering.");

   // create a queue
   if (MAMA_STATUS_OK !=
      (status =  mamaQueue_create (&upaRequestQueue_, bridgeImpl)))
   {
      mama_log (MAMA_LOG_LEVEL_ERROR, "RMDSSubscriber::Initialize:"
         "Failed to create upa command queue.");
      return false;
   }

   mamaQueue_setQueueName(upaRequestQueue_, "UPA_SUBSCRIBER_QUEUE");

   return true;
}

void *RMDSSubscriber::threadFunc(void *state)
{
   // The thread monitor outputs debug when the thread starts and stops
   utils::os::ThreadMonitor mon("RMDSSubscriber-UPAConsumer");

   RMDSSubscriber *pOwner = (RMDSSubscriber *) state;
   pOwner->Consumer()->Run();
   return 0;
}

bool RMDSSubscriber::Start(const char* interfaceName, RsslConnectionTypes connType )
{
   if (consumer_)
   {
      // We've already started
      return true;
   }

   if (interfaceName)
   {
      interfaceName_ = ::strdup(interfaceName);
   }
   else
   {
      interfaceName_ = NULL;
   }

   // initialise from config
   connType_ = connType;
 
   consumer_ = UPAConsumer_ptr_t(new UPAConsumer(this));
   if (consumer_->IsConnectionConfigValid() || ! consumer_->RequiresConnection())
   {
      consumer_->AddListener(this);
      sources_->Initialise(consumer_);

      // and fire up the thread
      bool result = (0 == wthread_create(&hConsumerThread_, 0, threadFunc, this));
      if (result)
      {
         subscriberState_ = connecting;
      }
      t42log_info("RMDSSubscriber start Consumer thread %p", hConsumerThread_);
      return result;
   }
   else
   {
      subscriberState_ = unconnected;
      t42log_error("Error! Connection configuration is invalid please check properties: hosts and retrysched.");
      notify_.onConnectionFailed("Connection Failed. Invalid Connection Configuration");
   }
   return false;
}

bool RMDSSubscriber::Stop()
{
   // Stop the consumer thread
   t42log_info("RMDSSubscriber stop Consumer thread %p", hConsumerThread_);
   if (0 != hConsumerThread_)
   {
      consumer_->JoinThread(hConsumerThread_);
      hConsumerThread_ = 0;
   }

   if (sources_)
   {
      sources_->Shutdown();
   }

   subscriberState_ = unconnected;
   return true;
}

// Will create string representation of the important part of RsslLoginResponseInfo
void LogResponseInfo(const UPALogin::RsslLoginResponseInfo &responseInfo)
{
   t42log_debug("Got Login Response:" );
   t42log_debug("\tStreamId [%u]", responseInfo.StreamId);
   t42log_debug("\tUsername [%s]", responseInfo.Username);
   t42log_debug("\tApplicationId [%s]", responseInfo.ApplicationId);
   t42log_debug("\tApplicationName [%s]", responseInfo.ApplicationName);
   t42log_debug("\tPosition [%s]", responseInfo.Position);
   t42log_debug("\tProvidePermissionProfile [%ld]", responseInfo.ProvidePermissionProfile);
   t42log_debug("\tProvidePermissionExpressions [%ld]", responseInfo.ProvidePermissionExpressions);
   t42log_debug("\tSingleOpen [%ld]",responseInfo.SingleOpen);
   t42log_debug("\tAllowSuspectData [%ld]", responseInfo.AllowSuspectData);
   t42log_debug("\tSupportPauseResume [%ld]", responseInfo.SupportPauseResume);
   t42log_debug("\tSupportOptimizedPauseResume [%ld]", responseInfo.SupportOptimizedPauseResume);
   t42log_debug("\tSupportOMMPost [%ld]", responseInfo.SupportOMMPost);
   t42log_debug("\tSupportViewRequests [%ld]", responseInfo.SupportViewRequests);
   t42log_debug("\tSupportBatchRequests [%ld]", responseInfo.SupportBatchRequests);
   t42log_debug("\tSupportStandby [%ld]", responseInfo.SupportStandby);
   t42log_debug("\tisSolicited [%s]", responseInfo.isSolicited ? "true" : "false");
}

void RMDSSubscriber::LoginResponse(UPALogin::RsslLoginResponseInfo * pResponseInfo, bool loginSucceeded, const char* extraInfo)
{
   // Copy current login state
   memcpy(&responseInfo_, pResponseInfo, sizeof(responseInfo_));

   LogResponseInfo(*pResponseInfo);

   if (connected_ && loginSucceeded)
   {
      t42log_debug(extraInfo);

      subscriberState_ = requestingSourceDirectory;

      consumer_->RequestSourceDirectory(upaRequestQueue_);
   }
   else
   {
      notify_.onConnectionFailed(extraInfo);
      subscriberState_ = unconnected;
   }
}


void RMDSSubscriber::ConnectionNotification(bool connected, const char* extraInfo)
{
   t42log_debug("Connected = %s", connected ? "true" : "false");

   bool wasConnected = connected_;
   connected_ = connected;

   if (connected_)
   {

      if (!consumer_->RequestLogin(upaRequestQueue_))
      {
         subscriberState_ = unconnected;
         notify_.onLoginFailed(extraInfo);
      }
      else
      {
         subscriberState_ = loggingin;
      }
   }
   else
   {
      if (wasConnected)
      {
         subscriberState_ = connecting;
         recovering_ = true;
         notify_.onConnectionDisconnect("disconnected from RMDS");
         sources_->SetAllStale();
      }
      else
      {
         subscriberState_ = unconnected;
         notify_.onConnectionFailed(extraInfo);
      }

   }
}

void RMDSSubscriber::SourceDirectoryUpdate( RsslSourceDirectoryResponseInfo * pResponseInfo, bool isRefresh )
{

   const char * state = (int)pResponseInfo->ServiceStateInfo.ServiceState == 0 ? "DOWN" : "UP";
   const char * acceptingRequests = (int)pResponseInfo->ServiceStateInfo.AcceptingRequests == 0 ? "FALSE" : "TRUE";
   int sid = (int)pResponseInfo->ServiceId;


    t42log_info("Received SourceInfo for %s, service ID is %d, state = %s, accepting requests = %s\n" ,
        pResponseInfo->ServiceGeneralInfo.ServiceName, sid, state, acceptingRequests);

    // The update to the source will propagate any state changes through to all the subscriptions active on the source
    sources_->UpdateOrCreate(
        pResponseInfo->ServiceId,
        pResponseInfo->ServiceGeneralInfo.ServiceName,
        ServiceState(
        pResponseInfo->ServiceStateInfo.ServiceState != 0,
        pResponseInfo->ServiceStateInfo.AcceptingRequests != 0 ) );

}


void RMDSSubscriber::SourceDirectoryRefreshComplete(bool succeeded)
{
   t42log_info( "Source directory response complete");

   // this is where we make the dictionary request

   if (succeeded)
   {
      subscriberState_ = requestingDictionary;
      consumer_->RequestDictionary(upaRequestQueue_);
   }
   else
   {
      subscriberState_ = unconnected;
      notify_.onSourceDirectoryRequestFailed("Failed to process source directory");
   }


}

bool RMDSSubscriber::CreateUpaMamaFieldMap()
{
   bool result = false;

   if (config_->exists("fidsoffset"))
   {
      boost::shared_ptr<UpaMamaFieldMapHandler_t> upaMamaFieldMapTmp(
         new UpaMamaFieldMapHandler_t(
         config_->getString("fieldmap", Default_Fieldmap),
         config_->getUint16("fidsoffset", Default_FieldOffset),
         config_->getBool("unmapdfld", Default_PassUnmappedFields),
         config_->getString("mama_dict")
         ));
      UpaMamaFieldMap_ = upaMamaFieldMapTmp;
   }
   else
   {
      boost::shared_ptr<UpaMamaFieldMapHandler_t> upaMamaFieldMapTmp (
         new UpaMamaFieldMapHandler_t(
         config_->getString("fieldmap", Default_Fieldmap),
         config_->getBool("unmapdfld", Default_PassUnmappedFields),
         config_->getString("mama_dict")
         ));
      UpaMamaFieldMap_ = upaMamaFieldMapTmp;
   }


   result = UpaMamaFieldMap_ ? true : false;

   if (!result)
      t42log_warn( "Could not create field map!"); //<- should never come here!
   return result;
};

bool RMDSSubscriber::UpdateUpaMamaFieldMap()
{
   bool result = false;

   UPADictionary *upaDictionary = GetUpaDictionary();
   if ( upaDictionary)
   {
      UPADictionaryWrapper_ptr_t underlayingRsslDictionary = (upaDictionary->GetUnderlyingDictionary());
      if (underlayingRsslDictionary)
      {
         result = UpaMamaFieldMap_->SetUPADictionaryHandler(underlayingRsslDictionary);
      }
   }


   if (!result)
      t42log_warn( "Could not update field map because RMDS dictionary is not initialized!"); //<- should never come here!

   return result;
}

void RMDSSubscriber::DictionaryUpdate(bool dictionaryComplete)
{
#ifdef _WIN32
   unsigned int tid =  GetCurrentThreadId() ;
#else
   unsigned int tid = pthread_self();
#endif
   void * pThis = this;
   t42log_info("received dictionary update on subscriber 0x%0x on transport %s thread %d\n", pThis, transport_name_.c_str(), tid);

   if (dictionaryComplete)
   {
      // now have a complete rmds dictionary so can build the field map
      UpdateUpaMamaFieldMap();

      t42log_debug("update mama field map on transport %s, sentDictionary = %d, dictionaryReply = 0x%x\n", transport_name_.c_str(), sentDictionary_, dictionaryReply_.get());
      if (!sentDictionary_ && dictionaryReply_)
      {
         dictionaryReply_->Send();
         t42log_debug("Sent dictionary on transport %s\n", transport_name_.c_str());
         sentDictionary_ = true;
      }
      else
      {
          // we need to combine the dictionaries anyway
          UpaMamaFieldMap_->GetCombinedMamaDictionary();
      }
      SetLive();

      // if the feed is recovering, receipt of the dictionary completes the reconnection, so resubscribe to everything
      if (recovering_)
      {
         recovering_ = false;
         sources_->ResubscribeAll();
         notify_.onConnectionReconnect("connection recovered");
      }
      else
      {
         notify_.onConnectionComplete("Connection Live");
      }
   }
   else
   {
      subscriberState_ = unconnected;
      notify_.onDictionaryRequestFailed("Failed to process data dictionary");
   }
}

bool RMDSSubscriber::AddSubscription( subscriptionBridge* subscriber, const char* source, const char* symbol, mamaTransport transport, mamaQueue queue, mamaMsgCallbacks callback, mamaSubscription subscription, void* closure )
{
   bool logrmdsvalues = config_->getBool("logrmdsvalues");

   // trim the source name
   if (source == NULL) source = "";
   string strSource(source);
   boost::algorithm::trim(strSource);

   // if the subscription has no symbol it will be a "special" internal one
   if (symbol == 0 || ::strlen(symbol) == 0)
   {
      // see what we have after we have stripped the topic root (something like "_MD") from the source
      // for the moment we can just add it to a map of publisher new item subscriptions
      if ( publisherRequestSubMap_.find(source) == publisherRequestSubMap_.end())
      {
         // just add this subscription to the map
         publisherRequestSubMap_[source] = subscription;
         return true;
      }
      else
      {
         t42log_warn("Attempt to add duplicate newItem subscription for source %s\n", source);
         return false;
      }


   }

   //now trim the symbol
   string strSymbol(symbol);
   boost::algorithm::trim(strSymbol);


   RMDSBridgeSubscription_ptr_t sub = RMDSBridgeSubscription_ptr_t(
      new RMDSBridgeSubscription(strSource, strSymbol, transport, queue, callback, subscription, closure, logrmdsvalues));

   *subscriber = (subscriptionBridge) sub.get();

   // Add the subscription to the pending queue. This will allow the initialization to complete asynchronously.
   // The open mama subscription creation model requires the subscription to complete without error so we defer the parts that can fail
   // (invalid source etc)
   {
       // take a lock on the list while we add
       utils::thread::T42Lock l(&pendingListLock_);
       pendingSubscriptions_.push_back(sub);
   }



   return true;
}



void RMDSSubscriber::SetLive()
{
   subscriberState_ = live;

}



void RMDSSubscriber::ProcessPendingSubcriptions()
{
    if(subscriberState_ != live)
    {
        // cant dop anything
        return;
    }

    // process pending subscriptions

    utils::thread::T42Lock l(&pendingListLock_);
    while(pendingSubscriptions_.size() !=0)
    {
        RMDSBridgeSubscription_ptr_t sub = pendingSubscriptions_.front();
        pendingSubscriptions_.pop_front();
        // release the lock as this may call back into mama (if there is an error
        pendingListLock_.unlock();
        try
        {
            AddSubscriptionToSource(sub);
        }
        catch (...)
        {
            t42log_warn("caught exception processing pending subscription\n");
        }
        pendingListLock_.lock();
    }



    // still holding the lock at this point
    // process pending snapshots
    while(pendingSnapshots_.size() !=0)
    {
        RMDSBridgeSnapshot_ptr_t snap = pendingSnapshots_.front();
        pendingSnapshots_.pop_front();
        // release the lock as this may call back into mama (if there is an error
        pendingListLock_.unlock();
        try
        {
            AddSnaphotToSource(snap);
        }
        catch (...)
        {
            t42log_warn("caught exception processing pending snapshot\n");
        }
        pendingListLock_.lock();
    }

}




bool RMDSSubscriber::AddSubscriptionToSource( RMDSBridgeSubscription_ptr_t sub )
{
   //add the sub to the source

   // find whether we already have a UPA subscription for this symbol/source, if we do then just added it, otherwise we have to create a new UPASubscription

   RMDSSource_ptr_t src;
   if (!sources_->Find(sub->SourceName(), src))
   {
      //notify invalid source
      t42log_info("Unknown source name '%s' for symbol '%s' \n", sub->SourceName().c_str(), sub->Symbol().c_str());
      sub->Callback().onError(sub->Subscription(), MAMA_STATUS_BAD_SYMBOL, 0, 0, sub->Closure());

      badSourceFailures_.insert(BadSourceFailuresMap_t::value_type(sub.get(), sub));
      return false;
   }

   // Link the RMDSSubscription to the source, so that it can check for updates being paused
   sub->Source(src);

   UPASubscription_ptr_t upaSub;

   if (src->FindSubscription(sub->Symbol(), upaSub))
   {
      // we have a subscription for this already so just add to the listeners
      upaSub->AddListener(sub);
      sub->UpaSubscription(upaSub);

      // if the subscription is live then trigger an image
      // There are 2 options (at least) - Normally this function will be called from the client thread (unless the second or subsequent subscription request
      // is made during the startup or recovery and the request is made from the pending queue)
      //
      // If we take the simple approach and handle the image request on the client thread then we impose on the client the need to handle images being delivered on a different
      // thread to the updates. We also need to put a lock on the message payload in order to serialize dispatching.
      //
      // The alternative is to dispatch the image request onto the consumer thread and handle it there. This potentially introduces a small latency in the image (although
      // probably less than the latency of the request to rmds for the first image) and adds the complication of having to dispatch.
      //
      // In this version we are going to take the second approach and move the image request onto the consumer thread.

      if (upaSub->GetSubscriptionState() == UPASubscription::SubscriptionStateLive)
      {
         // request an initial image now
         upaSub->RequestImage(sub);
      }
      // otherwise the image will appear in due course as the subscription is on the listeners list
   }
   else
   {
      // create a new upaSubscription
      upaSub = src->CreateSubscription(sub->Symbol(), sub->LogRmdsValues());
      if (upaSub == 0)
      {
         return false;
      }
      src->AddSubscription(upaSub);
      upaSub->SetDomain(src->SourceDomain());
      upaSub->Source(src);
      upaSub->AddListener(sub);

      // and open it
      upaSub->Open(consumer_);
   }


   return true;

}




bool RMDSSubscriber::RemoveSubscription( RMDSBridgeSubscription * pSubscription )
{
   RMDSSource_ptr_t src;
   if (!sources_->Find(pSubscription->SourceName(), src))
   {
       // The source for this subscription is not in the sources map so assume it was created on a bad source
       BadSourceFailuresMap_t::iterator it = badSourceFailures_.find(pSubscription);
       if(it != badSourceFailures_.end())
       {
           badSourceFailures_.erase(it);
       }
       else
       {
          t42log_warn( "RMDSSubscriber::RemoveSubscription - Unable to remove subscription for %s:%s - no such source", pSubscription->SourceName().c_str(), pSubscription->Symbol().c_str());
       }

      return false;
   }


   //  get the UPA subscription from the source remove the bridge subscription as a listener then, only if the listener count is zero
   // remove the subscription from the source

   UPASubscription_ptr_t sub;

   if (!src->FindSubscription(pSubscription->Symbol(), sub))
   {
      t42log_warn( "RMDSSubscriber::RemoveSubscription - Unable to remove subscription for %s:%s - no such symbol in source", pSubscription->SourceName().c_str(), pSubscription->Symbol().c_str());
      return false;
   }

   string symbol = pSubscription->Symbol();

   sub->RemoveListener(pSubscription);
   if (sub->ListenerCount() == 0)
   {
      src->RemoveSubscription(symbol);
   }

   return true;


}


void RMDSSubscriber::SetDictionaryReply(boost::shared_ptr<DictionaryReply_t> dictionaryReply)
{

   void * pThis = this;
   void * dictReply = dictionaryReply.get();

   dictionaryReply_ = dictionaryReply;
   consumer_->ClientRequestDictionary(upaRequestQueue_);

}



bool RMDSSubscriber::FindSubscription( const string & source, const string & symbol, UPASubscription_ptr_t & sub )
{
   RMDSSource_ptr_t src;
   if (!sources_->Find(source, src))
   {

      t42log_warn("FindSubscription - unknown source %s \n", source.c_str());
      return false;
   }


   if (!src->FindSubscription(symbol, sub))
   {
      t42log_debug("FindSubscription - no subscription source %s : symbol %s \n", source.c_str(), symbol.c_str());
      return false;
   }

   return true;
}


RsslUInt32 RMDSSubscriber::GetServiceId(const string & sourceName) const
{
   RsslUInt32 ret = 0;

   // look up the source from the name and returnm its service id

   RMDSSource_ptr_t src;
   if (!sources_->Find(sourceName, src))
   {

      t42log_warn("GetServiceId - unknown source %s \n", sourceName.c_str());
      return 0;
   }

   return (RsslUInt32)src->ServiceId();

}

// get the new item subscription for the specified source
bool RMDSSubscriber::GetNewItemSubscription(const std::string & sourceName, mamaSubscription * sub)
{
   std::string sourceFullName = "_MD." + sourceName;
   PublisherRequestSubMap_t::iterator it = publisherRequestSubMap_.find(sourceFullName);
   if ( it == publisherRequestSubMap_.end())
   {
      t42log_warn("failed to find new item subscription for source %s\n", sourceFullName.c_str());
      return false;
   }
   else
   {
      *sub = it->second;
   }

   return true;
}

// get the insert subscription for the specified source
bool RMDSSubscriber::GetInsertSubscription(const std::string & sourceName, mamaSubscription * sub)
{
    t42log_warn("The enhanced version of the Tick42 RMDS bridge is required to accept posted messages");

    return false;
}

//////////////////////////////////////////////////////////////////////////
//
void RMDSSubscriber::PauseUpdates()
{
   if (0 != sources_)
   {
      sources_->PauseUpdates();
   }
}

//////////////////////////////////////////////////////////////////////////
//
void RMDSSubscriber::ResumeUpdates()
{
   if (0 != sources_)
   {
      sources_->ResumeUpdates();
   }
}

void RMDSSubscriber::SendSnapshotRequest( SnapshotReply_ptr_t snapReply )
{
    bool logrmdsvalues = config_->getBool("logrmdsvalues");



    RMDSBridgeSnapshot_ptr_t snap = RMDSBridgeSnapshot_ptr_t(
        new RMDSBridgeSnapshot(snapReply, logrmdsvalues));


    // Add the subscription to the pending queue. This will allow the initialization to complete asynchronously.
    // The open mama subscription creation model requires the subscription to complete without error so we defer the parts that can fail
    // (invalid source etc)
    {
        // take a lock on the list while we add
        utils::thread::T42Lock l(&pendingListLock_);
        pendingSnapshots_.push_back(snap);
    }


}

bool RMDSSubscriber::AddSnaphotToSource( RMDSBridgeSnapshot_ptr_t snap )
{
    //add the snapshot to the source

    // find whether we already have a UPA subscription for this symbol/source, if we do then just added it, otherwise we have to create a new UPASubscription
    RMDSSource_ptr_t src;
    if (!sources_->Find(snap->SourceName(), src))
    {
        //notify invalid source
        t42log_info("Unknown source name '%s' for symbol '%s' \n", snap->SourceName().c_str(), snap->Symbol().c_str());
        snap->OnError(MAMA_STATUS_BAD_SYMBOL);
        return false;
    }

    UPASubscription_ptr_t upaSnap = src->CreateSubscription(snap->Symbol(), snap->LogRMDSValues());

    // save the subscription so we can destroy it on the way out
    snap->Subscription(upaSnap);

    upaSnap->SetDomain(src->SourceDomain());
    upaSnap->Source(src);
    upaSnap->Snapshot(consumer_, snap);

    return true;
}

bool RMDSSubscriber::SendInsertMessage( const std::string & sourceName, mamaMsg msg )
{
    t42log_warn("The enhanced version of the Tick42 RMDS bridge is required to accept posted messages");
    return false;
}
