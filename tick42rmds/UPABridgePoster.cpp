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

#include "UPABridgePoster.h"
#include "inbox.h"
#include "UPAMamaCommonFields.h"
#include "RMDSPublisher.h"
#include "RMDSPublisherSource.h"
#include "UPAPublisherItem.h"
#include "UPAProvider.h"
#include "UPAFieldEncoder.h"

// post message buffer size
const RsslUInt32 postMsgSize = 4096;

static RsslInt32 nextNIStreamId = -2;

UPABridgePublisher::UPABridgePublisher(const string & root, const string& sourceName, const string& symbol, mamaTransport transport, void * queue, mamaPublisher parent )
   :root_(root), sourceName_(sourceName), symbol_(symbol), transport_(transport), nativeQueue_(queue), parent_(parent)
{
}

UPABridgePublisher::~UPABridgePublisher()
{
}

bool UPABridgePublisher::Initialise()
{
   return true;
}

/* Build up the RV subject. This should only need to be set once for the
publisher. Duplication of some of the logic  */
mama_status UPABridgePublisher::BuildSendSubject()
{
   char lSubject[256];
   if (!root_.empty() && !sourceName_.empty())
   {
      snprintf (lSubject, sizeof(lSubject), "%s.%s", root_.c_str(), sourceName_.c_str());
   }
   else if (!sourceName_.empty() && !symbol_.empty())
   {
      snprintf (lSubject, sizeof(lSubject), "%s.%s", sourceName_.c_str(), symbol_.c_str());
   }
   else if (!symbol_.empty())
   {
      snprintf (lSubject, sizeof(lSubject), "%s", symbol_.c_str());
   }

   subject_ = lSubject;
   return MAMA_STATUS_OK;
}

mama_status UPABridgePublisher::PublishMessage(mamaMsg msg)
{
   return MAMA_STATUS_OK;
}

mama_status UPABridgePublisher::PublishMessage(mamaMsg msg, PublisherPostMessageReply * reply)
{
   return MAMA_STATUS_OK;
}

// factory
UPABridgePublisher_ptr_t UPABridgePoster::CreatePoster(const string & root, const string& sourceName,
   const string& symbol, mamaTransport transport, void * queue, mamaPublisher parent,
   RMDSSubscriber_ptr_t subscriber,TransportConfig_t config )
{
   UPABridgePoster* newPoster = new UPABridgePoster(root, sourceName, symbol, transport, queue, parent, subscriber);
   UPABridgePoster_ptr_t p = UPABridgePoster_ptr_t(newPoster);
   UPABridgePublisher_ptr_t ret = p;

   // set the pointer
   newPoster->Initialise(p, config);

   return ret;
}

UPABridgePoster::UPABridgePoster(const string & root, const string& sourceName, const string& symbol,
      mamaTransport transport, void * queue, mamaPublisher parent,  RMDSSubscriber_ptr_t subscriber)

   : UPABridgePublisher(root, sourceName, symbol, transport, queue, parent)
   , subscriber_(subscriber), gotServiceId_(false), lastMsgStatus_(MAMA_MSG_STATUS_OK)
{
}

UPABridgePoster::~UPABridgePoster()
{
}

void UPABridgePoster::Shutdown()
{
   sharedPtr_.reset();
}

bool UPABridgePoster::Initialise(UPABridgePoster_ptr_t shared_ptr, TransportConfig_t config)
{
   sharedPtr_ = shared_ptr;

   // Should we copy the Mama seqNum field?
   useSeqNum_ = config.getBool("useseqnum", Default_UseSeqNum);

   // Should we post on-stream or off-stream?
   alwaysOnStream_ = config.getString("onstreampost", Default_OnStreamPost);
   if ((0 != strcasecmp(AlwaysOnStream(), "off"))    // If it's not forcibly-disabled ...
      && subscriber_->FindSubscription(sourceName_, symbol_, onStreamSubscription_)) // ... and we can find a stream ...
   {
      // we have a subscriber that we can use for on-stream publishing
      // take the stream id from it
      streamId_ = onStreamSubscription_->StreamId();
      postOnStream_ = true;
   }
   else
   {
      // Either we have disabled on-stream posting or we couldn't
      // find a streamId to use, so we do an off-stream post
      streamId_ = subscriber_->Consumer()->LoginStreamId();
      postOnStream_ = false;
   }

   // set up dictionaries and field map references
   upaFieldMap_ = subscriber_->FieldMap();
   mamaDictionary_ = upaFieldMap_->GetCombinedMamaDictionary().get();
   rmdsDictionary_ = subscriber_->Consumer()->RsslDictionary()->RsslDictionary();

   return true;
}

mama_status UPABridgePoster::PublishMessage( mamaMsg msg )
{
   if (subscriber_->GetState() != RMDSSubscriber::live )
   {
      return MAMA_STATUS_OK;
   }

   if (!gotServiceId_)
   {
      serviceId_ = subscriber_->GetServiceId(sourceName_);
      gotServiceId_ = true;
   }

#if 0
   // need to dispatch onto consumer thread
   PostMessageRequest * pubMsg = new PostMessageRequest(sharedPtr_, msg);
   RequestPostMessage(subscriber_->Consumer()->RequestQueue(), pubMsg);
#else
   // short circuit  the queue and call directly
   bool bret = DoPostMessage(msg, 0);
   if (bret == false) return MAMA_STATUS_WRONG_FIELD_TYPE;
#endif

   return MAMA_STATUS_OK;
}

mama_status UPABridgePoster::PublishMessage( mamaMsg msg, PublisherPostMessageReply * reply)
{
   if (subscriber_->GetState() != RMDSSubscriber::live )
   {
      return MAMA_STATUS_OK;
   }

   if (!gotServiceId_)
   {
      serviceId_ = subscriber_->GetServiceId(sourceName_);
      gotServiceId_ = true;
   }

#if 0
   // need to dispatch onto consumer thread
   PostMessageRequest * pubMsg = new PostMessageRequest(sharedPtr_, msg, reply);
   RequestPostMessage(subscriber_->Consumer()->RequestQueue(), pubMsg);
#else
   // short circuit  the queue and call directly
   bool bret = DoPostMessage(msg, reply);
   if (bret == false) return MAMA_STATUS_WRONG_FIELD_TYPE;
#endif

   return MAMA_STATUS_OK;
}

// request post message
void MAMACALLTYPE UPABridgePoster::PostMessageRequestCb(mamaQueue queue, void * closure)
{
   PostMessageRequest * req = (PostMessageRequest*) closure;

   UPABridgePoster_ptr_t poster = req->Poster();
   PublisherPostMessageReply * reply = req->Reply();

   bool bret = poster->DoPostMessage(req->Message(), reply);
}

bool UPABridgePoster::RequestPostMessage(mamaQueue requestQueue, PostMessageRequest * req)
{
   mamaQueue_enqueueEvent(requestQueue, UPABridgePoster::PostMessageRequestCb, (void *) req);
   return true;
}

bool UPABridgePoster::DoPostMessage(mamaMsg msg, PublisherPostMessageReply * reply)
{
   RsslChannel *chnl = subscriber_->Consumer()->RsslConsumerChannel();
   RsslError err;
   RsslRet ret = RSSL_RET_FAILURE;

   mamaMsgStatus msgStatus;
   mama_i32_t stat = 0;
   mamaMsg_getI32(msg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, &stat);
   bool statusChange = ((msgStatus = (mamaMsgStatus)stat) != lastMsgStatus_);

   // If the new status is stale, mark the item before we send the stale data
   if (statusChange && (lastMsgStatus_ == MAMA_MSG_STATUS_OK))
   {
      // Prevent the code below from sending another status message, because lastMsgStatus_
      // will be altered by the call PostStatusMessage()
      statusChange = false;

      if ((ret = PostStatusMessage(chnl, msgStatus, msg)) != RSSL_RET_SUCCESS)
      {
         // Failed to post the status message
         return false;
      }
   }

   // Now go on to process the refresh/update as normal
   RsslBuffer *rsslMessageBuffer = rsslGetBuffer(chnl, postMsgSize, RSSL_FALSE, &err);

   if (rsslMessageBuffer == 0)
   {
      t42log_warn("Unable to obtain RsslBuffer to post message for %s : %s - error code is %d (%s) \n",
         sourceName_.c_str(), symbol_.c_str(), err.rsslErrorId, err.text);
      return false;
   }

   // Build and send the rssl message
   bool bret = BuildPostMessage(chnl, rsslMessageBuffer, msg, reply);
   if (bret == false) return false;

   SendUPAMessage(chnl, rsslMessageBuffer);

   // If the status changed and we didn't post a status message above (because the new status is OK),
   // we should send the new status now
   if (statusChange
      && (lastMsgStatus_ != MAMA_MSG_STATUS_OK)
      && ((ret = PostStatusMessage(chnl, msgStatus, msg)) != RSSL_RET_SUCCESS))
   {
      // Failed to post the status message
      return false;
   }

   return true;
}

bool UPABridgePoster::BuildPostMessage(RsslChannel * chnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg,
   PublisherPostMessageReply * reply )
{
   // Message structure that the rssl encoder uses
   RsslPostMsg rsslMsg = RSSL_INIT_POST_MSG;

   // for the moment assume market price
   rsslMsg.msgBase.msgClass = RSSL_MC_POST;
   rsslMsg.msgBase.streamId = streamId_;
   rsslMsg.msgBase.domainType = RSSL_DMT_MARKET_PRICE;
   rsslMsg.msgBase.containerType = RSSL_DT_MSG;

   rsslMsg.flags = RSSL_PSMF_POST_COMPLETE 
      | RSSL_PSMF_ACK // request ACK
      | RSSL_PSMF_HAS_POST_ID
      | RSSL_PSMF_HAS_POST_USER_RIGHTS;

   // we will add a message key for both on and off-stream
   if (!postOnStream_)
   {
      // need to add a message key for off-stream posts to carry the source and symbol identity
      rsslMsg.flags |= RSSL_PSMF_HAS_MSG_KEY;

      rsslMsg.msgBase.msgKey.flags = RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_SERVICE_ID ; //| RSSL_MKF_HAS_IDENTIFIER;
      rsslMsg.msgBase.msgKey.name.data = const_cast<char *>(symbol_.c_str());
      rsslMsg.msgBase.msgKey.name.length = (RsslUInt32)symbol_.size();
      rsslMsg.msgBase.msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
      rsslMsg.msgBase.msgKey.serviceId = (RsslUInt16)serviceId_;
      //rsslMsg.msgBase.msgKey.identifier = pubId_;
   }

   // postid

   RsslUInt32 postId = subscriber_->Consumer()->PostManager().AddPost(sharedPtr_, reply);
   rsslMsg.postId = postId;

   if (UseSeqNum())
   {
      // get the seqnum from mamamsg
      mama_i32_t  seqNum = 0;
	  if (mamaMsg_getI32(msg, MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid,  &seqNum) == MAMA_STATUS_OK)
      {
         rsslMsg.flags |= RSSL_PSMF_HAS_SEQ_NUM;
         rsslMsg.seqNum = (RsslUInt32)seqNum;
      }
   }

   // populate post user info 
   RsslBuffer hostName = RSSL_INIT_BUFFER;
   char hostNameBuf[256];
   hostName.data = hostNameBuf;
   hostName.length = 256;
   gethostname(hostName.data, hostName.length);
   hostName.length = (RsslUInt32)strlen(hostName.data);
   RsslRet ret = 0;
   if ((ret = rsslHostByName(&hostName, &rsslMsg.postUserInfo.postUserAddr)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("Populating postUserInfo for %s : %s failed. Error %s (%d) with rsslHostByName: %s\n",
         sourceName_.c_str(), symbol_.c_str(), rsslRetCodeToString(ret), ret, rsslRetCodeInfo(ret));
      return false;
   }
   rsslMsg.postUserInfo.postUserId = getpid();

   // encode the message 
   RsslEncodeIterator itEncode = RSSL_INIT_ENCODE_ITERATOR;
   if ((ret = rsslSetEncodeIteratorBuffer(&itEncode, rsslMessageBuffer)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("rsslEncodeIteratorBuffer()  for %s : %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), ret);
      return false;
   }
   rsslSetEncodeIteratorRWFVersion(&itEncode, chnl->majorVersion, chnl->minorVersion);

   // encode the message header
   if ((ret = rsslEncodeMsgInit(&itEncode, (RsslMsg*)&rsslMsg, 0)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("rsslEncodeMsgInit() for %s : %s  failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), ret);
      return false;
   }

   // going to  encode the message data into a sub message
   RsslBuffer payloadMsgBuf = RSSL_INIT_BUFFER;
   ret = rsslEncodeNonRWFDataTypeInit(&itEncode, &payloadMsgBuf);
   if (ret != RSSL_RET_SUCCESS)
   {
      t42log_warn("encodePostWithMsg: rsslEncodeNonRWFDataTypeInit() for  %s : %s  failed\n", sourceName_.c_str(), symbol_.c_str());
      return false;
   }

   // now we can encode the set of fields from the mama msg
   bool bret = EncodeMessageData( chnl, &payloadMsgBuf, msg);
   if (bret == false)
   {
   	  return false;
   }

   // and wrap it up
   ret = rsslEncodeNonRWFDataTypeComplete(&itEncode, &payloadMsgBuf, RSSL_TRUE);
   if (ret != RSSL_RET_SUCCESS)
   {
      t42log_warn("encodePostWithMsg: rsslEncodeNonRWFDataTypeInit() for  %s : %s failed\n", sourceName_.c_str(), symbol_.c_str());
      return false;
   }

   /* complete encode message */
   if ((ret = rsslEncodeMsgComplete(&itEncode, RSSL_TRUE)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("rsslEncodeMsgComplete() failed  for  %s : %s with return code: %d\n",  sourceName_.c_str(), symbol_.c_str(), ret);
      return false;
   }

   rsslMessageBuffer->length = rsslGetEncodedBufferLength(&itEncode);

   return true;
}

bool UPABridgePoster::EncodeMessageData(RsslChannel * chnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg)
{
   // first off is this an update or an initial
   // if nothing in mama msg type then treat as update
   RsslUInt32 msgType;
   if (mamaMsg_getI32(msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid,  (mama_i32_t*)&msgType) != MAMA_STATUS_OK)
   {
      msgType = MAMA_MSG_TYPE_UPDATE;
   }

   bool isRefresh = false;
   if (msgType == MAMA_MSG_TYPE_INITIAL || msgType == MAMA_MSG_TYPE_REFRESH)
   {
      // todo will have to also check for book message types but probably in different method
      isRefresh = true;
   }

   RsslUInt32 msgStatus;
   if (mamaMsg_getI32(msg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid,  (mama_i32_t*)&msgStatus) != MAMA_STATUS_OK)
   {
      msgStatus = MAMA_MSG_STATUS_POSSIBLY_STALE;
   }

   RsslRefreshMsg rsslRefreshMsg = RSSL_INIT_REFRESH_MSG;
   RsslUpdateMsg rsslUpdateMsg = RSSL_INIT_UPDATE_MSG;
   RsslMsgBase* rsslMsgBase;
   RsslMsg* rsslMsg;

   // the encode iterator
   RsslEncodeIterator itEncode;
   rsslClearEncodeIterator(&itEncode);

   // so if its a refresh message we need to set up some state and identity stuff
   if (isRefresh)
   {
      rsslMsgBase = &rsslRefreshMsg.msgBase;
      rsslMsgBase->msgClass = RSSL_MC_REFRESH;

      // todo see if we need support for non-streaming data
      // but otherwise we can say the stream is open and the data is there
      rsslRefreshMsg.state.streamState = RSSL_STREAM_OPEN;
      rsslRefreshMsg.state.dataState = (MAMA_MSG_STATUS_OK == msgStatus) ? RSSL_DATA_OK : RSSL_DATA_SUSPECT;
      rsslRefreshMsg.state.code = RSSL_SC_NONE;

      // todo may want to put something in the state text - is there a mama field for this?

      // if we re-use this code for Interactive provider then will need to add solicited flag
      rsslRefreshMsg.flags = RSSL_RFMF_HAS_MSG_KEY | RSSL_RFMF_REFRESH_COMPLETE | RSSL_RFMF_HAS_QOS | RSSL_RFMF_CLEAR_CACHE;

	  // we need to always include a msg key for a refresh because sources like the TR ATS require it to create new cache items
	  rsslMsgBase->msgKey.flags = RSSL_MKF_HAS_SERVICE_ID | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_NAME_TYPE;
	  // // ServiceId
	  rsslMsgBase->msgKey.serviceId = serviceId_;
	  //// Itemname
	  rsslMsgBase->msgKey.name.data = const_cast<char *>(symbol_.c_str());
	  rsslMsgBase->msgKey.name.length = (RsslUInt32)symbol_.size();
	  rsslMsgBase->msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;


      // 
      // Qos 
      rsslRefreshMsg.qos.dynamic = RSSL_FALSE;
      rsslRefreshMsg.qos.rate = RSSL_QOS_RATE_TICK_BY_TICK;
      rsslRefreshMsg.qos.timeliness = RSSL_QOS_TIME_REALTIME;

      rsslMsg = (RsslMsg *)&rsslRefreshMsg;
   }
   else
   {
      rsslMsgBase = &rsslUpdateMsg.msgBase;
      rsslMsgBase->msgClass = RSSL_MC_UPDATE;

      rsslMsg = (RsslMsg *)&rsslUpdateMsg;
   }

   //now we can build the rest of the message body
   rsslMsgBase->domainType = RSSL_DMT_MARKET_PRICE;
   rsslMsgBase->containerType = RSSL_DT_FIELD_LIST;

   // StreamId 
   rsslMsgBase->streamId = streamId_;

   //encode message 
   UPAFieldEncoder encoder(mamaDictionary_, upaFieldMap_, rmdsDictionary_, sourceName_, symbol_);
   return encoder.encode(msg, chnl, rsslMsg, rsslMessageBuffer);
}

void UPABridgePoster::PrintMsg(RsslBuffer* buffer)
{
	RsslRet ret = 0;
	RsslChannel *chnl = subscriber_->Consumer()->RsslConsumerChannel();
	RsslDecodeIterator dIter;
	RsslMsg msg = RSSL_INIT_MSG;
	RsslFieldList fList = RSSL_INIT_FIELD_LIST;
	RsslFieldEntry fEntry = RSSL_INIT_FIELD_ENTRY;

	rsslClearDecodeIterator(&dIter);
 	rsslSetDecodeIteratorRWFVersion(&dIter, chnl->majorVersion, chnl->minorVersion);
	rsslSetDecodeIteratorBuffer(&dIter, buffer);

	rsslDecodeMsg(&dIter, &msg);				
	rsslDecodeFieldList(&dIter, &fList, 0);

	t42log_info("--- RSSL BUFFER ---");
	while ((ret = rsslDecodeFieldEntry(&dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
	{
		printf("RSSL: %d\n", fEntry.fieldId);
	}
}

RsslRet UPABridgePoster::ProcessAck(RsslMsg* msg, RsslDecodeIterator* dIter,  PublisherPostMessageReply * reply)
{
   bool nak = false;
   string nakText;
   RsslUInt8 nakCode = RSSL_NAKC_NONE;
   const CommonFields &commonFields = UpaMamaCommonFields::CommonFields();

   // see if we have a nak code and extract it
   if (  rsslAckMsgCheckHasNakCode(&msg->ackMsg))
   {
      // have a NAK code

      nakCode = msg->ackMsg.nakCode;
      if (nakCode != RSSL_NAKC_NONE)
      {

         nak = true;
         // this is a nak so log as warning 
         if (rsslAckMsgCheckHasText(&msg->ackMsg))
         {
            nakText = string( msg->ackMsg.text.data, msg->ackMsg.text.length);
         }
      }
   }

   if (reply != 0 && reply->Inbox() != 0)
   {
      // build a message for the inbox		
      mamaMsg msg;
      mamaMsg_createForPayload(&msg, MAMA_PAYLOAD_TICK42RMDS);
	  mamaMsg_addI32(msg,MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_SEC_STATUS);

      mamaMsgStatus status = MAMA_MSG_STATUS_OK;
      if (nakCode != RSSL_NAKC_NONE)
      {
         status = RsslNakCode2MamaMsgStatus(nakCode);
      }

	  mamaMsg_addI32(msg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, status);
	  mamaMsg_addString(msg, commonFields.wIssueSymbol.mama_field_name.c_str(), commonFields.wIssueSymbol.mama_fid, symbol_.c_str());
	  mamaMsg_addString(msg, commonFields.wSymbol.mama_field_name.c_str(), commonFields.wSymbol.mama_fid, symbol_.c_str());

      rmdsMamaInbox_send(reply->Inbox(), msg);

      mamaMsg_destroy(msg);
   }
   if (  rsslAckMsgCheckHasNakCode(&msg->ackMsg))
   {
      // have a NAK code
      if (msg->ackMsg.nakCode != RSSL_NAKC_NONE)
      {
         nak = true;
         // this is a nak so log as warning 
         string nakText;

         if (rsslAckMsgCheckHasText(&msg->ackMsg))
         {
            nakText = string( msg->ackMsg.text.data, msg->ackMsg.text.length);
         }

         t42log_warn("Received NAK for postid %d for %s : %s - NAK code is %d (%s)\n", msg->ackMsg.ackId, sourceName_.c_str(), symbol_.c_str(), msg->ackMsg.nakCode, nakText.c_str());
      }
   }

   if ( ! nak)
   {
      // was an ACK

      t42log_debug("Received ACK for postid %d for %s : %s \n", msg->ackMsg.ackId, sourceName_.c_str(), symbol_.c_str());
   }

   return RSSL_RET_SUCCESS;
}

mamaMsgStatus UPABridgePoster::RsslNakCode2MamaMsgStatus(RsslUInt8 nakCode)
{
   // convert from nak code to mama status
   mamaMsgStatus ret = MAMA_MSG_STATUS_UNKNOWN;

   switch (nakCode)
   {
   case RSSL_NAKC_NONE:			 	//! No Nak Code
      ret = MAMA_MSG_STATUS_OK;
      break;
   case RSSL_NAKC_ACCESS_DENIED:  	// Access Denied. The user not properly permissioned for posting on the item or service. 
      ret = MAMA_MSG_STATUS_NOT_ENTITLED;
      break;
   case RSSL_NAKC_DENIED_BY_SRC:  	// Denied by source. The source being posted to has denied accepting this post message. 
      ret = MAMA_MSG_STATUS_NOT_PERMISSIONED;
      break;
   case RSSL_NAKC_SOURCE_DOWN:    	// Source Down. Source being posted to is down or not available. 
   case RSSL_NAKC_GATEWAY_DOWN:	   // Gateway is Down. The gateway device for handling posted or contributed information is down or not available. 
      ret = MAMA_MSG_STATUS_LINE_DOWN;
      break;

   case RSSL_NAKC_SOURCE_UNKNOWN: 	// Source Unknown. The source being posted to is unknown and is unreachable. 
   case RSSL_NAKC_SYMBOL_UNKNOWN: 	// Unknown Symbol. The item information provided within the post message is not recognized by the system.
      // treat both of these together
      ret = MAMA_MSG_STATUS_BAD_SYMBOL;
      break;

   case RSSL_NAKC_NOT_OPEN:       	// Item not open. The item being posted to does not have an available stream open. 
      // might not the the intended semantics os this messsage status but seems to match quite well
      ret = MAMA_MSG_STATUS_NO_SUBSCRIBERS;
      break;

   case RSSL_NAKC_NO_RESOURCES:   	// No Resources. A component along the path of the post message does not have appropriate resources available to continue processing the post. 
   case RSSL_NAKC_NO_RESPONSE:    	// No Response. This code may mean that the source is unavailable or there is a delay in processing the posted information. 
   case RSSL_NAKC_INVALID_CONTENT:	// Nak being sent due to invalid content. The content of the post message is invalid and cannot be posted, it does not match the expected formatting for this post. 
      // no direct equivalent here
      ret = MAMA_MSG_STATUS_PLATFORM_STATUS;
      break;


   default:
      t42log_warn("Received unknown NAK code (%d) for :s : %s \n", nakCode, sourceName_.c_str(), symbol_.c_str());
      break;

   } /*RsslNakCodes*/

   return ret;
}

UPABridgePublisherItem_ptr_t UPABridgePublisherItem::CreatePublisherItem(
   const string & root, const string& sourceName, const string& symbol, mamaTransport transport, void * queue, 
   mamaPublisher parent, RMDSPublisherBase_ptr_t RMDSPublisher, TransportConfig_t config, bool interactive )
{

   UPABridgePublisherItem* newItem = new UPABridgePublisherItem(root, sourceName, symbol, transport, queue, parent, RMDSPublisher );
   UPABridgePublisherItem_ptr_t p = UPABridgePublisherItem_ptr_t(newItem);
   UPABridgePublisherItem_ptr_t ret = p;

   // set the pointer
   newItem->Initialise(p, config, interactive);

   return ret;
}

void MAMACALLTYPE UPABridgePublisherItem::InboxOnMessageCB(mamaMsg msg, void *closure)
{
   ((UPABridgePublisherItem*)closure)->InboxOnMessage(msg);
}

bool UPABridgePublisherItem::Initialise(UPABridgePublisherItem_ptr_t shared_ptr, TransportConfig_t config, bool interactive)
{
   sharedPtr_ = shared_ptr;

   RMDSPublisherSource *source = RMDSPublisher_->GetSource();

   if (source == 0)
   {
      t42log_error("Failed to look up source %s\n", sourceName_.c_str());
      return false;
   }

   // Now find the UPA item 
   UPAItem_ = source->GetItem(symbol_);

   if (UPAItem_.get() == 0)
   {

	   if (!interactive)
	   {
		   // should add it to the source straight away
		   // this will happen with the NI publisher so we can use that to get at the channel.
		   // (whith an interactive publisher this will get handled by the new item request processing)
		   bool isNew;
		   UPAItem_ = source->AddItem(RMDSPublisher_->GetChannel() ,nextNIStreamId--, sourceName_, symbol_, source->ServiceId(), RMDSPublisher_.get(), isNew);
	   }
	   else
	   {
		   // if its interactive then we can create it with a null channel and stream id and fiox it up later when we get a new item request
		   bool isNew;
		   UPAItem_ = source->AddItem(0 ,0, sourceName_, symbol_, source->ServiceId(), RMDSPublisher_.get(), isNew);

	   }

   }

   return true;
}

void UPABridgePublisherItem::InboxOnMessage( mamaMsg msg )
{

}

void UPABridgePublisherItem::ProcessRecapMessage( mamaMsg msg )
{

   UPAItem_->ProcessRecapMessage(msg);
}


// request post message
void MAMACALLTYPE UPABridgePublisherItem::PublishMessageRequestCb(mamaQueue queue, void * closure)
{
   PublishMessageRequest * req = (PublishMessageRequest*) closure;

   UPABridgePublisherItem_ptr_t pub = req->Publisher();

   pub->DoPublishMessage(req->Message());
}

bool UPABridgePublisherItem::RequestPublishMessage(mamaQueue requestQueue, PublishMessageRequest * req)
{
   mamaQueue_enqueueEvent(requestQueue, UPABridgePublisherItem::PublishMessageRequestCb, (void *) req);

   return true;
}


bool UPABridgePublisherItem::DoPublishMessage(mamaMsg msg)
{
   UPAItem_->PublishMessage(msg);
   return true;
}

mama_status UPABridgePublisherItem::PublishMessage( mamaMsg msg )
{
   // just publish the message on this thread
   DoPublishMessage(msg);
   return MAMA_STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::PostStatusMessage(RsslChannel *chnl, mamaMsgStatus msgStatus, mamaMsg msg)
{
   RsslMsg rsslMsg;
   rsslClearMsg(&rsslMsg);
   RsslPostMsg &rsslPostMsg = rsslMsg.postMsg;

   // for the moment assume market price
   rsslPostMsg.msgBase.msgClass = RSSL_MC_POST;
   rsslPostMsg.msgBase.streamId = streamId_;
   rsslPostMsg.msgBase.domainType = RSSL_DMT_MARKET_PRICE;
   rsslPostMsg.msgBase.containerType = RSSL_DT_MSG;

   rsslPostMsg.flags = RSSL_PSMF_POST_COMPLETE 
      | RSSL_PSMF_ACK // request ACK
      | RSSL_PSMF_HAS_POST_ID;

   // we will add a message key for both on and off-stream
   if (!postOnStream_)
   {
      // need to add a message key for off-stream posts to carry the source and symbol identity
      rsslPostMsg.flags |= RSSL_PSMF_HAS_MSG_KEY;

      rsslPostMsg.msgBase.msgKey.flags = RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_SERVICE_ID ; //| RSSL_MKF_HAS_IDENTIFIER;
      rsslPostMsg.msgBase.msgKey.name.data = const_cast<char *>(symbol_.c_str());
      rsslPostMsg.msgBase.msgKey.name.length = (RsslUInt32)symbol_.size();
      rsslPostMsg.msgBase.msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
      rsslPostMsg.msgBase.msgKey.serviceId = (RsslUInt16)serviceId_;
   }

   // postid

   PublisherPostMessageReply *reply = 0;
   RsslUInt32 postId = subscriber_->Consumer()->PostManager().AddPost(sharedPtr_, reply);
   rsslPostMsg.postId = postId;

   // get the seqnum from mamamsg
   mama_i32_t seqNum = 0;
   if (mamaMsg_getI32(msg, MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, &seqNum) == MAMA_STATUS_OK)
   {
      rsslPostMsg.flags |= RSSL_PSMF_HAS_SEQ_NUM;
      rsslPostMsg.seqNum = (RsslUInt32)seqNum;
   }

   // populate post user info 
   RsslRet ret = 0;
   if ((ret = SetPostUserInfo(rsslPostMsg.postUserInfo)) != RSSL_RET_SUCCESS)
   {
      return ret;
   }
   rsslPostMsg.flags |= RSSL_STMF_HAS_POST_USER_INFO;

   // encode the message 
   RsslBuffer *buffer;
   RsslEncodeIterator itEncode;
   rsslClearEncodeIterator(&itEncode);
   if ((buffer = AllocateEncoder(chnl, itEncode)) == 0)
   {
      return ret;
   }

   // encode the message header
   if ((ret = StartEncoding(itEncode, &rsslMsg)) != RSSL_RET_SUCCESS)
   {
      RsslError err;
      rsslReleaseBuffer(buffer, &err);
      return ret;
   }

   // now we can encode the status message itself
   if ((ret = EncodeStatusMessage(itEncode, msgStatus, msg)) != RSSL_RET_SUCCESS)
   {
      RsslError err;
      rsslReleaseBuffer(buffer, &err);
      return ret;
   }
   
   if ((ret = FinishEncoding(itEncode)) != RSSL_RET_SUCCESS)
   {
      RsslError err;
      rsslReleaseBuffer(buffer, &err);
      return ret;
   }

   // The encoding worked, so we can set the final length
   buffer->length = rsslGetEncodedBufferLength(&itEncode);

   // And post it
   SendUPAMessage(chnl, buffer);
   return ret;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::EncodeStatusMessage(RsslEncodeIterator &itEncode,
   mamaMsgStatus msgStatus, mamaMsg msg)
{
   RsslRet ret = 0;
   RsslMsg rsslMsg;
   RsslStatusMsg &rsslStatus = rsslMsg.statusMsg;
   rsslClearStatusMsg(&rsslStatus);

   /* set-up message, assuming Market price for the moment (as per BuildPostMessage()) */
   rsslStatus.msgBase.msgClass = RSSL_MC_STATUS;
   rsslStatus.msgBase.streamId = streamId_;
   rsslStatus.msgBase.domainType = RSSL_DMT_MARKET_PRICE;
   rsslStatus.msgBase.containerType = RSSL_DT_NO_DATA;

   // Set the state flags and text
   if ((ret = SetState(rsslStatus.state, msgStatus, msg)) != RSSL_RET_SUCCESS)
   {
      return ret;
   }
   rsslStatus.flags |= RSSL_STMF_HAS_STATE;

   // Add the msg key that encodes the service/symbol for off-stream messages
   if ((ret = SetMsgKey(rsslStatus.msgBase.msgKey)) != RSSL_RET_SUCCESS)
   {
      return ret;
   }
   rsslStatus.flags |= RSSL_STMF_HAS_MSG_KEY;

   if ((ret = StartEncoding(itEncode, &rsslMsg)) == RSSL_RET_SUCCESS)
   {
      ret = FinishEncoding(itEncode);
   }

   return ret;
}

//////////////////////////////////////////////////////////////////////////
//
RsslBuffer *UPABridgePoster::AllocateEncoder(RsslChannel *chnl, RsslEncodeIterator &itEncode)
{
   RsslRet ret;
   RsslError err;
   RsslBuffer *buffer;
   if ((buffer = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &err)) == 0)
   {
      t42log_warn("[%s:%s]: StartEncoding() failed to get buffer for posting, error %s(%d)",
         sourceName_.c_str(), symbol_.c_str(), err.text, err.rsslErrorId);
   }
   else
   {
      if ((ret = rsslSetEncodeIteratorBuffer(&itEncode, buffer)) < RSSL_RET_SUCCESS)
      {
         t42log_warn("[%s:%s]: rsslEncodeIteratorBuffer() failed with error %s(%d)",
            sourceName_.c_str(), symbol_.c_str(), rsslRetCodeToString(ret), ret);
         rsslReleaseBuffer(buffer, &err);
         return 0;
      }
      rsslSetEncodeIteratorRWFVersion(&itEncode, chnl->majorVersion, chnl->minorVersion);
   }
   return buffer;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::StartEncoding(RsslEncodeIterator &itEncode, RsslMsg *rsslMsg)
{
   RsslRet ret;
   if ((ret = rsslEncodeMsgInit(&itEncode, rsslMsg, 0)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("[%s:%s]: rsslEncodeMsgInit() failed with error %s(%d)",
         sourceName_.c_str(), symbol_.c_str(), rsslRetCodeToString(ret), ret);
      return ret;
   }
   return RSSL_RET_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::FinishEncoding(RsslEncodeIterator &itEncode)
{
   /* complete encode message */
   RsslRet ret;
   if ((ret = rsslEncodeMsgComplete(&itEncode, RSSL_TRUE)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("[%s:%s]: rsslEncodeMsgComplete() failed with error %s(%d)",
         sourceName_.c_str(), symbol_.c_str(), rsslRetCodeToString(ret), ret);
      return ret;
   }
   return RSSL_RET_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::SetState(RsslState &state, mamaMsgStatus msgStatus, mamaMsg msg)
{
   statusText_ = mamaMsgStatus_stringForStatus(msgStatus);

   //!!! Debug code to include the sequence number in the status text
   // so that we may work out why the output from the provider does not seem
   // to be in synch
   mama_i32_t seqNum = 0;
   if (mamaMsg_getI32(msg, MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, &seqNum) == MAMA_STATUS_OK)
   {
      char buffer[16];
      snprintf(&buffer[0], sizeof(buffer) / sizeof(buffer[0]), " (%d)", seqNum);
      statusText_.append(&buffer[0]);
   }

   state.text.data = const_cast<char *>(statusText_.c_str());
   state.text.length = (RsslUInt32)statusText_.length();
   state.streamState = RSSL_STREAM_OPEN;
   state.dataState =
      (lastMsgStatus_ == msgStatus) ? RSSL_DATA_NO_CHANGE
      : (MAMA_MSG_STATUS_OK == msgStatus) ? RSSL_DATA_OK
      : RSSL_DATA_SUSPECT;
   state.code = RSSL_SC_NONE;
   lastMsgStatus_ = msgStatus;
   return RSSL_RET_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::SetMsgKey(RsslMsgKey &msgKey)
{
   msgKey.flags = RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_SERVICE_ID;
   msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
   msgKey.name.data = const_cast<char *>(symbol_.c_str());
   msgKey.name.length = (RsslUInt32)symbol_.length();
   msgKey.serviceId = serviceId_;
   return RSSL_RET_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
RsslRet UPABridgePoster::SetPostUserInfo(RsslPostUserInfo &postUserInfo)
{
   RsslRet ret = RSSL_RET_FAILURE;
   RsslBuffer hostName = RSSL_INIT_BUFFER;

   // The host name is cached by the UPALogin object for performance
   hostName.data = const_cast<char *>(UPALogin::GetHostName(&hostName.length));
   if ((ret = rsslHostByName(&hostName, &postUserInfo.postUserAddr)) < RSSL_RET_SUCCESS)
   {
      t42log_warn("[%s:%s]: populating postUserInfo failed with error %s(%d), rsslRetCodeInfo=%s",
         sourceName_.c_str(), symbol_.c_str(), rsslRetCodeToString(ret), ret, rsslRetCodeInfo(ret));
      return ret;
   }
   postUserInfo.postUserId = getpid();
   return RSSL_RET_SUCCESS;
}
