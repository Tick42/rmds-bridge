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
#include "utils/t42log.h"


typedef struct upaQueueBridge_t 
{
    mamaQueue          parent_;
    wombatQueue        queue_;
    uint8_t            isNative_;
} upaQueueBridge_t;

typedef struct upaQueueClosure_t 
{
    upaQueueBridge_t* impl_;
    mamaQueueEventCB cb_;
    void*            userClosure_;
} upaQueueClosure_t;

#define upaQueue(queue) ((upaQueueBridge_t*) queue)
#define CHECK_QUEUE(queue) \
    do {  \
    if (upaQueue(queue) == 0) return MAMA_STATUS_NULL_ARG; \
    if (upaQueue(queue)->queue_ == NULL) return MAMA_STATUS_NULL_ARG; \
    } while(0)


  /*=========================================================================
   =                         Mandatory Functions                            =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaQueue_create (queueBridge*  queue,
                            mamaQueue     parent)
 {
     // wrap our bridge queue type around a wonbat queue
     upaQueueBridge_t* upaQueue = NULL;
     if (queue == NULL)
         return MAMA_STATUS_NULL_ARG;
     *queue = NULL;

     upaQueue = (upaQueueBridge_t*)calloc (1, sizeof (upaQueueBridge_t));
     if (upaQueue == NULL)
         return MAMA_STATUS_NOMEM;

     upaQueue->parent_  = parent;

     wombatQueue_allocate (&upaQueue->queue_);
     wombatQueue_create (upaQueue->queue_, 0, 0, 0);

     *queue = (queueBridge) upaQueue;

     return MAMA_STATUS_OK;


 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_destroy (queueBridge queue)
 {
     // destroy the wombat queue and free the bridge queue
     CHECK_QUEUE(queue);
     if (upaQueue(queue)->isNative_)
         wombatQueue_destroy (upaQueue(queue)->queue_);
     free(upaQueue(queue));
     return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_dispatch (queueBridge queue)
 {
     wombatQueueStatus status;

     // dispatch  an object with a 500 ms timeout
     CHECK_QUEUE(queue);
     do
     {
         /* 500 is .5 seconds */
         status = wombatQueue_timedDispatch (upaQueue(queue)->queue_,
             NULL, NULL, 500);
     }
     while ((status == WOMBAT_QUEUE_OK ||
         status == WOMBAT_QUEUE_TIMEOUT) &&
         mamaQueueImpl_isDispatching (upaQueue(queue)->parent_));

     if (status != WOMBAT_QUEUE_OK && status != WOMBAT_QUEUE_TIMEOUT)
     {
         mama_log (MAMA_LOG_LEVEL_ERROR,
             "Failed to dispatch upa Middleware queue. %d",
             "mamaQueue_dispatch ():",
             status);
         return MAMA_STATUS_PLATFORM;
     }

     return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_timedDispatch (queueBridge queue, uint64_t timeout)
 {
     wombatQueueStatus status;
     CHECK_QUEUE(queue);

     // dispatch an object with specified timeout
     status = wombatQueue_timedDispatch (upaQueue(queue)->queue_,
         NULL, NULL, timeout);
     if (status == WOMBAT_QUEUE_TIMEOUT)
         return MAMA_STATUS_TIMEOUT;

     if (status != WOMBAT_QUEUE_OK)
     {
         mama_log (MAMA_LOG_LEVEL_ERROR,
             "Failed to dispatch upa Middleware queue. %d",
             "mamaQueue_timeddispatch ():",
             status);
         return MAMA_STATUS_PLATFORM;
     }

     return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_dispatchEvent (queueBridge queue)
 {
     wombatQueueStatus status;
     CHECK_QUEUE(queue);

     status = wombatQueue_dispatch (upaQueue(queue)->queue_,
         NULL, NULL);

     if (status != WOMBAT_QUEUE_OK)
     {
         mama_log (MAMA_LOG_LEVEL_ERROR,
             "Failed to dispatch upa Middleware queue. %d",
             "mamaQueue_dispatchEvent ():",
             status);
         return MAMA_STATUS_PLATFORM;
     }

     return MAMA_STATUS_OK;

 }
 
 
 // call back for queued event
 static void MAMACALLTYPE queueCb (void *ignored, void* closure)
 {
     upaQueueClosure_t* cl = (upaQueueClosure_t*)closure;
     if (NULL ==cl) return;
     try
     {
         cl->cb_ (cl->impl_->parent_, cl->userClosure_);
     }
     catch (...)
     {
         t42log_warn("Caught exception in queue callback\n");
     }


     free (cl);
 }



 mama_status
 tick42rmdsBridgeMamaQueue_enqueueEvent (queueBridge      queue,
                                  mamaQueueEventCB callback,
                                  void*            closure)
 {
     wombatQueueStatus status;
     upaQueueClosure_t* cl = NULL;
     CHECK_QUEUE(queue);

     cl = (upaQueueClosure_t*)calloc(1, sizeof(upaQueueClosure_t));
     if (NULL == cl) return MAMA_STATUS_NOMEM;

     cl->impl_ = upaQueue(queue);
     cl->cb_    = callback;
     cl->userClosure_ = closure;

     status = wombatQueue_enqueue (upaQueue(queue)->queue_,
         queueCb, NULL, cl);

     if (status != WOMBAT_QUEUE_OK)
    {
       //!!! DP Does this happen often - will it cause a memory leak?
         return MAMA_STATUS_PLATFORM;
    }

     return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_stopDispatch (queueBridge queue)
 {
     wombatQueueStatus status;
     CHECK_QUEUE(queue);

     if (queue == NULL)
         return MAMA_STATUS_NULL_ARG;

     status = wombatQueue_unblock (upaQueue(queue)->queue_);
     if (status != WOMBAT_QUEUE_OK)
     {
         mama_log (MAMA_LOG_LEVEL_ERROR,
             " Failed to stop dispatching upa Middleware queue.",
             "wmwMamaQueue_stopDispatch ():");
         return MAMA_STATUS_PLATFORM;
     }

     return MAMA_STATUS_OK;

 }
 
  /*=========================================================================
   =                        Recommended Functions                           =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaQueue_create_usingNative (queueBridge*  queue,
                                        mamaQueue     parent,
                                        void*         nativeQueue)
 {
     // wrap a call specified queue
     upaQueueBridge_t* upaQueue = NULL;
     if (queue == NULL)
         return MAMA_STATUS_NULL_ARG;
     *queue = NULL;

     upaQueue = (upaQueueBridge_t*)calloc (1, sizeof (upaQueueBridge_t));
     if (upaQueue == NULL)
         return MAMA_STATUS_NOMEM;

     upaQueue->parent_  = parent;
     upaQueue->queue_   = (wombatQueue)nativeQueue;
     upaQueue->isNative_ = 1;

     *queue = (queueBridge) upaQueue;

     return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_setEnqueueCallback (queueBridge        queue,
                                        mamaQueueEnqueueCB callback,
                                        void*              closure)
 {
    CHECK_QUEUE(queue);
    return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_removeEnqueueCallback (queueBridge queue)
 {
     CHECK_QUEUE(queue);
    return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_getNativeHandle (queueBridge  queue,
                                     void**       result)
 {
     CHECK_QUEUE(queue);
     *result = upaQueue(queue)->queue_;

     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_setHighWatermark (queueBridge  queue,
                                      size_t       highWatermark)
 {
    CHECK_QUEUE(queue);
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_setLowWatermark (queueBridge  queue,
                                     size_t       lowWatermark)
 {
    CHECK_QUEUE(queue);
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaQueue_getEventCount (queueBridge queue, size_t* count)
 {
     CHECK_QUEUE(queue);
     *count = 0;
     wombatQueue_getSize (upaQueue(queue)->queue_, (int*)count);
     return MAMA_STATUS_OK;
 }
