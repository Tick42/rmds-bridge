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

#include <wombat/wConfig.h>
#include <mama/types.h>

#if defined(__cplusplus)
extern "C" {
#endif
 
/*Definitions for all of the bridge functions which will be used by the
*             MAMA code when delegating calls to the bridge         */
 
/*=========================================================================
=                    Functions for the bridge                           =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridge_init(mamaBridge bridgeImpl);
 
MAMAExpBridgeDLL
extern const char*
tick42rmdsBridge_getVersion();
 
MAMAExpBridgeDLL
mama_status
tick42rmdsBridge_getDefaultPayloadId (char***name, char** id);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridge_open(mamaBridge bridgeImpl);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridge_close (mamaBridge bridgeImpl);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridge_start (mamaQueue defaultEventQueue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridge_stop (mamaQueue defaultEventQueue);
 
MAMAExpBridgeDLL
extern const char*
tick42rmdsBridge_getName();
 
MAMAExpBridgeDLL
extern mama_bool_t
tick42rmdsBridge_areEntitlementsDeferred(mamaBridge bridgeImpl);

/**
* Get the underlying default mamaQueue of the bridge. the queue is referred as
* impl->mDefaultEventQueue of mamaBridgeImpl
* 
* @param queue the requested queue
*/
MAMAExpBridgeDLL
mama_status
tick42rmdsBridge_getTheMamaQueue (mamaQueue* queue);

 
/*=========================================================================
  =                    Functions for the mamaQueue                        =
  =========================================================================*/
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_create (queueBridge *queue, mamaQueue parent);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_create_usingNative (queueBridge *queue, 
                                    mamaQueue    parent, 
                                    void*        nativeQueue);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_destroy (queueBridge queue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_getEventCount (queueBridge queue, size_t* count);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_dispatch (queueBridge queue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_timedDispatch (queueBridge queue, uint64_t timeout);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_dispatchEvent (queueBridge queue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_enqueueEvent (queueBridge        queue,
                                mamaQueueEnqueueCB callback,
                                void*              closure);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_stopDispatch (queueBridge queue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_setEnqueueCallback (queueBridge        queue,
                                    mamaQueueEnqueueCB callback,
                                    void*              closure);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_removeEnqueueCallback (queueBridge queue);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_getNativeHandle (queueBridge  queue,
                                    void**       result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_setLowWatermark (queueBridge  queue,
                                    size_t       lowWatermark);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaQueue_setHighWatermark (queueBridge queue,
                                    size_t      highWatermark);
 
 
/*=========================================================================
=                    Functions for the mamaTransport                    =
=========================================================================*/
MAMAExpBridgeDLL
extern int
tick42rmdsBridgeMamaTransport_isValid (transportBridge transport);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_destroy (transportBridge transport);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_create (transportBridge* result,
                            const char*      name,
                            mamaTransport    parent);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_findConnection (transportBridge* transports,
                                    int              numTransports,
                                    mamaConnection*  result,
                                    const char*      ipAddress,
                                    uint16_t         port);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getAllConnections (transportBridge*  transports,
                                        int               numTransports,
                                        mamaConnection**  result,
                                        uint32_t*         len);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getAllConnectionsForTopic (
                                            transportBridge* transports,
                                            int              numTransports,
                                            const char*      topic,
                                            mamaConnection** result,
                                            uint32_t*        len);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_freeAllConnections (transportBridge*  transports,
                                        int               numTransports,
                                        mamaConnection*   connections,
                                        uint32_t          len);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getAllServerConnections (
                                        transportBridge*       transports,
                                        int                    numTransports,
                                        mamaServerConnection** result,
                                        uint32_t*              len);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_freeAllServerConnections (
                                        transportBridge*       transports,
                                        int                    numTransports,
                                        mamaServerConnection*  connections,
                                        uint32_t               len);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getNumLoadBalanceAttributes (
                                    const char* name,
                                    int*        numLoadBalanceAttributes);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getLoadBalanceSharedObjectName (
                                    const char*  name,
                                    const char** loadBalanceSharedObjectName);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getLoadBalanceScheme (const char*    name,
                                            tportLbScheme* scheme);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_sendMsgToConnection (
                                    transportBridge transport,
                                    mamaConnection  connection,
                                    mamaMsg         msg,
                                    const char*     topic);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_isConnectionIntercepted (
                                    mamaConnection connection,
                                    uint8_t*       result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_installConnectConflateMgr (
                                    transportBridge       transport,
                                    mamaConflationManager mgr,
                                    mamaConnection        connection,
                                    conflateProcessCb     processCb,
                                    conflateGetMsgCb      msgCb);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_uninstallConnectConflateMgr (
                                    transportBridge       transport,
                                    mamaConflationManager mgr,
                                    mamaConnection        connection);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_startConnectionConflation (
                                    transportBridge        transport,
                                    mamaConflationManager  mgr,
                                    mamaConnection         connection);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_requestConflation (transportBridge* transports,
                                        int              numTransports);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_requestEndConflation (transportBridge* transports,
                                            int              numTransports);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getNativeTransport (transportBridge transport,
                                        void**          result);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaTransport_getNativeTransportNamingCtx (transportBridge transport,
                                                    void**          result);
 
MAMAExpBridgeDLL
extern mama_status 
tick42rmdsBridgeMamaTransport_forceClientDisconnect (transportBridge* transports,
                                            int              numTransports,
                                            const char*      ipAddress,
                                            uint16_t         port);
 
 
/*=========================================================================
=                    Functions for the mamaSubscription                 =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_create (
                                subscriptionBridge* subsc_,
                                const char*         source,
                                const char*         symbol,
                                mamaTransport       transport,
                                mamaQueue           queue,
                                mamaMsgCallbacks    callback,
                                mamaSubscription    subscription,
                                void*               closure );
 
MAMAExpBridgeDLL
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
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_mute
                                (subscriptionBridge subscriber);
 
MAMAExpBridgeDLL
extern  mama_status tick42rmdsBridgeMamaSubscription_destroy
                                (subscriptionBridge subscriber);
 
MAMAExpBridgeDLL
extern int tick42rmdsBridgeMamaSubscription_isValid
                                (subscriptionBridge bridge);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_getSubject
                                (subscriptionBridge subscriber,
                                const char**       subject);
 
MAMAExpBridgeDLL
extern int tick42rmdsBridgeMamaSubscription_hasWildcards
                                (subscriptionBridge subscriber);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_getPlatformError
                                (subscriptionBridge subsc, void** error);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_setTopicClosure
                                (subscriptionBridge subsc, void* closure);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaSubscription_muteCurrentTopic
                                (subscriptionBridge subsc);
 
MAMAExpBridgeDLL
extern int tick42rmdsBridgeMamaSubscription_isTportDisconnected
                                (subscriptionBridge subsc);
 
 
/*=========================================================================
=                    Functions for the mamaTimer                        =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaTimer_create (timerBridge* timer,
                                            void*        nativeQueueHandle,
                                            mamaTimerCb  action,
                                            mamaTimerCb  onTimerDestroyed,
                                            mama_f64_t   interval,
                                            mamaTimer    parent,
                                            void*        closure);

MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaTimer_destroy (timerBridge timer);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaTimer_reset (timerBridge timer);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaTimer_setInterval (timerBridge timer,
                                                mama_f64_t  interval);
 
MAMAExpBridgeDLL
extern mama_status tick42rmdsBridgeMamaTimer_getInterval (timerBridge timer,
                                                mama_f64_t* interval);
 
 
/*=========================================================================
=                    Functions for the mamaIo                           =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaIo_create (ioBridge*   result,
                        void*       nativeQueueHandle,
                        uint32_t    descriptor,
                        mamaIoCb    action,
                        mamaIoType  ioType,
                        mamaIo      parent,
                        void*       closure);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaIo_destroy (ioBridge io);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaIo_getDescriptor (ioBridge io, uint32_t* result);
 
 
/*=========================================================================
=                    Functions for the mamaPublisher                    =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status 
tick42rmdsBridgeMamaPublisher_createByIndex (publisherBridge*  result,
                            mamaTransport     tport,
                            int               tportIndex,
                            const char*       topic,
                            const char*       source,
                            const char*       root,
                            mamaPublisher     parent);
  
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_destroy (publisherBridge publisher);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_send (publisherBridge publisher, mamaMsg msg);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_sendReplyToInbox (publisherBridge publisher,
                                        void *         request,
                                        mamaMsg         reply);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_sendFromInbox (publisherBridge publisher,
                                    mamaInbox       inbox,
                                    mamaMsg         msg);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex (publisherBridge publisher,
                                            int             tportIndex,
                                            mamaInbox       inbox,
                                            mamaMsg         msg);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaPublisher_sendReplyToInboxHandle (publisherBridge publisher,
                                            void *          wmwReply,
                                            mamaMsg         reply);
 
MAMAExpBridgeDLL
mama_status
tick42rmdsBridgeMamaPublisher_setUserCallbacks(publisherBridge publisher,
    mamaQueue               queue,
    mamaPublisherCallbacks* cb,
    void*                   closure);
 
/*=========================================================================
=                    Functions for the mamaInbox                        =
=========================================================================*/
MAMAExpBridgeDLL
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
 
MAMAExpBridgeDLL
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
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaInbox_destroy (inboxBridge inbox);
 
 
/*=========================================================================
=                    Functions for the mamaMsg                           =
=========================================================================*/
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_create (msgBridge* msg, mamaMsg parent);
 
MAMAExpBridgeDLL
extern int
tick42rmdsBridgeMamaMsg_isFromInbox (msgBridge msg);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_destroy (msgBridge msg, int destroyMsg);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_destroyMiddlewareMsg (msgBridge msg);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_detach (msgBridge msg);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_getPlatformError (msgBridge msg, void** error);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_setSendSubject (msgBridge   msg,
                                const char* symbol,
                                const char* subject);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_getNativeHandle (msgBridge msg, void** result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_duplicateReplyHandle (msgBridge msg, void** result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_copyReplyHandle (void* src, void** dest);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsg_destroyReplyHandle (void* result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsgImpl_setReplyHandle (msgBridge msg, void* result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsgImpl_setReplyHandleAndIncrement (msgBridge msg, void* result);
 
MAMAExpBridgeDLL
extern mama_status
tick42rmdsBridgeMamaMsgImpl_setAttributesAndSecure (msgBridge msg, void* attributes, uint8_t secure);
 
#if defined(__cplusplus)
} 
#endif

#endif //__TICK42RMDSBRIDGEFUNCTIONS_H__
