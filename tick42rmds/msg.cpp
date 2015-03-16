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

#include "msg.h"

// Message types 

typedef struct 
{
	std::string source_;
	std::string symbol_;
} RMDSBridgeMsgReplyHandle_t;

typedef struct 
{
	mamaMsg                     parent_;
	RMDSBridgeMsgType_t         msgType_;
	uint8_t                     isValid_;
	std::string					sendSubject_;

	RMDSBridgeMsgReplyHandle_t    replyHandle_;

} RMDSBridgeMsgImpl_t;

 
  /*=========================================================================
   =                        Recommended Functions                           =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaMsg_create (msgBridge* msg, mamaMsg parent)
 {
	 RMDSBridgeMsgImpl_t * impl = NULL;

	 if (NULL == msg)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }

	 /* Null initialize the msgBridge pointer */
	 *msg = NULL;

	 /* Allocate memory for the implementation struct */
	 impl = new RMDSBridgeMsgImpl_t();

	 if (NULL == impl)
	 {
		 t42log_error( "tick42rmdsBridgeMamaMsg_create (): Failed to allocate memory for bridge message.\n");
		 return MAMA_STATUS_NOMEM;
	 }

	 /* Back reference the parent message */
	 impl->parent_       = parent;
	 impl->isValid_      = 1;

	 impl->replyHandle_.source_="";
	 impl->replyHandle_.symbol_ = "";
	 impl->sendSubject_ = "";

	 /* Populate the msgBridge pointer with the implementation */
	 *msg = (msgBridge) impl;

	 return MAMA_STATUS_OK;
 }

 
 mama_status
 tick42rmdsBridgeMamaMsg_destroy (msgBridge msg, int destroyMsg)
 {
	 if (NULL == msg)
	 {
		 return MAMA_STATUS_INVALID_ARG;
	 }
	 /* Free the underlying implementation */
	 delete (RMDSBridgeMsgImpl_t*)msg;

	 return MAMA_STATUS_OK;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_destroyMiddlewareMsg (msgBridge msg)
 {

     // The bridge message is never responsible for the memory associated with
     // the underlying middleware message (it's owned by publishers and
     // transports) so no need to do anything here
     
    return MAMA_STATUS_OK;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_detach (msgBridge msg)
 {

      //The bridge message is never responsible for the memory associated with
      //the underlying middleware message (it's owned by publishers and
      //transports) so no need to do anything here

    return MAMA_STATUS_OK;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_getPlatformError (msgBridge msg, void** error)
 {

	 if (NULL != error)
	 {
		 *error  = NULL;
	 }
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsgImpl_setReplyHandle (msgBridge msg, void* result)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsgImpl_setReplyHandleAndIncrement (msgBridge msg, void* result)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 int
 tick42rmdsBridgeMamaMsg_isFromInbox (msgBridge msg)
 {
	 if (NULL == msg)
	 {
		 return -1;
	 }
	 if (RMDS_MSG_INBOX_REQUEST == ((RMDSBridgeMsgImpl_t*)msg)->msgType_ || RMDS_MSG_PUB_NEW_ITEM_REQUEST == ((RMDSBridgeMsgImpl_t*)msg)->msgType_ )
	 {
		 return 1;
	 }

	 return 0;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_setSendSubject (msgBridge  msg,
                                 const char* symbol,
                                 const char* subject)
 { 
	 RMDSBridgeMsgImpl_t* impl     = (RMDSBridgeMsgImpl_t*) msg;
	 mama_status        status   = MAMA_STATUS_OK;

	 if (NULL == impl || NULL == symbol || NULL == subject)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }

	  //Update the MAMA message with the send subject if it has a parent 
	 if (NULL != impl->parent_)
	 {
		 status = mamaMsg_updateString (impl->parent_,
			 MamaFieldSubscSymbol.mName,
			 MamaFieldSubscSymbol.mFid,
			 symbol);
	 }
	 return status;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_getNativeHandle (msgBridge msg, void** result)
 {
	 RMDSBridgeMsgImpl_t* impl = (RMDSBridgeMsgImpl_t*) msg;
	 if (NULL == impl || NULL == result)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }
	 *result = impl;
	 return MAMA_STATUS_OK;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_duplicateReplyHandle (msgBridge msg, void** result)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_copyReplyHandle (void* src, void** dest)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsg_destroyReplyHandle (void* result)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaMsgImpl_setAttributesAndSecure (msgBridge msg, void* attributes, uint8_t secure)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }


 // metadata functions
 mama_status tick42rmdsBridgeMamaMsgImpl_setMsgType (msgBridge msg, RMDSBridgeMsgType_t   type)
 {
	 RMDSBridgeMsgImpl_t*  impl   = (RMDSBridgeMsgImpl_t*) msg;

	 if (NULL == impl)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }
	 impl->msgType_ = type;
	 return MAMA_STATUS_OK;
 }

 mama_status tick42rmdsBridgeMamaMsgImpl_setReplyTo (msgBridge msg, std::string source, std::string symbol)
 {
	 RMDSBridgeMsgImpl_t *  impl        = (RMDSBridgeMsgImpl_t*) msg;

	 if (NULL == impl)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }

	 impl->replyHandle_.source_ = source;
	 impl->replyHandle_.symbol_ = symbol;

	 return MAMA_STATUS_OK;
 }

 mama_status tick42rmdsBridgeMamaMsgImpl_getReplyTo (msgBridge msg, std::string & source, std::string & symbol)
 {
	 RMDSBridgeMsgImpl_t *  impl        = (RMDSBridgeMsgImpl_t*) msg;

	 if (NULL == impl)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }


	 source = impl->replyHandle_.source_;
	 symbol = impl->replyHandle_.symbol_;

	 return MAMA_STATUS_OK;
 }

 mama_status tick42rmdsBridgeMamaMsgImpl_getMsgType( msgBridge msg, RMDSBridgeMsgType_t * type )
 {
	 RMDSBridgeMsgImpl_t*  impl   = (RMDSBridgeMsgImpl_t*) msg;

	 if (NULL == impl)
	 {
		 return MAMA_STATUS_NULL_ARG;
	 }

	 * type = impl->msgType_;
	 return MAMA_STATUS_OK;
 }

