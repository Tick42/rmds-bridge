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
#include "version.h"
#include "tick42rmdsbridgefunctions.h"
#include "RMDSSubscriber.h"
#include "RMDSBridgeImpl.h"
#include "transport.h"
#include "utils/t42log.h"

static mamaQueue gPublisher_MamaQueue= NULL;
timerHeap gTimerHeap = NULL;


static const char* PAYLOAD_NAMES[] = {"tick42rmdsmsg", NULL};
static char PAYLOAD_IDS[] = {MAMA_PAYLOAD_TICK42RMDS, NULL};
/*=========================================================================
=                         Mandatory Functions                            =
=========================================================================*/

mama_status tick42rmdsBridge_init(mamaBridge bridgeImpl)
{
    mama_status status = MAMA_STATUS_OK;

    /* Will set the bridge's compile time MAMA version */
    MAMA_SET_BRIDGE_COMPILE_TIME_VERSION(BRIDGE_NAME_STRING);

    /* Ensure that the bridge is defined as not deferring entitlements */
    status = mamaBridgeImpl_setReadOnlyProperty(bridgeImpl,
                                                MAMA_PROP_BARE_ENT_DEFERRED,
                                                "false");

    // If we need to we can put some object heRE
    RMDSBridgeImpl* upaBridge = new RMDSBridgeImpl();
    mamaBridgeImpl_setClosure(bridgeImpl, upaBridge);

    return status;
}

const char*
tick42rmdsBridge_getVersion (void)
{
   return BRIDGE_NAME_STRING " " BRIDGE_VERSION_STRING;
}

const char*
tick42rmdsBridge_getName (void)
{
   return BRIDGE_NAME_STRING;
}

mama_bool_t
tick42rmdsBridge_areEntitlementsDeferred(mamaBridge bridgeImpl)
{
    return true;
}

mama_status
tick42rmdsBridge_getDefaultPayloadId (char*** name, char** id)
{
   // const cast the static constant string to remove warnings on some compilers.
   // A better solution could be for openMAMA to define two signatures  the current signature should be change to
   // XXX_getDefaultPayloadId (const char***name, char** id)

   const char* payload = mama_getProperty ("mama.tick42rmds.payload.name");
   mama_log(MAMA_LOG_LEVEL_NORMAL, "tick42rmdsBridge_getDefaultPayloadId: payload=%s", payload);
   if (payload)
   {
       PAYLOAD_NAMES[0] = payload;
   }

   *name = const_cast<char **>(PAYLOAD_NAMES);
   *id = PAYLOAD_IDS;
   return MAMA_STATUS_OK;
}


mama_status
tick42rmdsBridge_open (mamaBridge bridgeImpl)
{

   mama_status status = MAMA_STATUS_OK;
   mamaBridgeImpl* impl =  (mamaBridgeImpl*)bridgeImpl;

   wsocketstartup();
   mama_log (MAMA_LOG_LEVEL_FINEST, "tick42rmdsBridge_open(): Entering.");

   if (MAMA_STATUS_OK !=
      (status =  mamaQueue_create (&impl->mDefaultEventQueue, bridgeImpl)))
   {
      mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridge_open():"
         "Failed to create upa bridge queue.");
      return status;
   }

   mamaQueue_setQueueName (impl->mDefaultEventQueue,
      "UPA_DEFAULT_MAMA_QUEUE");


   gPublisher_MamaQueue = impl->mDefaultEventQueue;
   mama_log (MAMA_LOG_LEVEL_NORMAL,
      "tick42rmdsBridge_open(): Successfully created tick42upa queue");

   if (0 != createTimerHeap(&gTimerHeap))
   {
      mama_log (MAMA_LOG_LEVEL_NORMAL,
         "tick42rmdsBridge_open(): Failed to initialize timers.");
      return MAMA_STATUS_PLATFORM;
   }

   if (0 != startDispatchTimerHeap(gTimerHeap))
   {
      mama_log (MAMA_LOG_LEVEL_NORMAL,
         "tick42rmdsBridge_open(): Failed to start timer thread.");
      return MAMA_STATUS_PLATFORM;
   }

   return MAMA_STATUS_OK;
}


mama_status tick42rmdsBridge_close(mamaBridge bridgeImpl)
{
   mama_status status = MAMA_STATUS_OK;
   wthread_t timerThread = INVALID_THREAD;

   mamaBridgeImpl* impl =  (mamaBridgeImpl*)bridgeImpl;
   RMDSBridgeImpl* upaBridge = NULL;
   mamaBridgeImpl_getClosure((mamaBridge) mamaQueueImpl_getBridgeImpl(impl->mDefaultEventQueue), (void**) &upaBridge);
   for (size_t index = 0; index < upaBridge->NumTransports(); index++)
   {
      upaBridge->getTransportBridge(index)->Stop();
   }

   /* Remove the timer heap */
   if (NULL != gTimerHeap)
   {
      mama_log (MAMA_LOG_LEVEL_FINEST, "tick42rmdsBridge_close(): destroying RMDS timer heap");
      /* The timer heap allows us to access its thread ID for joining */
      timerThread = timerHeapGetTid(gTimerHeap);
      if (0 != destroyHeap(gTimerHeap))
      {
         mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridge_close(): Failed to destroy RMDS timer heap");
         status = MAMA_STATUS_PLATFORM;
      }
      gTimerHeap = NULL;

      /* The timer thread expects us to be responsible for terminating it */
      if (INVALID_THREAD != timerThread)
      {
         mama_log (MAMA_LOG_LEVEL_FINEST, "tick42rmdsBridge_close(): joining timer thread.");
         wthread_join(timerThread, NULL);
         wthread_destroy(timerThread);
      }
   }
   mama_log (MAMA_LOG_LEVEL_FINEST, "tick42rmdsBridge_close(): finished");
   return status;
}

mama_status
   tick42rmdsBridge_start(mamaQueue defaultEventQueue)
{
   mama_status status = MAMA_STATUS_NOT_INITIALISED;
   RMDSBridgeImpl* upaBridge = NULL;

   if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) mamaQueueImpl_getBridgeImpl(defaultEventQueue), (void**) &upaBridge)))
   {
      t42log_error("tick42rmdsBridge_start(): Could not get rmds bridge object\n");
      return status;
   }

   if (!upaBridge->hasTransportBridge())
   {
      // just dispatch on the default queue and return.
      // the transport bridge create will start the transports
      t42log_info("tick42rmdsBridge_start: start dispatch default queue");
      return mamaQueue_dispatch(defaultEventQueue);
   }

   // start up all the transports
   for (size_t index = 0; index < upaBridge->NumTransports(); index++)
   {
      // Calling Resume() will call Start(), if the bridge has never been started
      t42log_info("tick42rmdsBridge_start: start dispatch transport #%d", index+1);
      const RMDSTransportBridge_ptr_t& transportBridgePtr = upaBridge->getTransportBridge(index);
      if (!transportBridgePtr->Stopped())
      {
          mama_status result = transportBridgePtr->Resume();
          if ( result != MAMA_STATUS_OK)
          {
             //can just wait and start on transport create
             t42log_error ("tick42rmdsBridge_start(): Could not start dispatching on rmds\n");
             return result;
          }
      }
   }

   // start Mama event loop
   return mamaQueue_dispatch(defaultEventQueue);
}


mama_status
   tick42rmdsBridge_stop(mamaQueue defaultEventQueue)
{
   mama_status status = MAMA_STATUS_OK;
   RMDSBridgeImpl* rmdsBridge = NULL;

   mama_log (MAMA_LOG_LEVEL_NORMAL, "tick42rmdsBridge_stop(): Stopping bridge.");
   if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) mamaQueueImpl_getBridgeImpl(defaultEventQueue), (void**) &rmdsBridge))) {
      mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridge_stop(): Could not get RMDS bridge object");
      return status;
   }


   // stop Mama event loop
   status = mamaQueue_stopDispatch (defaultEventQueue);
   if (status != MAMA_STATUS_OK)
   {
      mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridge_stop(): Failed to unblock  queue.");
      return status;
   }

   // Pause the transport to prevent any more updates from being sent out
   RMDSBridgeImpl *upaBridge = NULL;
   mamaBridgeImpl_getClosure((mamaBridge) mamaQueueImpl_getBridgeImpl(defaultEventQueue), (void**) &upaBridge);
   for (size_t index = 0; index < upaBridge->NumTransports(); index++)
   {
      upaBridge->getTransportBridge(index)->Pause();
   }

   return MAMA_STATUS_OK;
}

// this provides global access to the queue dioctionary messages are sent on
mama_status
   tick42rmdsBridge_getTheMamaQueue (mamaQueue* queue)
{
   if (!gPublisher_MamaQueue)
      return MAMA_STATUS_NOT_INITIALISED;

   *queue = gPublisher_MamaQueue;

   return MAMA_STATUS_OK;
}
