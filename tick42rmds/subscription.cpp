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
#include "transport.h"
#include "RMDSBridgeImpl.h"
#include "RMDSSubscriber.h"

#define RMDSBridgeSub(subscriber) ((RMDSBridgeSubscription*)(subscriber))

#define CHECK_SUBSCRIBER(subscriber) \
	do {  \
	if (RMDSBridgeSub(subscriber) == 0) return MAMA_STATUS_NULL_ARG; \
	} while(0)
 
  //=========================================================================
  //=                         Mandatory Functions                           =
  // =========================================================================
 
 mama_status
 tick42rmdsBridgeMamaSubscription_create (subscriptionBridge*  subscriber,
                                   const char*          source,
                                   const char*          symbol,
                                   mamaTransport        transport,
                                   mamaQueue            queue,
                                   mamaMsgCallbacks     callback,
                                   mamaSubscription     subscription,
                                   void*                closure)
 {
	 CHECK_SUBSCRIBER(subscriber);
	 mama_status status;

	 // get hold of the bridge implementation
	 mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(transport);
	 if (!bridgeImpl) {
		 mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaSubscription_create(): Could not get bridge");
		 return MAMA_STATUS_PLATFORM;
	 }

	 RMDSBridgeImpl*  upaBridge = NULL;
	 if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &upaBridge))) 
	 {
		 mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaSubscription_create(): Could not get UPA bridge object");
		 return status;
	 }

	 // actually lookup on the transport itself
	 RMDSSubscriber_ptr_t sub = upaBridge->getTransportBridge(transport)->Subscriber();
	 sub->AddSubscription(subscriber, source, symbol, transport, queue, callback, subscription, closure);


     return MAMA_STATUS_OK;
 }
 
 
 
 mama_status
 tick42rmdsBridgeMamaSubscription_destroy (subscriptionBridge subscriber)
 {
	 CHECK_SUBSCRIBER(subscriber);

	 // Get the Bridge subscription object 
	 RMDSBridgeSubscription* pSubscription = RMDSBridgeSub(subscriber);

	 mama_status status;

	 // get hold of the bridge implementation
	 mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(pSubscription->Transport());
	 if (!bridgeImpl) {
		 mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaSubscription_destroy(): Could not get bridge");
		 return MAMA_STATUS_PLATFORM;
	 }

	 RMDSBridgeImpl*  upaBridge = NULL;
	 if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &upaBridge))) 
	 {
		 mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaSubscription_destroy(): Could not get UPA bridge object");
		 return status;
	 }

	 RMDSSubscriber_ptr_t sub = upaBridge->getTransportBridge(pSubscription->Transport())->Subscriber();

	 // grab stuff for onDestroyCB from our object before we kill it
	 void * closure = pSubscription->Closure();
	  wombat_subscriptionDestroyCB destroyCb = pSubscription->Callback().onDestroy;
	  mamaSubscription parent = pSubscription->Subscription();

	 sub->RemoveSubscription(pSubscription);

	     
     //Invoke the subscription callback to inform that the bridge has been
     //destroyed.
    if (NULL != destroyCb)
        (*(wombat_subscriptionDestroyCB)destroyCb)(parent, closure);

     return MAMA_STATUS_OK;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaSubscription_mute (subscriptionBridge subscriber)
 {
	 CHECK_SUBSCRIBER(subscriber);
	 // Get the Bridge subscription object 
	 RMDSBridgeSubscription* pSubscription = RMDSBridgeSub(subscriber);

	 // set the shutdown flag on the subscription;
	 // this will block if in a callback and also will signal that no further callbacks should be made
	 pSubscription->Shutdown();

     return MAMA_STATUS_OK;
 }
 
 
 int
 tick42rmdsBridgeMamaSubscription_isValid (subscriptionBridge subscriber)
 {
     return MAMA_STATUS_PLATFORM;
 }
 
 int
 tick42rmdsBridgeMamaSubscription_isTportDisconnected (subscriptionBridge subscriber)
 {
     return MAMA_STATUS_PLATFORM;
 }
 
  //=========================================================================
  //=                        Reccomended Functions                          =
  // =========================================================================
 
 mama_status
 tick42rmdsBridgeMamaSubscription_createWildCard (
                                 subscriptionBridge* subscriber,
                                 const char*         source,
                                 const char*         symbol,
                                 mamaTransport       transport,
                                 mamaQueue           queue,
                                 mamaMsgCallbacks    callback,
                                 mamaSubscription    subscription,
                                 void*               closure)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaSubscription_getPlatformError (subscriptionBridge  subscriber,
                                             void**              error)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaSubscription_setTopicClosure (subscriptionBridge subscriber,
                                            void*              closure)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaSubscription_muteCurrentTopic (subscriptionBridge subscriber)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 int
 tick42rmdsBridgeMamaSubscription_hasWildcards (subscriptionBridge subscriber)
 {
     return 0;
 }
 
