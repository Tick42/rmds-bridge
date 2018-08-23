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
#include "UPAPublisherItem.h"
#include "UPAMessage.h"
#include "utils/t42log.h"
#include "RMDSPublisher.h"
#include "RMDSSubscriber.h"
#include "UPAFieldEncoder.h"
#include <utils/namespacedefines.h>

using namespace std;

const RsslUInt32 StateTextLen = 128;

utils::collection::unordered_set<int> suppressBadEnumWarnings_;

UPAPublisherItem::UPAPublisherItem(RsslChannel * chnl, RsslUInt32 streamId, const std::string& source, const std::string& symbol, RsslUInt32 serviceId)
   : source_(source), symbol_(symbol), serviceId_(serviceId), solicitedMessages_(true)
{
    // This gets created in one of 2 contexts -
    // either (a) handling a RSSL_MC_REQUEST message from the TREP, in which case it has a channel and stream id
    // or (b) because the mama client has chosen to create a publisher where there will be no channel/stream
    // in case (b) we just go ahead and create without setting up a channel. When eventually we receive a request for the item from
    // the TREP
    if (chnl != 0)
    {
       // don't have this channel so create one
       UpaChannel_t * upaChannel = new UpaChannel_t();
       upaChannel->channel_ = chnl;

       // and insert into the map
       channelMap_[chnl] = upaChannel;

       // now add the stream to the refresh list
       upaChannel->refreshStreamList_.push_back(streamId);
    }
}

UPAPublisherItem::~UPAPublisherItem()
{
   Shutdown();
}

bool UPAPublisherItem::Initialise( const UPAPublisherItem_ptr_t& ptr,  RMDSPublisherBase * publisher  )
{
   sharedptr_ = ptr;

   solicitedMessages_ = publisher->SolicitedMessages();

   // set up dictionaries and field map references
   upaFieldMap_ = publisher->FieldMap();
   mamaDictionary_ = upaFieldMap_->GetCombinedMamaDictionary().get();
   rmdsDictionary_ = publisher->Subscriber()->Consumer()->RsslDictionary()->RsslDictionary();
   maxMessageSize_ = publisher->MaxMessageSize();
   return true;
}

UPAPublisherItem_ptr_t UPAPublisherItem::CreatePublisherItem(RsslChannel * chnl, RsslUInt32 streamId, const std::string& source, const std::string& symbol, RsslUInt32 serviceId, RMDSPublisherBase * publisher)
{
   UPAPublisherItem * newItem  = new UPAPublisherItem(chnl, streamId, source, symbol, serviceId);
   UPAPublisherItem_ptr_t ret = UPAPublisherItem_ptr_t(newItem);
   newItem->Initialise(ret, publisher);

   return ret;
}

RsslRet UPAPublisherItem::SendItemRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslUInt8 domainType,
   RsslStateCodes code, const std::string& stateText,  RsslBool isPrivateStream, unsigned int maxMessageSize)
{
   RsslError error;

   // get a buffer for the item request reject status
   RsslBuffer* msgBuf = 0;
   msgBuf = rsslGetBuffer(chnl, maxMessageSize, RSSL_FALSE, &error);

   if (msgBuf != NULL)
   {
      RsslEncodeIterator encodeIter;

      RsslStatusMsg msg = RSSL_INIT_STATUS_MSG;
      // clear encode iterator
      rsslClearEncodeIterator(&encodeIter);

      /* set-up message */
      msg.msgBase.msgClass = RSSL_MC_STATUS;
      msg.msgBase.streamId = streamId;
      msg.msgBase.domainType = domainType;
      msg.msgBase.containerType = RSSL_DT_NO_DATA;
      msg.flags = RSSL_STMF_HAS_STATE;
      if (isPrivateStream)
      {
         msg.flags |= RSSL_STMF_PRIVATE_STREAM;
      }

      msg.state.streamState = RSSL_STREAM_CLOSED_RECOVER;
      msg.state.dataState = RSSL_DATA_SUSPECT;

      msg.state.text.data = const_cast<char *>(stateText.c_str());
      msg.state.text.length = (RsslUInt32)stateText.size();

      // encode message
      rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf);
      rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
      RsslRet ret = 0;
      if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&msg)) < RSSL_RET_SUCCESS)
      {
        RsslError err;
        rsslReleaseBuffer(msgBuf, &err);

        printf("rsslEncodeMsg() failed with return code: %d\n", ret);
        return ret;
      }

      msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

      t42log_info("Rejecting Item Request with streamId=%d and domain %s.  Reason: %s\n", streamId,  rsslDomainTypeToString(domainType), stateText.c_str());

      // send request reject status
      if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
         return RSSL_RET_FAILURE;
   }
   else
   {
      t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
      return RSSL_RET_FAILURE;
   }

   return RSSL_RET_SUCCESS;
}

void UPAPublisherItem::AddChannel( RsslChannel * chnl, RsslUInt32 streamId )
{
   // we don't expect a second stream to be added on a channel, but is is possible to configure UPA based components so that
   // this might happen.

    t42log_debug("Publisher item %s added channel 0x%x for stream id %d\n", symbol_.c_str(),  chnl, streamId);

   UpaChannel_t * upaChannel = 0;
   ChannelMap_t::iterator it = channelMap_.find(chnl);
   if (it == channelMap_.end())
   {
      // dont have this channel so create one
      upaChannel = new UpaChannel_t();
      upaChannel->channel_ = chnl;

      // and insert into the map
      channelMap_[chnl] = upaChannel;
   }
   else
   {
       upaChannel = it->second;
   }

   // now add the stream to the refresh list
   upaChannel->refreshStreamList_.push_back(streamId);
}


bool UPAPublisherItem::RemoveChannel( RsslChannel * chnl, RsslUInt32 streamId )
{
   UpaChannel_t * upaChannel = 0;
   ChannelMap_t::iterator chnlIt = channelMap_.find(chnl);
   if (chnlIt == channelMap_.end())
   {
      t42log_warn("attempt to remove unknown channel from %s : %s \n ", source_.c_str(), symbol_.c_str());
      return true;
   }

   upaChannel = chnlIt->second;
   // it is possible there is more than one stream on the list
   if (upaChannel->refreshStreamList_.size() == 0)
   {
      t42log_warn("attempt to remove  channel from %s : %s when there are no streams\n ", source_.c_str(), symbol_.c_str());
      return true;
   }

   // now walk the stream list (which probably only has one item)
   StreamList_t::iterator streamIt = upaChannel->refreshStreamList_.begin();
   while(streamIt != upaChannel->refreshStreamList_.end())
   {
      if (*streamIt == streamId)
      {
         upaChannel->refreshStreamList_.erase(streamIt);
         break;
      }
   }

   // now if there are no streams left on the channel we can remove the channel from this item
   if ( upaChannel->refreshStreamList_.size() == 0)
   {
      channelMap_.erase(chnlIt);
   }

   // now if there are no channels left we can close the item
   if (channelMap_.size() == 0)
   {
      // to do, shutdown this item

      // can shut down client item
      return false;
   }
   return true;
}

void UPAPublisherItem::ProcessRecapMessage( mamaMsg msg )
{
}

mama_status UPAPublisherItem::PublishMessage( mamaMsg msg, std::string& errorText )
{
   ChannelMap_t::iterator it = channelMap_.begin();

   if (it == channelMap_.end())
   {
      // if the last channel has been closed.
      // it looks like the client is still sending updates, just log and return
      t42log_debug("No Channels to publish message for %s : %s \n", source_.c_str(), symbol_.c_str());
      return MAMA_STATUS_NOT_INITIALISED;
   }


   t42log_debug("channel map for %s has %d entries\n ", symbol_.c_str(), channelMap_.size());

   // todo iterate this over all the channels
   UpaChannel_t *UpaChannel = it->second;
   RsslChannel *chnl = channelMap_.begin()->second->channel_;
   RsslError err;
   RsslBuffer *rsslMessageBuffer = rsslGetBuffer(chnl, maxMessageSize_, RSSL_FALSE, &err );

   if (rsslMessageBuffer == 0)
   {
      t42log_warn("Unable to obtain rssl buffer to post message for %s : %s - error code is %d (%s) \n",
         source_.c_str(), symbol_.c_str(), err.rsslErrorId, err.text);

      errorText.assign(err.text);

      return MAMA_STATUS_NOMEM;
   }

   if(!BuildPublishMessage(UpaChannel, rsslMessageBuffer, msg))
   {
       // Release buffer since we a re not going to send the message
       RsslError err;
       rsslReleaseBuffer(rsslMessageBuffer, &err);

       errorText.assign(err.text);

       return MAMA_STATUS_NOT_INITIALISED;
   }

   RsslRet rsslRet = SendUPAMessageWithErrorText(chnl, rsslMessageBuffer, errorText);
   if (rsslRet >= RSSL_RET_SUCCESS)
   {
       return MAMA_STATUS_OK;
   }

   return MAMA_STATUS_PLATFORM;
}

bool UPAPublisherItem::BuildPublishMessage( UpaChannel_t * chnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg )
{
   RsslMsg * rsslMsg;
   RsslMsgBase* msgBase;

   // need to work out whether this is refresh or update
   // first off is this an update or an initial
   // if nothing in mama msg type then treat as update
   RsslUInt32 msgType;
   if (!mamaMsg_getI32(msg,MamaFieldMsgType.mName, MamaFieldMsgType.mFid,  (mama_i32_t*)&msgType) == MAMA_STATUS_OK)
   {
      msgType = MAMA_MSG_TYPE_UPDATE;
   }

   bool isRefresh = false;
   if (msgType == MAMA_MSG_TYPE_INITIAL || msgType == MAMA_MSG_TYPE_REFRESH || msgType == MAMA_MSG_TYPE_RECAP)
   {
      // todo will have to also check for book message types but probably in different method
      isRefresh= true;
   }

   RsslRefreshMsg refreshMsg = RSSL_INIT_REFRESH_MSG;
   RsslUpdateMsg updateMsg = RSSL_INIT_UPDATE_MSG;

   RsslInt32 streamId = chnl->refreshStreamList_.front();

   if (isRefresh)
   {
      msgBase = &refreshMsg.msgBase;
      msgBase->msgClass = RSSL_MC_REFRESH;

      refreshMsg.state.streamState = RSSL_STREAM_OPEN;
      refreshMsg.state.dataState = RSSL_DATA_OK;
      refreshMsg.state.code = RSSL_SC_NONE;

      if (solicitedMessages_)
      {
         refreshMsg.flags = RSSL_RFMF_HAS_MSG_KEY | RSSL_RFMF_SOLICITED | RSSL_RFMF_REFRESH_COMPLETE | RSSL_RFMF_HAS_QOS | RSSL_RFMF_CLEAR_CACHE;
      }
      else
      {
         refreshMsg.flags = RSSL_RFMF_HAS_MSG_KEY | RSSL_RFMF_REFRESH_COMPLETE | RSSL_RFMF_HAS_QOS | RSSL_RFMF_CLEAR_CACHE;
      }

      char stateText[StateTextLen];
      sprintf(stateText, "Item Refresh Completed");
      refreshMsg.state.text.data = stateText;
      refreshMsg.state.text.length = (RsslUInt32)strlen(stateText);
      msgBase->msgKey.flags = RSSL_MKF_HAS_SERVICE_ID | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_NAME_TYPE;
      /* ServiceId */
      msgBase->msgKey.serviceId = serviceId_;
      /* Itemname */
      msgBase->msgKey.name.data = const_cast<char *>(symbol_.c_str());
      msgBase->msgKey.name.length = (RsslUInt32)symbol_.size();
      msgBase->msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
      /* Qos */
      refreshMsg.qos.dynamic = RSSL_FALSE;
      refreshMsg.qos.rate = RSSL_QOS_RATE_TICK_BY_TICK;
      refreshMsg.qos.timeliness = RSSL_QOS_TIME_REALTIME;
      rsslMsg = (RsslMsg *)&refreshMsg;
   }
   else
   {
      msgBase = &updateMsg.msgBase;
      msgBase->msgClass = RSSL_MC_UPDATE;

       //include msg key in updates for non-interactive provider streams
      if (streamId < 0)
      {
         updateMsg.flags = RSSL_UPMF_HAS_MSG_KEY;
         msgBase->msgKey.flags = RSSL_MKF_HAS_SERVICE_ID | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_NAME_TYPE;
         /* ServiceId */
         msgBase->msgKey.serviceId = serviceId_;
         /* Itemname */
         msgBase->msgKey.name.data = const_cast<char *>(symbol_.c_str());
         msgBase->msgKey.name.length = (RsslUInt32)symbol_.size();
         msgBase->msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
      }
      rsslMsg = (RsslMsg *)&updateMsg;
   }

   msgBase->domainType = RSSL_DMT_MARKET_PRICE;
   msgBase->containerType = RSSL_DT_FIELD_LIST;

    //StreamId
   msgBase->streamId = chnl->refreshStreamList_.front();

   // encode the message
   UPAFieldEncoder encoder(mamaDictionary_, upaFieldMap_, rmdsDictionary_, source_, symbol_);
   return encoder.encode(msg, chnl->channel_, rsslMsg, rsslMessageBuffer);
}
