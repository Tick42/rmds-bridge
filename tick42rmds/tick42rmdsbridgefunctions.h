 /* 
  * OpenMAMA: The open middleware agnostic messaging API
  * Copyright (C) 2011 NYSE Inc.
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
  */
#pragma once
#ifndef __TICK42RMDSBRIDGEFUNCTIONS_H__
#define __TICK42RMDSBRIDGEFUNCTIONS_H__

#if defined(__cplusplus)
extern "C" {
#endif
 
 /*Definitions for all of the bridge functions which will be used by the
  *             MAMA code when delegating calls to the bridge         */
 
  /*=========================================================================
   =                    Functions for the bridge                           =
   =========================================================================*/
 MAMAExpBridgeDLL
 extern void
 tick42rmdsBridge_createImpl (mamaBridge* result);
 
 extern const char*
 tick42rmdsBridge_getVersion();
 
 mama_status
 tick42rmdsBridge_getDefaultPayloadId (char***name, char** id);
 
 extern mama_status
 tick42rmdsBridge_open (mamaBridge bridgeImpl);
 
 extern mama_status
 tick42rmdsBridge_close (mamaBridge bridgeImpl);
 
 extern mama_status
 tick42rmdsBridge_start (mamaQueue defaultEventQueue);
 
 extern mama_status
 tick42rmdsBridge_stop (mamaQueue defaultEventQueue);
 
 extern const char*
 tick42rmdsBridge_getName();
 

 /**
 * Get the underlying default mamaQueue of the bridge. the queue is referred as
 * impl->mDefaultEventQueue of mamaBridgeImpl
 * 
 * @param queue the requested queue
 */
mama_status
tick42rmdsBridge_getTheMamaQueue (mamaQueue* queue);

 
 /*=========================================================================
   =                    Functions for the mamaQueue                        =
   =========================================================================*/
 extern mama_status
 tick42rmdsBridgeMamaQueue_create (queueBridge *queue, mamaQueue parent);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_create_usingNative (queueBridge *queue, 
                                        mamaQueue    parent, 
                                        void*        nativeQueue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_destroy (queueBridge queue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_getEventCount (queueBridge queue, size_t* count);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_dispatch (queueBridge queue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_timedDispatch (queueBridge queue, uint64_t timeout);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_dispatchEvent (queueBridge queue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_enqueueEvent (queueBridge        queue,
                                  mamaQueueEnqueueCB callback,
                                  void*              closure);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_stopDispatch (queueBridge queue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_setEnqueueCallback (queueBridge        queue,
                                        mamaQueueEnqueueCB callback,
                                        void*              closure);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_removeEnqueueCallback (queueBridge queue);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_getNativeHandle (queueBridge  queue,
                                     void**       result);
 
 extern mama_status
 tick42rmdsBridgeMamaQueue_setLowWatermark (queueBridge  queue,
                                     size_t       lowWatermark);
 extern mama_status
 tick42rmdsBridgeMamaQueue_setHighWatermark (queueBridge queue,
                                      size_t      highWatermark);
 
 
 /*=========================================================================
   =                    Functions for the mamaTransport                    =
   =========================================================================*/
 extern int
 tick42rmdsBridgeMamaTransport_isValid (transportBridge transport);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_destroy (transportBridge transport);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_create (transportBridge* result,
                                const char*      name,
                                mamaTransport    parent);
 extern mama_status
 tick42rmdsBridgeMamaTransport_findConnection (transportBridge* transports,
                                        int              numTransports,
                                        mamaConnection*  result,
                                        const char*      ipAddress,
                                        uint16_t         port);
 extern mama_status
 tick42rmdsBridgeMamaTransport_getAllConnections (transportBridge*  transports,
                                           int               numTransports,
                                           mamaConnection**  result,
                                           uint32_t*         len);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_getAllConnectionsForTopic (
                                              transportBridge* transports,
                                              int              numTransports,
                                              const char*      topic,
                                              mamaConnection** result,
                                              uint32_t*        len);
 extern mama_status
 tick42rmdsBridgeMamaTransport_freeAllConnections (transportBridge*  transports,
                                            int               numTransports,
                                            mamaConnection*   connections,
                                            uint32_t          len);
 extern mama_status
 tick42rmdsBridgeMamaTransport_getAllServerConnections (
                                         transportBridge*       transports,
                                         int                    numTransports,
                                         mamaServerConnection** result,
                                         uint32_t*              len);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_freeAllServerConnections (
                                         transportBridge*       transports,
                                         int                    numTransports,
                                         mamaServerConnection*  connections,
                                         uint32_t               len);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_getNumLoadBalanceAttributes (
                                     const char* name,
                                     int*        numLoadBalanceAttributes);
 extern mama_status
 tick42rmdsBridgeMamaTransport_getLoadBalanceSharedObjectName (
                                     const char*  name,
                                     const char** loadBalanceSharedObjectName);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_getLoadBalanceScheme (const char*    name,
                                              tportLbScheme* scheme);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_sendMsgToConnection (
                                     transportBridge transport,
                                     mamaConnection  connection,
                                     mamaMsg         msg,
                                     const char*     topic);
 extern mama_status
 tick42rmdsBridgeMamaTransport_isConnectionIntercepted (
                                     mamaConnection connection,
                                     uint8_t*       result);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_installConnectConflateMgr (
                                     transportBridge       transport,
                                     mamaConflationManager mgr,
                                     mamaConnection        connection,
                                     conflateProcessCb     processCb,
                                     conflateGetMsgCb      msgCb);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_uninstallConnectConflateMgr (
                                     transportBridge       transport,
                                     mamaConflationManager mgr,
                                     mamaConnection        connection);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_startConnectionConflation (
                                     transportBridge        transport,
                                     mamaConflationManager  mgr,
                                     mamaConnection         connection);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_requestConflation (transportBridge* transports,
                                           int              numTransports);
 extern mama_status
 tick42rmdsBridgeMamaTransport_requestEndConflation (transportBridge* transports,
                                              int              numTransports);
 
 extern mama_status
 tick42rmdsBridgeMamaTransport_getNativeTransport (transportBridge transport,
                                            void**          result);
 extern mama_status
 tick42rmdsBridgeMamaTransport_getNativeTransportNamingCtx (transportBridge transport,
                                                     void**          result);
 
 extern mama_status 
 tick42rmdsBridgeMamaTransport_forceClientDisconnect (transportBridge* transports,
                                               int              numTransports,
                                               const char*      ipAddress,
                                               uint16_t         port);
 
 
 /*=========================================================================
   =                    Functions for the mamaSubscription                 =
   =========================================================================*/
 extern mama_status tick42rmdsBridgeMamaSubscription_create (
                                 subscriptionBridge* subsc_,
                                 const char*         source,
                                 const char*         symbol,
                                 mamaTransport       transport,
                                 mamaQueue           queue,
                                 mamaMsgCallbacks    callback,
                                 mamaSubscription    subscription,
                                 void*               closure );
 
 extern mama_status
 tick42rmdsBridgeMamaSubscription_createWildCard (
                                 subscriptionBridge* subsc_,
                                 const char*         source,
                                 const char*         symbol,
                                 mamaTransport       transport,
                                 mamaQueue           queue,
                                 mamaMsgCallbacks    callback,
                                 mamaSubscription    subscription,
                                 void*               closure );
 
 extern mama_status tick42rmdsBridgeMamaSubscription_mute
                                 (subscriptionBridge subscriber);
 
 extern  mama_status tick42rmdsBridgeMamaSubscription_destroy
                                 (subscriptionBridge subscriber);
 
 extern int tick42rmdsBridgeMamaSubscription_isValid
                                 (subscriptionBridge bridge);
 
 extern mama_status tick42rmdsBridgeMamaSubscription_getSubject
                                 (subscriptionBridge subscriber,
                                  const char**       subject);
 
 extern int tick42rmdsBridgeMamaSubscription_hasWildcards
                                 (subscriptionBridge subscriber);
 
 extern mama_status tick42rmdsBridgeMamaSubscription_getPlatformError
                                 (subscriptionBridge subsc, void** error);
 
 extern mama_status tick42rmdsBridgeMamaSubscription_setTopicClosure
                                 (subscriptionBridge subsc, void* closure);
 
 extern mama_status tick42rmdsBridgeMamaSubscription_muteCurrentTopic
                                 (subscriptionBridge subsc);
 
 extern int tick42rmdsBridgeMamaSubscription_isTportDisconnected
                                 (subscriptionBridge subsc);
 
 
 /*=========================================================================
   =                    Functions for the mamaTimer                        =
   =========================================================================*/
 extern mama_status tick42rmdsBridgeMamaTimer_create (timerBridge* timer,
                                               void*        nativeQueueHandle,
                                               mamaTimerCb  action,
                                               mamaTimerCb  onTimerDestroyed,
                                               mama_f64_t   interval,
                                               mamaTimer    parent,
                                               void*        closure);
 
 extern mama_status tick42rmdsBridgeMamaTimer_destroy (timerBridge timer);
 
 extern mama_status tick42rmdsBridgeMamaTimer_reset (timerBridge timer);
 
 extern mama_status tick42rmdsBridgeMamaTimer_setInterval (timerBridge timer,
                                                    mama_f64_t  interval);
 
 extern mama_status tick42rmdsBridgeMamaTimer_getInterval (timerBridge timer,
                                                    mama_f64_t* interval);
 
 
 /*=========================================================================
   =                    Functions for the mamaIo                           =
   =========================================================================*/
 extern mama_status
 tick42rmdsBridgeMamaIo_create (ioBridge*   result,
                         void*       nativeQueueHandle,
                         uint32_t    descriptor,
                         mamaIoCb    action,
                         mamaIoType  ioType,
                         mamaIo      parent,
                         void*       closure);
 
 extern mama_status
 tick42rmdsBridgeMamaIo_destroy (ioBridge io);
 
 extern mama_status
 tick42rmdsBridgeMamaIo_getDescriptor (ioBridge io, uint32_t* result);
 
 
 /*=========================================================================
   =                    Functions for the mamaPublisher                    =
   =========================================================================*/
 extern mama_status 
 tick42rmdsBridgeMamaPublisher_createByIndex (
                                publisherBridge*  result,
                                mamaTransport     tport,
                                int               tportIndex,
                                const char*       topic,
                                const char*       source,
                                const char*       root,
                                void*             nativeQueueHandle,
                                mamaPublisher     parent);
 
 extern mama_status
 tick42rmdsBridgeMamaPublisher_create (publisherBridge* result,
                                mamaTransport    tport,
                                const char*      topic,
                                const char*      source,
                                const char*      root,
                                void*            nativeQueueHandle,
                                mamaPublisher    parent);
 
 extern mama_status
 tick42rmdsBridgeMamaPublisher_destroy (publisherBridge publisher);
 
 extern mama_status
 tick42rmdsBridgeMamaPublisher_send (publisherBridge publisher, mamaMsg msg);
 
 extern mama_status
 tick42rmdsBridgeMamaPublisher_sendReplyToInbox (publisherBridge publisher,
                                          void *         request,
                                          mamaMsg         reply);
 
 extern mama_status
 tick42rmdsBridgeMamaPublisher_sendFromInbox (publisherBridge publisher,
                                       mamaInbox       inbox,
                                       mamaMsg         msg);
 extern mama_status
 tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex (publisherBridge publisher,
                                              int             tportIndex,
                                              mamaInbox       inbox,
                                              mamaMsg         msg);
 extern mama_status
 tick42rmdsBridgeMamaPublisher_sendReplyToInboxHandle (publisherBridge publisher,
                                                void *          wmwReply,
                                                mamaMsg         reply);
 
 
 /*=========================================================================
   =                    Functions for the mamaInbox                        =
   =========================================================================*/
 extern mama_status
 tick42rmdsBridgeMamaInbox_create (
             inboxBridge*             bridge,
             mamaTransport            tport,
             mamaQueue                queue,
             mamaInboxMsgCallback     msgCB,
             mamaInboxErrorCallback   errorCB,
             mamaInboxDestroyCallback onInboxDestroyed,
             void*                    closure,
             mamaInbox                parent);
 
 extern mama_status
 tick42rmdsBridgeMamaInbox_createByIndex (
             inboxBridge*             bridge,
             mamaTransport            tport,
             int                      tportIndex,
             mamaQueue                queue,
             mamaInboxMsgCallback     msgCB,
             mamaInboxErrorCallback   errorCB,
             mamaInboxDestroyCallback onInboxDestroyed,
             void*                    closure,
             mamaInbox                parent);
 
 extern mama_status
 tick42rmdsBridgeMamaInbox_destroy (inboxBridge inbox);
 
 
 /*=========================================================================
   =                    Functions for the mamaMsg                           =
   =========================================================================*/
 extern mama_status
 tick42rmdsBridgeMamaMsg_create (msgBridge* msg, mamaMsg parent);
 
 extern int
 tick42rmdsBridgeMamaMsg_isFromInbox (msgBridge msg);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_destroy (msgBridge msg, int destroyMsg);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_destroyMiddlewareMsg (msgBridge msg);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_detach (msgBridge msg);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_getPlatformError (msgBridge msg, void** error);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_setSendSubject (msgBridge   msg,
                                  const char* symbol,
                                  const char* subject);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_getNativeHandle (msgBridge msg, void** result);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_duplicateReplyHandle (msgBridge msg, void** result);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_copyReplyHandle (void* src, void** dest);
 
 extern mama_status
 tick42rmdsBridgeMamaMsg_destroyReplyHandle (void* result);
 
 extern mama_status
 tick42rmdsBridgeMamaMsgImpl_setReplyHandle (msgBridge msg, void* result);
 
 extern mama_status
 tick42rmdsBridgeMamaMsgImpl_setReplyHandleAndIncrement (msgBridge msg, void* result);
 
 extern mama_status
 tick42rmdsBridgeMamaMsgImpl_setAttributesAndSecure (msgBridge msg, void* attributes, uint8_t secure);
 
 #if defined(__cplusplus)
 } 
 #endif

#endif //__TICK42RMDSBRIDGEFUNCTIONS_H__