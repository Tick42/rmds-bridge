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
#define __STDC_LIMIT_MACROS

#include "stdafx.h"
#include "UPASubscription.h"
#include "RMDSSubscriber.h"
#include <utils/t42log.h>
#include <utils/stdlib_port.h>
#include <utils/time.h>

#if defined(ENABLE_TICK42_ENHANCED)
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
#include "enhanced/T42Enh_UPAMfeedParser.h"
#endif

#include "UPABridgePoster.h"
#include "UPAMamaCommonFields.h"
#include "UPADecodeUtils.h"

extern "C"
{
#include "../tick42rmdsmsg/upapayloadimpl.h"
};

#include "UPAFieldDecoder.h"

using namespace utils::thread;

// fwd ref local function
int MonthFromName(char * monthName);

UPASubscription::UPASubscription(const std::string&  sourceName, const std::string& symbol, bool logRmdsValues )
    :sourceName_(sourceName), symbol_(symbol),  msgTotal_(0), streamId_(0),    msgNum_(0), msgSeqNum_(0), state_(SubscriptionStateInactive), subscriptionType_(SubscriptionTypeUnknown), logRmdsValues_(logRmdsValues),
    numDecodeFailures_(0), numDecodeFailuresLast_(0), timeLastReport_(0),openCloseCount_(0), gotInitial_(false), isSnapshot_(false),isRefresh_(false),
    reportedMFeedNotSupported_(false), reportedAnsiNotSupported_(false), sendRecap_(true), useCallbacks_(false), sendAckMessages_(true)
{
    t42log_debug("created new subscription for %s on stream %d\n", symbol_.c_str(), streamId_);
}

UPASubscription::~UPASubscription()
{
}

bool UPASubscription::Open( UPAConsumer_ptr_t consumer )
{
    SetSubscriptionState(SubscriptionStateSubscribing);
    consumer_ = consumer;

    // grab a reference to the mamaMessage from the consumer
    msg_ = consumer_->MamaMsg();

    // get the field map
    fieldmap_ = consumer_->GetOwner()->FieldMap();

    // initialise the book message structures
    bookByPriceMessage_.Fieldmap(fieldmap_);
    bookByOrderMessage_.Fieldmap(fieldmap_);

    std::string transportName = consumer->GetOwner()->GetTransportName();
    boost::shared_ptr<TransportConfig_t> config_ = boost::make_shared<TransportConfig_t>(transportName);
    std::string sendDup = config_->getString("sendRecapOnDuplicate");
    sendRecap_ = (sendDup == "" || sendDup == "true");
    useCallbacks_ = config_->getBool("use-callbacks", Default_useCallbacks);
    sendAckMessages_ = config_->getBool("send-ack-messages", Default_sendAckMessage);

    t42log_debug("queue open request for %s on stream %d\n", symbol_.c_str(), streamId_);
    QueueOpenRequest();

    return true;
}

void UPASubscription::QueueOpenRequest()
{
    // create closure to carry a boost shared pointer to this on the queue and ensure it doesn't get deleted while queued
    UPASubscriptionClosure * closure = new UPASubscriptionClosure(shared_from_this());
    mamaQueue_enqueueEvent(consumer_->RequestQueue(),  UPASubscription::SubscriptionOpenRequestCb, (void*) closure);
}

bool UPASubscription::Snapshot( UPAConsumer_ptr_t consumer, RMDSBridgeSnapshot_ptr_t snapRequest )
{
    SetSubscriptionState(SubscriptionStateSubscribing);
    consumer_ = consumer;

    // grab a reference to the mamaMessage from the consumer
    msg_ = consumer_->MamaMsg();


    isSnapshot_ = true;
    snapShot_ = snapRequest;
    fieldmap_ = consumer_->GetOwner()->FieldMap();


    t42log_debug("queue open request for %s on stream %d\n", symbol_.c_str(), streamId_);
    QueueSnapRequest();

    return true;
}

bool UPASubscription::Refresh( UPAConsumer_ptr_t consumer, RMDSBridgeSubscription_ptr_t subRequest )
{
    consumer_ = consumer;
    isRefresh_ = true;
    subscription_ = subRequest;
    fieldmap_ = consumer_->GetOwner()->FieldMap();

    t42log_debug("queue refresh request for %s on stream %d\n", symbol_.c_str(), streamId_);
    QueueRefreshRequest();

    return true;
}


void UPASubscription::QueueSnapRequest()
{
    // create closure to carry a boost shared pointer to this on the queue and ensure it doesn't get deleted while queued
    UPASubscriptionClosure * closure = new UPASubscriptionClosure(shared_from_this());
    mamaQueue_enqueueEvent(consumer_->RequestQueue(),  UPASubscription::SubscriptionSnapshotRequestCb, (void*) closure);
}

void UPASubscription::QueueRefreshRequest()
{
    // create closure to carry a boost shared pointer to this on the queue and ensure it doesn't get deleted while queued
    UPASubscriptionClosure * closure = new UPASubscriptionClosure(shared_from_this());
    mamaQueue_enqueueEvent(consumer_->RequestQueue(),  UPASubscription::SubscriptionRefreshRequestCb, (void*) closure);
}


void MAMACALLTYPE UPASubscription::SubscriptionOpenRequestCb(mamaQueue queue, void *closure)
{
    // extract the object from the closure and do stuff with it
    // when we have finished, delete the closure to release the shared pointer
    UPASubscriptionClosure * cl = (UPASubscriptionClosure *)closure;
    UPASubscription_ptr_t sub = cl->GetPtr();

    int curOpenCloseCount = ++sub->openCloseCount_;

    // Check the state on the object. It may have been closed while it was on the queue
    // the other states are inactive in which case it has been closed or live/stale in which case it has already been opened
    if (sub->GetSubscriptionState() == SubscriptionStateSubscribing)
    {
        sub->SendOpenRequest();
    }
    else
    {
        t42log_debug("Dispatched Open request ignored for %s : %s on stream %d as item is in state %d ", sub->SourceName().c_str(), sub->Symbol().c_str(), sub->StreamId(), sub->GetSubscriptionState());
    }

    delete cl;
}



void MAMACALLTYPE UPASubscription::SubscriptionSnapshotRequestCb(mamaQueue queue, void *closure)
{
    // extract the object from the closure and do stuff with it
    // when we have finished, delete the closure to release the shared pointer
    UPASubscriptionClosure * cl = (UPASubscriptionClosure *)closure;
    UPASubscription_ptr_t sub = cl->GetPtr();

    if (sub->GetSubscriptionState() == SubscriptionStateSubscribing)
    {
        sub->SendOpenRequest(true);
    }
    else
    {
        t42log_debug("Dispatched Open request ignored for %s : %s on stream %d as item is in state %d ", sub->SourceName().c_str(), sub->Symbol().c_str(), sub->StreamId(), sub->GetSubscriptionState());
    }

    delete cl;
}

void MAMACALLTYPE UPASubscription::SubscriptionRefreshRequestCb(mamaQueue queue, void *closure)
{
    // extract the object from the closure and do stuff with it
    // when we have finished, delete the closure to release the shared pointer
    UPASubscriptionClosure * cl = (UPASubscriptionClosure *)closure;
    UPASubscription_ptr_t sub = cl->GetPtr();

    if (sub->GetSubscriptionState() == SubscriptionStateSubscribing || sub->GetSubscriptionState() == SubscriptionStateLive)
    {
        sub->SendOpenRequest(false);
    }
    else
    {
        t42log_debug("Dispatched Open request ignored for %s : %s on stream %d as item is in state %d ", sub->SourceName().c_str(), sub->Symbol().c_str(), sub->StreamId(), sub->GetSubscriptionState());
    }

    delete cl;
}

bool UPASubscription::SendOpenRequest( bool isSnapshot )
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

    // if we dont already have a stream id allocated;
    if (streamId_ == 0)
    {
        // get a stream id
        streamId_ = consumer_->StreamManager().AddItem(shared_from_this());
    }

    if (streamId_ == 0)
    {
        t42log_error("Market Price Get streamID failed\n");
        NotifyListenersError(MAMA_STATUS_PLATFORM);
        return false;
    }

    RsslChannel * UPAChannel = consumer_->RsslConsumerChannel();

    // get a buffer for the item request
    msgBuf = rsslGetBuffer(UPAChannel, consumer_->MaxMessageSize(), RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
        // encode the item request
        if (EncodeItemRequest(UPAChannel, msgBuf, streamId_, isSnapshot) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_error("Market Price encodeItemRequest() failed\n");
            NotifyListenersError(MAMA_STATUS_PLATFORM);
            return false;
        }

        t42log_debug("Send Open for %s on stream %d\n", Symbol().c_str(), streamId_);

        consumer_->StatsSubscribed();
        // Update the pending opens count]

        t42log_debug("inc pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
        UPAStreamManager &mgr = consumer_->StreamManager();
        mgr.addPendingItem(this);

        if (SendUPAMessage(UPAChannel, msgBuf) != RSSL_RET_SUCCESS)
        {
            mgr.removePendingItem(this);
            consumer_->StatsSubscriptionsFailed();
            return false;
        }
    }

    return true;
}



// Encode an item request
// Set the DMT domain according to the subscription type
RsslRet UPASubscription::EncodeItemRequest(RsslChannel* UPAChannel, RsslBuffer* msgBuffer, RsslInt32 streamId,  bool isSnapshot)
{
    RsslRet ret = 0;
    RsslRequestMsg msg = RSSL_INIT_REQUEST_MSG;
    RsslEncodeIterator encodeIter;

    // clear encode iterator
    rsslClearEncodeIterator(&encodeIter);

    // initialise message
    msg.msgBase.msgClass = RSSL_MC_REQUEST;
    msg.msgBase.streamId = streamId;
    msg.msgBase.containerType = RSSL_DT_NO_DATA;

    // todo add support for QoS information
    if(isSnapshot )
    {
        // for snapshot dont want to add the RSSL_RQMF_STREAMING flag
        msg.flags =  RSSL_RQMF_HAS_PRIORITY;
    }
    else
    {
        msg.flags = RSSL_RQMF_STREAMING | RSSL_RQMF_HAS_PRIORITY;
    }


    msg.priorityClass = 1;
    msg.priorityCount = 1;

    // set the DMT domain
    // todo add a function to convert type enum to string for logging
    switch (subscriptionType_)
    {
    default:
    case SubscriptionTypeUnknown:
        t42log_warn("UPASubscription::EncodeItemRequest - Unknown subscription type for %s : %s", sourceName_.c_str(), symbol_.c_str() );
        return RSSL_RET_FAILURE;

    case SubscriptionTypeMarketByOrder:
        msg.msgBase.domainType = RSSL_DMT_MARKET_BY_ORDER;
        t42log_debug("UPASubscription::EncodeItemRequest - Requesting  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;

    case SubscriptionTypeMarketByPrice:
        msg.msgBase.domainType = RSSL_DMT_MARKET_BY_PRICE;
        t42log_debug("UPASubscription::EncodeItemRequest - Requesting  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;

    case SubscriptionTypeMarketPrice:
        msg.msgBase.domainType = RSSL_DMT_MARKET_PRICE;
        t42log_debug("UPASubscription::EncodeItemRequest - Requesting  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;
    }

    // initialise the message key
    msg.msgBase.msgKey.flags = RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_SERVICE_ID;
    msg.msgBase.msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
    msg.msgBase.msgKey.name.data = const_cast<char *>(symbol_.c_str());
    msg.msgBase.msgKey.name.length = (rtrUInt32)symbol_.length();

    msg.msgBase.msgKey.serviceId = (RsslUInt16) source_->ServiceId();

    // encode the message
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
        return ret;
    }
    rsslSetEncodeIteratorRWFVersion(&encodeIter, UPAChannel->majorVersion, UPAChannel->minorVersion);
    if ((ret = rsslEncodeMsgInit(&encodeIter, (RsslMsg*)&msg, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslEncodeMsgInit() failed with return code: %d\n", ret);
        return ret;
    }

    if ((ret = rsslEncodeMsgComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslEncodeMsgComplete() failed with return code: %d\n", ret);
        return ret;
    }

    msgBuffer->length = rsslGetEncodedBufferLength(&encodeIter);

    return RSSL_RET_SUCCESS;
}

// Process a market price response. Caller has decoded the message to the point where the message type is knowm
RsslRet UPASubscription::InternalProcessMarketPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;

    UPAStreamManager &mgr = consumer_->StreamManager();
    UPASubscriptionState st = GetSubscriptionState();
    if (st == SubscriptionStateInactive)
    {
        // its been closed so do nothing
        t42log_debug("got response on closed item for %s on stream %d\n", symbol_.c_str(), streamId_);

        if (!gotInitial_)
        {
            t42log_debug("got response on closed item no initial - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
            mgr.removePendingItem(this);
        }
        return RSSL_RET_SUCCESS;
    }

    if (st == SubscriptionStateSubscribing || st == SubscriptionStateStale)
    {
        // response to a subscribe message so decrement  the count
        t42log_debug("got response - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
        bool found = mgr.removePendingItem(this);
        gotInitial_ = true;
    }

    setLineTime();

    RsslUInt8 updateType = 0;
    RsslUInt32 seqNum = 0;

    //  init a temp buffer for logging the state
    char tempData[1024];
    RsslBuffer stateBuffer;
    stateBuffer.data = tempData;
    stateBuffer.length = 1024;

    RsslMsgKey* key = 0;

    mamaMsgType msgType = MAMA_MSG_TYPE_UNKNOWN;

    // will need to know later of we were processing a refresh
    bool isRefreshMsg = false;
    switch (msg->msgBase.msgClass)
    {
    case RSSL_MC_REFRESH:
        /* update our item state list if its a refresh, then process just like update */

        t42log_debug("got refresh for %s on stream %d\n", symbol_.c_str(), streamId_);

        isRefreshMsg = true;

        if (isRefresh_)
        {
            // Remove recap from pending items
            mgr.removePendingItem(this);
        }

        if (!SetRsslState(&msg->refreshMsg.state))
        {
            // seems that if we request a bad symbol we sometimes get a refresh with bad name state rather than a status message

            // SetRsslState will return false and log a message if the condition is bad. It will return true if there is not state change or the state is good. This happens
            // when we get a second or subsequent image

            break;
        }


        // fall through into the update handling

    case RSSL_MC_UPDATE:

        {
            // get key
            key = (RsslMsgKey *)rsslGetMsgKey(msg);

            // log the item name
            t42log_debug("Update on %s - %s\n", symbol_.c_str(), rsslDomainTypeToString(msg->msgBase.domainType));

            if (msg->msgBase.msgClass == RSSL_MC_UPDATE)
            {
                // log the update type
                t42log_debug("Update Type: %u\n", msg->updateMsg.updateType);
                updateType = msg->updateMsg.updateType;

                if (msg->updateMsg.flags & RSSL_UPMF_HAS_SEQ_NUM)
                {
                    seqNum = msg->updateMsg.seqNum;
                }
            }

            // bit nasty but we may need to fake an initial image if we have transitioned stale to live on an update
            bool fakeInitial = false;

            if (GetSubscriptionState() == SubscriptionStateStale && msg->refreshMsg.state.dataState == RSSL_DATA_OK)
            {
                // stale now live
                rsslStateToString(&stateBuffer, &msg->refreshMsg.state);
                if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
                {

                    t42log_debug("Stale to Live - Refresh %s\n", stateBuffer.data);
                }
                else
                {
                    // it was an update

                    // although feeds like IDN will always throw a new image (RSSL_MC_REFRESH) on a stale to live transition this cannot be guaranteed so
                    // ensure the client will see the update as an image

                    t42log_debug("Stale to Live - No refresh %s\n", stateBuffer.data);
                    fakeInitial = true;
                }

                SetSubscriptionState(SubscriptionStateLive);
            }


            // force to initial if we have faked an image
            msgType = SetMessageType(fakeInitial? RSSL_MC_REFRESH: msg->msgBase.msgClass, updateType, false);

            if (msg->refreshMsg.flags & RSSL_RFMF_HAS_SEQ_NUM)
            {
                seqNum = msg->refreshMsg.seqNum;
            }

            if (msg->msgBase.containerType ==  139) // Marketfeed
            {
                ProcessMFeedMessage(msg);
            }
            else if (msg->msgBase.containerType ==  134) // ANSI buffer
            {
                ProcessAnsiMessage(msg);
            }
            else
            {
                RsslFieldList fList = RSSL_INIT_FIELD_LIST;
                RsslFieldEntry fEntry = RSSL_INIT_FIELD_ENTRY;
                // use the rssl field list decoder to decode the fields
                if ((ret = rsslDecodeFieldList(dIter, &fList, 0)) == RSSL_RET_SUCCESS)
                {
                    UPAFieldDecoder decoder(consumer_, fieldmap_, symbol_, sourceName_);
                    // decode each field entry in list
                    while ((ret = rsslDecodeFieldEntry(dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
                    {
                        if (ret == RSSL_RET_SUCCESS)
                        {
                            // then process each field
                            if (decoder.DecodeFieldEntry(&fEntry, dIter, msg_) != RSSL_RET_SUCCESS)
                            {
                                ReportDecodeFailure("DecodeFieldEntry()", ret );
                                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                return RSSL_RET_SUCCESS;
                            }
                        }
                        else
                        {
                            ReportDecodeFailure("rsslDecodeFieldEntry()", ret );
                            // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                            return RSSL_RET_SUCCESS;
                        }
                    }
                }
                else
                {
                    ReportDecodeFailure("rsslDecodeFieldList()", ret );
                    // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                    return RSSL_RET_SUCCESS;
                }
            }

            // bump the mama message num fields
            setMsgNum(false);

            if (isSnapshot_)
            {
                // this is just a regular snapshot
                mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);

                if(0 != snapShot_)
                {
                    snapShot_->OnMessage(msg_);
                    // now we have sent the message we dont need the reply any more
                    snapShot_.reset();
                }
            }
            else if (isRefresh_ && isRefreshMsg)
            {
            // test the isRefresh here because we may get an update between requesting the snapshot and receiving the reponse
                mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);

                // in the subscription stream
                // send the message to each of the listeners
                NotifyListenersRefreshMessage(msg_, subscription_);
                isRefresh_= false;
                subscription_.reset();
            }
            else
            {
                // its just a regular subscription
                // send the message to each of the listeners
                NotifyListenersMessage(msg_, msgType);
            }
            break;
        }

    case RSSL_MC_STATUS:
        if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
        {
            //rsslStateToString(&stateBuffer, &msg->statusMsg.state);
            //t42log_debug("%s    %s", Symbol().c_str(), stateBuffer.data);

            //// and set the new state for the subscription
            //SetRsslState(&msg->statusMsg.state);
            rsslStateToString(&stateBuffer, &msg->statusMsg.state);
            t42log_debug("%s %s", Symbol().c_str(), stateBuffer.data);

            // and set the new state for the subscription
            if (isSnapshot_)
            {
                // this is just a regular snapshot
                t42log_debug("Calling OnMessage() method for a snapshot for MAMA_MSG_STATUS_NOT_FOUND state");
                mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_NOT_FOUND);

                if(0 != snapShot_)
                {
                    t42log_debug("Calling OnMessage() method for a snapshot for MAMA_MSG_STATUS_NOT_FOUND state");
                    snapShot_->OnMessage(msg_);
                    // now we have sent the message we dont need the reply any more
                    snapShot_.reset();
                }
             }
            else
            {
                SetRsslState(&msg->statusMsg.state);
            }
        }
        break;

    case RSSL_MC_ACK:

        // this will be an ack for an on-stream publish on this stream
        t42log_debug("Received AckMsg for stream %d", msg->msgBase.streamId);

        {
            UPABridgePoster_ptr_t poster;
            PublisherPostMessageReply_ptr_t reply;

            RsslUInt32 id = msg->ackMsg.ackId;
            if (consumer_->PostManager().RemovePost(id, poster, reply))
            {
                poster->ProcessAck(msg, dIter, reply.get());
            }
            else
            {
                t42log_warn("received response for on stream post with unknown id %d \n", id);
            }
        }
        break;

    default:
        t42log_warn("Received unhandled Item Msg Class: %d\n", msg->msgBase.msgClass);
        break;
    }


    return RSSL_RET_SUCCESS;
}


mamaMsgType UPASubscription::SetMessageType( RsslUInt8 messageClass, RsslUInt8 updateType, bool bookMessage)
{
    // record when we get and initial so we can differentiate between initial and recap
    const CommonFields &commonFields = UpaMamaCommonFields::CommonFields();
    mamaMsgType msgType = MAMA_MSG_TYPE_UNKNOWN;

    if (!bookMessage)
    {
        if(isSnapshot_)
        {
            msgType = MAMA_MSG_TYPE_SNAPSHOT;
        }
        else
        {
            if (messageClass == RSSL_MC_REFRESH)
            {
                msgType = MAMA_MSG_TYPE_INITIAL;
            }

            if (messageClass == RSSL_MC_UPDATE)
            {
                switch(updateType)
                {
                case RDM_UPD_EVENT_TYPE_QUOTE:
                    msgType = MAMA_MSG_TYPE_QUOTE;
                    break;
                case RDM_UPD_EVENT_TYPE_TRADE:
                    msgType = MAMA_MSG_TYPE_TRADE;
                    break;
                default:
                    // assume everything else is an update
                    // some providers (e.g. the upa sample providers) don't encode a trade/quote update type
                    msgType = MAMA_MSG_TYPE_UPDATE;
                }
            }

        }

    }
    else
    {
        // its a book message
        if(isSnapshot_)
        {
            msgType = MAMA_MSG_TYPE_BOOK_SNAPSHOT;
        }
        else
        {
            if (messageClass == RSSL_MC_REFRESH)
            {
                msgType = MAMA_MSG_TYPE_BOOK_INITIAL;
            }

            if (messageClass == RSSL_MC_UPDATE)
            {

                msgType = MAMA_MSG_TYPE_BOOK_UPDATE;

            }

        }

    }

    mamaMsgStatus msgStatus = (SubscriptionStateLive == GetSubscriptionState()) ? MAMA_MSG_STATUS_OK : MAMA_MSG_STATUS_STALE;

    mamaMsg_addU8(msg_, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, msgType);
    mamaMsg_addU8(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid,  msgStatus);
    // mamaMsg_addString(msg_, commonFields.wIssueSymbol.mama_field_name.c_str(),commonFields.wIssueSymbol.mama_fid, symbol_.c_str());
    // mamaMsg_addString(msg_, commonFields.wSymbol.mama_field_name.c_str(),commonFields.wSymbol.mama_fid, symbol_.c_str());

    return msgType;
}


void UPASubscription::setMsgNum(bool IsStatusMsg)
{
    mamaMsg_addI32(msg_, MamaFieldMsgTotal.mName, MamaFieldMsgTotal.mFid, msgTotal_);
    mamaMsg_addI32(msg_, MamaFieldMsgNum.mName, MamaFieldMsgNum.mFid, msgNum_);


    // There is an issue with rmds in that it appears that sequence numbers increment by 16
    // and roll at 64K. Neither of these features are conducive to sensible gap detection
    // and the dq processing in openMAMA just marks the subscription as stale and periodically
    // requests a recap. Until this is resolved, we will just use an internally generated
    // sequence number which is a count of the messages received on the stream.


    if(IsStatusMsg)
    {
        // for a status message we  need to set the seqnum to zero, otherwise it will be reported as a duplicate and the gapo detection gets confused
        mamaMsg_addI64(msg_, MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, 0);
    }
    else
    {
        // regular message insert and inc. the sequence number
        mamaMsg_addI64(msg_,  MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, msgSeqNum_);
        if (msgSeqNum_ == INT64_MAX)
        {
            msgSeqNum_ = 0;
        }
        else
        {
            ++msgSeqNum_;
        }
    }


    ++msgNum_;
    ++msgTotal_;
}

void UPASubscription::setMsgNumBook()
{
    // MamdaBook handling requires difference types for the book
    mamaMsg_addI32(msg_, MamaFieldMsgTotal.mName, MamaFieldMsgTotal.mFid, msgTotal_);
    mamaMsg_addI32(msg_, MamaFieldMsgNum.mName, MamaFieldMsgNum.mFid, msgNum_);
    mamaMsg_addI64(msg_,  MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, msgNum_);


    ++msgNum_;
    ++msgTotal_;
}

// send a stale message
bool UPASubscription::SetStale(const char* msg)
{
    if (GetSubscriptionState() != UPASubscription::SubscriptionStateInactive)
    {
        SetSubscriptionState(SubscriptionStateStale);

        if (msg != NULL)
        {
            mamaMsg_addString(msg_, MamaFieldFeedName.mName, MamaFieldFeedName.mFid, msg);
        }

        mamaMsg_addU8(msg_, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_SEC_STATUS);
        mamaMsg_addU8(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid,  MAMA_MSG_STATUS_STALE);
        setMsgNum(true);

        if (sendAckMessages_)
        {
            NotifyListenersMessage(msg_, MAMA_MSG_TYPE_SEC_STATUS);
        }

        if (useCallbacks_)
        {
            NotifyListenersQuality(MAMA_QUALITY_STALE, 0);
        }

        mamaMsg_clear(msg_);
        return true;
    }

    return false;
}

// send a live message
bool UPASubscription::SetLive()
{
    if (GetSubscriptionState() != UPASubscription::SubscriptionStateInactive)
    {
        SetSubscriptionState(SubscriptionStateLive);


        mamaMsg_addU8(msg_, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_SEC_STATUS);
        mamaMsg_addU8(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid,  MAMA_MSG_STATUS_OK);

        setMsgNum(true);

        if (sendAckMessages_)
        {
            NotifyListenersMessage(msg_, MAMA_MSG_TYPE_SEC_STATUS);
        }

        if (useCallbacks_)
        {
            NotifyListenersQuality(MAMA_QUALITY_OK, 0);
        }

        mamaMsg_clear(msg_);
        return true;
    }

    return false;
}

bool UPASubscription::ReSubscribe()
{
    return Open(consumer_);
}

void UPASubscription::LogRsslState(RsslState* state)
{
    RsslBuffer stateBuf;
    char buf[81];
    memset((void*)buf,0,sizeof(buf));
    stateBuf.data = buf;
    stateBuf.length = sizeof(buf)-1;
    rsslStateToString(&stateBuf, state);
    t42log_warn("SetRsslState: state notification %s.%s %s/%d %s/%d %s/%d - %s",
        SourceName().c_str(), Symbol().c_str(),
        rsslStreamStateToString(state->streamState), state->streamState,
        rsslStateCodeToString(state->code), state->code,
        rsslDataStateToString(state->dataState), state->dataState,
        stateBuf.data);
}

bool UPASubscription::SetRsslState(RsslState* state)
{
    itemState_.streamState = state ->streamState;
    itemState_.dataState = state->dataState;
    itemState_.code = state->code;

#if 0
    LogRsslState(state);
#endif

    // The 36 was seen from an RTIC, it is not defined in the state codes from RSSL
    if (state->streamState == RSSL_STREAM_CLOSED && (state->code == RSSL_SC_NOT_FOUND || state->code == 36))
    {
        // invalid security
        NotifyListenersError(MAMA_STATUS_NOT_FOUND);
        consumer_->StatsSubscriptionsFailed();
        return false;
    }
    else if (state->streamState == RSSL_STREAM_CLOSED && state->code == RSSL_SC_NONE && state->dataState == RSSL_DATA_SUSPECT)
    {
        // not found
        NotifyListenersError(MAMA_STATUS_NOT_FOUND);
        consumer_->StatsSubscriptionsFailed();
        return false;
    }
    else if (state->streamState == RSSL_STREAM_CLOSED && state->code ==RSSL_SC_NOT_ENTITLED)
    {
        // not entitled
        NotifyListenersError(MAMA_STATUS_NOT_ENTITLED);
        consumer_->StatsSubscriptionsFailed();
        return false;
    }
    else if (state->streamState == RSSL_STREAM_CLOSED && state->code ==RSSL_SC_TIMEOUT)
    {
        // timed out on rmds
        NotifyListenersError(MAMA_STATUS_TIMEOUT);
        consumer_->StatsSubscriptionsFailed();
        return false;
    }
    else if (state->streamState == RSSL_STREAM_NON_STREAMING)
    {
        if(0 != snapShot_)
        {
            //it was a standalone snaphot
            // there is no streaming data on this but treat as successful
            SendStatusMsg(MAMA_MSG_STATUS_OK);
            SetSubscriptionState(SubscriptionStateInactive);
        }

        // otherwise we dont need to change the ste or send a status messagwe
        consumer_->StatsSubscriptionsSucceeded();
        return true;
    }
    else if (state->streamState == RSSL_STREAM_OPEN && state->code == RSSL_SC_PREEMPTED)
    {
        // preempted by the infrastructure
        // really need the MAMA interface to define a preempted status
        NotifyListenersError(MAMA_STATUS_PLATFORM);
        consumer_->StatsSubscriptionsFailed();
        t42log_warn("Subscription for %s : %s preempted on RMDS", source_->ServiceName().c_str(), Symbol().c_str());
        return false;
    }
    else if (state->streamState == RSSL_STREAM_OPEN && state->dataState == RSSL_DATA_SUSPECT)
    {
        // this stream has been marked stale
        RsslBuffer stateBuf;
        char buf[256];
        memset((void*)buf, 0, sizeof(buf));
        stateBuf.data = buf;
        stateBuf.length = sizeof(buf)-1;
        rsslStateToString(&stateBuf, state);

        SetStale(buf);

        return true;
    }
    // we got data OK and we were were stale
    else if (state->streamState == RSSL_STREAM_OPEN && state->dataState == RSSL_DATA_OK && GetSubscriptionState() == SubscriptionStateStale)
    {
        SetLive();
        return true;
    }

    else if (state->streamState == RSSL_STREAM_OPEN && state->dataState == RSSL_DATA_NO_CHANGE)
    {
        // This comes back from RMDS sometimes, not just sure what it is
        // SetRsslState: state notification BBG_BPIPE_DEV.PG.UN RSSL_STREAM_OPEN/1 RSSL_SC_NONE/0 RSSL_DATA_NO_CHANGE/0 - State: Open/No Change/None - text: "A27: Message Ordering Error. Recalling."
        if (GetSubscriptionState() == SubscriptionStateSubscribing) {
            SetSubscriptionState(SubscriptionStateLive);
            consumer_->StatsSubscriptionsSucceeded();
            consumer_->StreamManager().IncOpenItems();
        }
        return true;
    }

    // we got data and we were waiting for it
    else if (state->streamState == RSSL_STREAM_OPEN && state->dataState == RSSL_DATA_OK && GetSubscriptionState() == SubscriptionStateSubscribing)
    {
        SetSubscriptionState(SubscriptionStateLive);
        consumer_->StatsSubscriptionsSucceeded();
        consumer_->StreamManager().IncOpenItems();
        return true;
    }
    else
    {
        // for the moment just log any other unhandled states (unless its an OK state)

        if ( !(state->streamState == RSSL_STREAM_OPEN && state->dataState == RSSL_DATA_OK))
        {
            LogRsslState(state);
            consumer_->StatsSubscriptionsFailed();
            return false;
        }

        return true;
    }

    return true;
}

// close a stream

RsslRet UPASubscription::CloseStream(RsslChannel* UPAChannel, RsslInt32 streamId)
{
    RsslError error;
    RsslBuffer* msgBuff = 0;

    // get a buffer for the item close
    msgBuff = rsslGetBuffer(UPAChannel, consumer_->MaxMessageSize(), RSSL_FALSE, &error);

    if (msgBuff != NULL)
    {
        // encode item close
        if (EncodeItemClose(UPAChannel, msgBuff, streamId) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuff, &error);
            t42log_warn("Market Price EncodeItemClose() failed\n");
            return RSSL_RET_FAILURE;
        }

        t42log_debug("Send close for %s on stream %d\n", Symbol().c_str(), streamId);
        // send close
        if (SendUPAMessage(UPAChannel, msgBuff) != RSSL_RET_SUCCESS)
        {
            return RSSL_RET_FAILURE;
        }
        consumer_->StreamManager().DecOpenItems();
    }
    else
    {
        t42log_warn("rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}

// Encode an Item Close message

RsslRet UPASubscription::EncodeItemClose(RsslChannel* chnl, RsslBuffer* msgBuf, RsslInt32 streamId)
{
    RsslRet ret = 0;
    RsslCloseMsg msg = RSSL_INIT_CLOSE_MSG;
    RsslEncodeIterator encodeIter;

    // clear the encode iterator
    rsslClearEncodeIterator(&encodeIter);

    // init the message
    msg.msgBase.msgClass = RSSL_MC_CLOSE;
    msg.msgBase.streamId = streamId;
    msg.flags |= RSSL_CLMF_ACK;

    switch (subscriptionType_)
    {
    case SubscriptionTypeUnknown:
        t42log_warn("UPASubscription::EncodeItemClose - Unknown subscription type for %s : %s", sourceName_.c_str(), symbol_.c_str() );
        return RSSL_RET_FAILURE;

    case SubscriptionTypeMarketByOrder:
        msg.msgBase.domainType = RSSL_DMT_MARKET_BY_ORDER;
        t42log_debug("UPASubscription::EncodeItemClose - closing  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;

    case SubscriptionTypeMarketByPrice:
        msg.msgBase.domainType = RSSL_DMT_MARKET_BY_PRICE;
        t42log_debug("UPASubscription::EncodeItemClose - closing  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;

    case SubscriptionTypeMarketPrice:
        msg.msgBase.domainType = RSSL_DMT_MARKET_PRICE;
        t42log_debug("UPASubscription::EncodeItemClose - closing  %s : %s as subscription type %d",  sourceName_.c_str(), symbol_.c_str(), subscriptionType_);
        break;


    }
    msg.msgBase.containerType = RSSL_DT_NO_DATA;
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
        return ret;
    }
    rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
    if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&msg)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeMsg() failed with return code: %d\n", ret);
        return ret;
    }

    msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

    return RSSL_RET_SUCCESS;
}

bool UPASubscription::Close()
{
    SetSubscriptionState(SubscriptionStateInactive);
    t42log_debug("queue close request for %s on stream %d\n", symbol_.c_str(), streamId_);
    QueueCloseRequest();
    return true;
}

void MAMACALLTYPE UPASubscription::SubscriptionCloseRequestCb(mamaQueue queue, void *closure)
{
    UPASubscription *pSubscription = (UPASubscription *)closure;

    // extract the object from the closure and do stuff with it
    // when we have finished, delete the closure to release the shared pointer
    UPASubscriptionClosure * cl = (UPASubscriptionClosure *)closure;
    UPASubscription_ptr_t sub = cl->GetPtr();

    sub->openCloseCount_--;
    sub->SendCloseRequest();

    delete cl;
}

void UPASubscription::QueueCloseRequest()
{
    // There is a potential hazard that if the subscription hasn't completed when the item is closed that consumer_ will still be unitialised
    if(consumer_ != 0)
    {
    consumer_->StreamManager().IncPendingCloses();

    // create closure to carry a boost shared pointer to this on the queue and ensure it doesn't get deleted while queued
    UPASubscriptionClosure * closure = new UPASubscriptionClosure(shared_from_this());

    mamaQueue_enqueueEvent(consumer_->RequestQueue(),  UPASubscription::SubscriptionCloseRequestCb, (void*) closure);
    }
}

bool UPASubscription::SendCloseRequest()
{
    RsslChannel *chnl = consumer_->RsslConsumerChannel();
    UPAStreamManager &mgr = consumer_->StreamManager();

    mgr.DecPendingCloses();

    // make sure we've actually opened (streamid != 0)
    if (streamId_ != 0)
    {
        CloseStream(chnl, streamId_);
        mgr.ReleaseStreamId(streamId_);

        // if we are not live at this point then we never will be so decrement pending count
        UPASubscriptionState state = GetSubscriptionState();
        if (state == SubscriptionStateSubscribing)
        {
            t42log_info("close dec pending items count for %s\n", symbol_.c_str());
            mgr.removePendingItem(this);
        }
    }

    return true;
}

// Process a market by order response message.
// Caller has decoded the message to the point where the message type is known
//
RsslRet UPASubscription::InternalProcessMarketByOrderResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;
    UPAStreamManager &mgr = consumer_->StreamManager();

    if ( GetSubscriptionState() == SubscriptionStateInactive)
    {
        // its been closed so do nothing
        t42log_debug("got response on closed item for %s on stream %d\n", symbol_.c_str(), streamId_);

        if (!gotInitial_)
        {
            t42log_debug("got response on closed item no initial - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
            mgr.removePendingItem(this);
        }
        return RSSL_RET_SUCCESS;
    }

    if (GetSubscriptionState() == SubscriptionStateSubscribing)
    {
        // response to a subscribe message so decrement  the count
        t42log_debug("got response - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
        mgr.removePendingItem(this);
        gotInitial_ = true;

    }

    // temp buffer for stringizing state
    char tempData[1024];
    RsslBuffer tempBuffer;
    tempBuffer.data = tempData;
    tempBuffer.length = 1024;


    RsslUInt8 updateType = 0;
    RsslUInt32 seqNum = 0;

    mamaMsgType msgType = MAMA_MSG_TYPE_UNKNOWN;

    // we use the book message object to maintain state between updates
    bookByOrderMessage_.StartUpdate();

    // will need to know later if we were processing a refresh
    bool isRefreshMsg = false;

    RsslMsgKey* key = 0;
    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REFRESH:

        // update the item state
        SetRsslState(&msg->refreshMsg.state);

        isRefreshMsg = true;

        if (isRefresh_)
        {
            // Remove recap from pending items
            mgr.removePendingItem(this);
        }

        // then just fall through

    case RSSL_MC_UPDATE:
        {
            key = (RsslMsgKey *)rsslGetMsgKey(msg);

            // log the symbol
            t42log_debug("%s - Domain: %s\n", symbol_.c_str(), rsslDomainTypeToString(msg->msgBase.domainType));

            // extract the sequence number according to the message type
            if (msg->msgBase.msgClass == RSSL_MC_UPDATE)
            {
                // log the update type
                t42log_debug("Update Type: %u\n", msg->updateMsg.updateType);
                updateType = msg->updateMsg.updateType;

                if (msg->refreshMsg.flags & RSSL_UPMF_HAS_SEQ_NUM)
                {
                    seqNum = msg->updateMsg.seqNum;
                }
            }

            if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
            {
                rsslStateToString(&tempBuffer, &msg->refreshMsg.state);
                t42log_debug("%s\n", tempBuffer.data);

                if (msg->refreshMsg.flags & RSSL_RFMF_HAS_SEQ_NUM)
                {
                    seqNum = msg->refreshMsg.seqNum;
                }
            }

            // bit nasty but we may need to fake an initial image if we have transitioned stale to live on an update
            bool fakeInitial = false;

            if (GetSubscriptionState() == SubscriptionStateStale && msg->refreshMsg.state.dataState == RSSL_DATA_OK)
            {
                // stale now live
                RsslBuffer stateBuffer;
                rsslStateToString(&stateBuffer, &msg->refreshMsg.state);
                if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
                {

                    t42log_debug("Stale to Live - Refresh %s\n", stateBuffer.data);
                }
                else
                {
                    // it was an update

                    // although feeds like IDN will always throw a new image (RSSL_MC_REFRESH) on a stale to live transition this cannot be guaranteed so
                    // ensure the client will see the update as an image

                    t42log_debug("Stale to Live - No refresh %s\n", stateBuffer.data);
                    fakeInitial = true;
                }

                SetSubscriptionState(SubscriptionStateLive);
            }


            // force to initial if we have faked an image
            msgType = SetMessageType(fakeInitial? RSSL_MC_REFRESH: msg->msgBase.msgClass, updateType, true);

            // now walk the message
            RsslMap map = RSSL_INIT_MAP;
            RsslFieldList fList = RSSL_INIT_FIELD_LIST;
            RsslFieldEntry fEntry = RSSL_INIT_FIELD_ENTRY;

            RsslLocalFieldSetDefDb localFieldSetDefDb;  // this must be cleared using the clear function below
            rsslClearLocalFieldSetDefDb(&localFieldSetDefDb);

            UPAFieldDecoder decoder(consumer_, fieldmap_, symbol_, sourceName_);

            // level 2 market by order is a map of field lists
            if ((ret = rsslDecodeMap(dIter, &map)) == RSSL_RET_SUCCESS)
            {
                // decode set definition database
                if (map.flags & RSSL_MPF_HAS_SET_DEFS)
                {
                    // required to decode the field list
                    if (rsslDecodeLocalFieldSetDefDb(dIter, &localFieldSetDefDb) != RSSL_RET_SUCCESS)
                    {
                        ReportDecodeFailure("rsslDecodeLocalFieldSetDefDb()", ret );
                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // decode any summary data - this should be a field list depending on the domain model
                if (map.flags & RSSL_MPF_HAS_SUMMARY_DATA)
                {
                    t42log_debug("SUMMARY DATA\n");
                    if ((ret = rsslDecodeFieldList(dIter, &fList, &localFieldSetDefDb)) == RSSL_RET_SUCCESS)
                    {
                        // decode each field entry in list
                        while ((ret = rsslDecodeFieldEntry(dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
                        {
                            if (ret == RSSL_RET_SUCCESS)
                            {
                                // decode each field
                                // these are regular fields so get decodeed into the mama message like any L1 fields
                                if (decoder.DecodeFieldEntry(&fEntry, dIter, msg_) != RSSL_RET_SUCCESS)
                                {
                                    ReportDecodeFailure("DecodeFieldEntry()", ret );
                                    // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                    return RSSL_RET_SUCCESS;
                                }
                            }
                            else
                            {
                                ReportDecodeFailure("rsslDecodeFieldEntry()", ret );
                                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                return RSSL_RET_SUCCESS;
                            }
                        }
                    }
                    else
                    {
                        ReportDecodeFailure("rsslDecodeFieldList()", ret );
                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // decode the map
                RsslMapEntry mapEntry = RSSL_INIT_MAP_ENTRY;
                RsslBuffer mapKey = RSSL_INIT_BUFFER;
                while ((ret = rsslDecodeMapEntry(dIter, &mapEntry, &mapKey)) != RSSL_RET_END_OF_CONTAINER)
                {
                    if (ret == RSSL_RET_SUCCESS)
                    {
                        const char* actionString;

                        UPABookEntry_ptr_t entry = UPABookEntry_ptr_t(new UPABookEntry);
                        entry.use_count();

                        /* convert the action to a string for display purposes */
                        switch(mapEntry.action)
                        {
                        case RSSL_MPEA_UPDATE_ENTRY:
                            actionString = "RSSL_MPEA_UPDATE_ENTRY";
                            entry->ActionCode('U');
                            break;
                        case RSSL_MPEA_ADD_ENTRY:
                            actionString = "RSSL_MPEA_ADD_ENTRY";
                            entry->ActionCode('A');
                            break;
                        case RSSL_MPEA_DELETE_ENTRY:
                            actionString = "RSSL_MPEA_DELETE_ENTRY";
                            entry->ActionCode('D');
                            break;
                        default:
                            actionString = "Unknown";
                            entry->ActionCode('Z');
                        }

                        /* print out the key */
                        if (mapKey.length)
                        {
                            t42log_debug("ORDER ID: %.*s\nACTION: %s\n", mapKey.length, mapKey.data, actionString);
                        }

                        // each entry in the map corresponds to an order
                        //
                        std::string orderId(mapKey.data,  mapKey.length);
                        entry->Orderid(orderId);

                        // only have a field list when the type is not a delete
                        if (mapEntry.action != RSSL_MPEA_DELETE_ENTRY)
                        {
                            // decode the field list
                            if ((ret = rsslDecodeFieldList(dIter, &fList, &localFieldSetDefDb)) == RSSL_RET_SUCCESS)
                            {

                                // and each entry
                                while ((ret = rsslDecodeFieldEntry(dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
                                {
                                    if (ret == RSSL_RET_SUCCESS)
                                    {
                                        if (decoder.DecodeBookFieldEntry(&fEntry, dIter, entry) != RSSL_RET_SUCCESS)
                                        {
                                            ReportDecodeFailure("decodeBookFieldEntry()", ret );
                                            // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                            return RSSL_RET_SUCCESS;
                                        }
                                    }
                                    else
                                    {
                                        t42log_warn("rsslDecodeFieldEntry() failed with return code: %d\n", ret);
                                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                        return RSSL_RET_SUCCESS;
                                    }
                                }

                                if (mapEntry.action == RSSL_MPEA_ADD_ENTRY )
                                {
                                    bookByOrderMessage_.AddEntry(entry);
                                }
                                else if (mapEntry.action == RSSL_MPEA_UPDATE_ENTRY)
                                {
                                    bookByOrderMessage_.UpdateEntry(entry);
                                }
                            }
                            else
                            {
                                ReportDecodeFailure("rsslDecodeFieldList()", ret );
                                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                return RSSL_RET_SUCCESS;
                            }
                        }
                        else
                        {
                            // do an entry delete
                            bookByOrderMessage_.RemoveEntry(entry);
                        }
                    }
                    else
                    {
                        ReportDecodeFailure("rsslDecodeMapEntry()", ret );

                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // now render the OpenMama message
                bookByOrderMessage_.BuildMamdaMessage(msg_);

                // send the message
                setMsgNum(false);


                if (isSnapshot_)
                {
                    // this is just a regular snaphot
                    mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);

                    // if its snapshot request     then notifiy that
                    if(0 != snapShot_)
                    {
                        snapShot_->OnMessage(msg_);
                        // now we have sent the message we dont need the reply any more
                        snapShot_.reset();
                    }
                }
                else if (isRefresh_ && isRefreshMsg)
                {
                    // test the isRefresh here because we may get an update between requesting the snapshot and receiving the reponse
                    mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);


                    // in the subscription stream
                    // send the message to each of the listeners
                    NotifyListenersRefreshMessage(msg_, subscription_);
                    isRefresh_= false;
                    subscription_.reset();


                }
                else
                {
                    // its just a regular subscription
                    // send the message to each of the listeners
                    NotifyListenersMessage(msg_, msgType);
                }

                // and clean up
                bookByOrderMessage_.EndUpdate(msg_);
            }
            else
            {
                ReportDecodeFailure("rsslDecodeMap()", ret );
                // t42log_warn("rsslDecodeMap() failed with return code: %d\n", ret);
                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                return RSSL_RET_SUCCESS;
            }

            break;
        }

    case RSSL_MC_STATUS:

        if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
        {
            rsslStateToString(&tempBuffer, &msg->statusMsg.state);
            t42log_debug("%s    %s\n\n", Symbol().c_str(), tempBuffer.data);
            SetRsslState(&msg->statusMsg.state);
        }

        break;

    default:
        t42log_warn("Received Unhandled Item Msg Class: %d\n", msg->msgBase.msgClass);
        break;
    }

    return RSSL_RET_SUCCESS;
}


// Process a market by price response message.
// The caller has decoded the message to the point where the message type is knowm
RsslRet UPASubscription::InternalProcessMarketByPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;
    UPAStreamManager &mgr = consumer_->StreamManager();

    if (GetSubscriptionState() == SubscriptionStateInactive)
    {
        // its been closed so do nothing
        t42log_debug("got response on closed item for %s on stream %d\n", symbol_.c_str(), streamId_);

        if (!gotInitial_)
        {
            t42log_debug("got response on closed item no initial - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
            mgr.removePendingItem(this);
        }
        return RSSL_RET_SUCCESS;
    }

    if (GetSubscriptionState() == SubscriptionStateSubscribing)
    {
        // response to a subscribe message so decrement the count

        t42log_debug("got response - dec pending items count for %s on stream %d\n", symbol_.c_str(), streamId_);
        mgr.removePendingItem(this);
        gotInitial_ = true;

    }

    char tempData[1024];
    RsslBuffer tempBuffer;
    tempBuffer.data = tempData;
    tempBuffer.length = 1024;

    bookByPriceMessage_.StartUpdate();
    RsslMsgKey* key = 0;

    RsslUInt8 updateType = 0;
    RsslUInt32 seqNum = 0;

    mamaMsgType msgType = MAMA_MSG_TYPE_UNKNOWN;

    // will need to know later of we were processing a refresh
    bool isRefreshMsg = false;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REFRESH:
        // update the item state
        SetRsslState(&msg->refreshMsg.state);
        isRefreshMsg = true;

        if (isRefresh_)
        {
            // Remove recap from pending items
            mgr.removePendingItem(this);
        }

        // then just fall thorugh

    case RSSL_MC_UPDATE:
        {

            key = (RsslMsgKey *)rsslGetMsgKey(msg);

            // log the symbol
            t42log_debug("%s - Domain: %s\n", symbol_.c_str(), rsslDomainTypeToString(msg->msgBase.domainType));

            // extract the sequence number according to the message type
            if (msg->msgBase.msgClass == RSSL_MC_UPDATE)
            {
                /* When displaying update information, we should also display the updateType information. */
                t42log_debug("UPDATE TYPE: %u\n", msg->updateMsg.updateType);
                updateType = msg->updateMsg.updateType;

                if (msg->refreshMsg.flags & RSSL_UPMF_HAS_SEQ_NUM)
                {
                    seqNum = msg->updateMsg.seqNum;
                }
            }

            if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
            {
                rsslStateToString(&tempBuffer, &msg->refreshMsg.state);
                t42log_debug("%s\n", tempBuffer.data);

                if (msg->refreshMsg.flags & RSSL_RFMF_HAS_SEQ_NUM)
                {
                    seqNum = msg->refreshMsg.seqNum;
                }
            }

            // bit nasty but we may need to fake an initial image if we have transitioned stale to live on an update
            bool fakeInitial = false;

            if (GetSubscriptionState() == SubscriptionStateStale && msg->refreshMsg.state.dataState == RSSL_DATA_OK)
            {
                // stale now live
                RsslBuffer stateBuffer;
                rsslStateToString(&stateBuffer, &msg->refreshMsg.state);
                if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
                {

                    t42log_debug("Stale to Live - Refresh %s\n", stateBuffer.data);
                }
                else
                {
                    // it was an update

                    // although feeds like IDN will always throw a new image (RSSL_MC_REFRESH) on a stale to live transition this cannot be guaranteed so
                    // ensure  the client will see the update as an image

                    t42log_debug("Stale to Live - No refresh %s\n", stateBuffer.data);
                    fakeInitial = true;
                }

                SetSubscriptionState(SubscriptionStateLive);
            }


            // force to initial if we have faked an image
            msgType = SetMessageType(fakeInitial? RSSL_MC_REFRESH: msg->msgBase.msgClass, updateType, true);

            // now walk the message
            RsslMap map = RSSL_INIT_MAP;
            RsslFieldList fList = RSSL_INIT_FIELD_LIST;
            RsslFieldEntry fEntry = RSSL_INIT_FIELD_ENTRY;

            RsslLocalFieldSetDefDb localFieldSetDefDb;  // this must be cleared using the clear function below
            rsslClearLocalFieldSetDefDb(&localFieldSetDefDb);

            UPAFieldDecoder decoder(consumer_, fieldmap_, symbol_, sourceName_);

            /* level 2 market by price is a map of field lists */
            if ((ret = rsslDecodeMap(dIter, &map)) == RSSL_RET_SUCCESS)
            {
                // decode set definition database
                if (map.flags & RSSL_MPF_HAS_SET_DEFS)
                {
                    // required to decode the field list
                    if (rsslDecodeLocalFieldSetDefDb(dIter, &localFieldSetDefDb) != RSSL_RET_SUCCESS)
                    {
                        ReportDecodeFailure("rsslDecodeLocalFieldSetDefDb()", ret );
                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // decode any summary data - this should be a field list depending on the domain model
                if (map.flags & RSSL_MPF_HAS_SUMMARY_DATA)
                {
                    t42log_debug("SUMMARY DATA\n");
                    if ((ret = rsslDecodeFieldList(dIter, &fList, &localFieldSetDefDb)) == RSSL_RET_SUCCESS)
                    {
                        // decode each field entry in the list
                        while ((ret = rsslDecodeFieldEntry(dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
                        {
                            if (ret == RSSL_RET_SUCCESS)
                            {
                                // decode each field
                                // these are regular fields so get decodeed into the mama message like any L1 fields
                                if (decoder.DecodeFieldEntry(&fEntry, dIter, msg_) != RSSL_RET_SUCCESS)
                                {
                                    ReportDecodeFailure("DecodeFieldEntry()", ret );
                                    // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                    return RSSL_RET_SUCCESS;
                                }
                            }
                            else
                            {
                                ReportDecodeFailure("rsslDecodeFieldEntry()", ret );
                                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                return RSSL_RET_SUCCESS;
                            }
                        }
                    }
                    else
                    {
                        ReportDecodeFailure("rsslDecodeFieldList()", ret );
                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // decode the map
                RsslMapEntry mapEntry = RSSL_INIT_MAP_ENTRY;
                RsslBuffer mapKey = RSSL_INIT_BUFFER;
                while ((ret = rsslDecodeMapEntry(dIter, &mapEntry, &mapKey)) != RSSL_RET_END_OF_CONTAINER)
                {
                    std::string pricePointKey(mapKey.data, mapKey.length);
                    if (ret == RSSL_RET_SUCCESS)
                    {

                        const char* actionString;
                        UPABookEntry_ptr_t entry = UPABookEntry_ptr_t(new UPABookEntry);
                        entry.use_count();

                        /* convert the action to a string for display purposes */
                        switch(mapEntry.action)
                        {
                        case RSSL_MPEA_UPDATE_ENTRY:
                            actionString = "RSSL_MPEA_UPDATE_ENTRY";
                            entry->ActionCode('U');
                            break;

                        case RSSL_MPEA_ADD_ENTRY:
                            actionString = "RSSL_MPEA_ADD_ENTRY";
                            entry->ActionCode('A');
                            break;

                        case RSSL_MPEA_DELETE_ENTRY:
                            actionString = "RSSL_MPEA_DELETE_ENTRY";
                            entry->ActionCode('D');
                            break;

                        default:
                            actionString = "Unknown";
                            entry->ActionCode('Z');
                        }

                        /* print out the key */
                        if (mapKey.length)
                        {
                            t42log_debug("ORDER ID: %.*s\nACTION: %s\n", mapKey.length, mapKey.data, actionString);
                        }

                        //char side = ExtractSideCode(mapKey);
      //                  // and set the side code into thge entry becuase the provider only includes it for a new level
      //                  entry->SideCode(::toupper(side));

                        // only have a field list when the type is not a delete
                        if (mapEntry.action != RSSL_MPEA_DELETE_ENTRY)
                        {

                            // decode the field list
                            if ((ret = rsslDecodeFieldList(dIter, &fList, &localFieldSetDefDb)) == RSSL_RET_SUCCESS)
                            {

                                // and each entry
                                while ((ret = rsslDecodeFieldEntry(dIter, &fEntry)) != RSSL_RET_END_OF_CONTAINER)
                                {
                                    if (ret == RSSL_RET_SUCCESS)
                                    {
                                        if (decoder.DecodeBookFieldEntry(&fEntry, dIter, entry) != RSSL_RET_SUCCESS)
                                        {
                                            t42log_warn("DecodeBookFieldEntry() failed\n");
                                            // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                            return RSSL_RET_SUCCESS;
                                        }
                                    }
                                    else
                                    {
                                        ReportDecodeFailure("rsslDecodeFieldEntry()", ret );
                                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                        return RSSL_RET_SUCCESS;
                                    }
                                }

                                if(mapEntry.action == RSSL_MPEA_ADD_ENTRY)
                                {
                                    // should create a new pricepoint
                                    //
                                    // may not be abale to assume the the message has a side code - if not extract it from the key and set it
                                    char side;
                                    if(!entry->HasSide())
                                    {
                                        side = ExtractSideCode(mapKey);
                                    }

                                    PricePoint_ptr_t pp = PricePoint_ptr_t(new PricePoint(entry->Price(), entry->SideCode()));
                                    PPMap_.insert(PricePointMap_t::value_type(pricePointKey, pp));
                                }

                                // if we havent decoded a side then should be able to get it from the price point map
                                if(!entry->HasSide())
                                {
                                    // we see this in data from the TR rssl Provider sample code, so for safety assume other sources might be similar
                                    PricePointMap_t::const_iterator itPricePoint = PPMap_.find(pricePointKey);
                                    if(itPricePoint != PPMap_.end())
                                    {
                                        const PricePoint& pricePoint = *(itPricePoint->second);
                                        entry->SideCode(pricePoint.SideCode());
                                    }
                                    else
                                    {
                                        // not much we can do about this
                                        ReportDecodeFailure("Failed to look up price point key", ret );
                                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                        return RSSL_RET_SUCCESS;
                                    }

                                }


                                if (mapEntry.action == RSSL_MPEA_ADD_ENTRY )
                                {
                                    bookByPriceMessage_.AddEntry(entry);
                                }
                                else if (mapEntry.action == RSSL_MPEA_UPDATE_ENTRY)
                                {
                                    // This is for OMM MarketByPrice models that update an existing entry with a new price.
                                    // Since OM uses the price as the key we need to send a delete before the update.
                                    PricePointMap_t::iterator itPricePoint = PPMap_.find(pricePointKey);
                                    if (itPricePoint != PPMap_.end())
                                    {
                                        // Existing price point
                                        PricePoint& pricePoint = *(itPricePoint->second);
                                        if (pricePoint.Price().value != entry->Price().value ||
                                            pricePoint.SideCode() != entry->SideCode())
                                        {
                                            // Check is there is an update for this on a different price point.
                                            // Only queue a delete if there is no preceeding update.
                                            bool found = false;
                                            const EntryList_t& entryList = bookByPriceMessage_.getEntryList();
                                            EntryList_t::const_iterator itEntry = entryList.begin();
                                            while (itEntry != entryList.end())
                                            {
                                                const UPABookEntry& bookEntry = *(*itEntry++);
                                                if (bookEntry.Price().value == pricePoint.Price().value &&
                                                    bookEntry.SideCode() == pricePoint.SideCode())
                                                {
                                                    found = true;
                                                    break;
                                                }
                                            }

                                            if (!found)
                                            {
                                                // The new one is different than the existing point, so queue a delete
                                                UPABookEntry_ptr_t delEntry = UPABookEntry_ptr_t(new UPABookEntry);
                                                delEntry->Price(pricePoint.Price());
                                                delEntry->SideCode(pricePoint.SideCode());
                                                delEntry->ActionCode('D');
                                                bookByPriceMessage_.AddEntry(delEntry);
                                            }

                                            // Now set the existing price point to the new data
                                            pricePoint.assign(*entry);
                                        }
                                    }
                                    bookByPriceMessage_.AddEntry(entry);
                                }

                            }
                            else
                            {
                                ReportDecodeFailure("rsslDecodeFieldList()", ret );
                                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                                return RSSL_RET_SUCCESS;
                            }
                        }
                        else
                        {
                            // do an entry delete

                            // look up the price Point
                            PricePointMap_t::const_iterator itPricePoint = PPMap_.find(pricePointKey);
                            if (itPricePoint != PPMap_.end())
                            {
                                const PricePoint& pricePoint = *(itPricePoint->second);
                                // still need to set the price point in the entry even if its a delete because the OMM message doesnt carry it in the payload
                                entry->Price(pricePoint.Price());
                                entry->SideCode(pricePoint.SideCode());
                                // and erase the price point from the map
                                PPMap_.erase(itPricePoint);

                                bookByPriceMessage_.AddEntry(entry);
                            }
                        }

                        t42log_debug("\n"); /* add a space between end of order and beginning of next order for readability */
                    }
                    else
                    {
                        ReportDecodeFailure("rsslDecodeMapEntry()", ret );
                        // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                        return RSSL_RET_SUCCESS;
                    }
                }

                // now render the OpenMama message
                bookByPriceMessage_.BuildMamdaMessage(msg_);

                // send the message
                setMsgNum(false);


                if (isSnapshot_)
                {
                    // this is just a regular snaphot
                    mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);

                    // if its snapshot request     then notify that
                    if(0 != snapShot_)
                    {
                        snapShot_->OnMessage(msg_);
                        // now we have sent the message we dont need the reply any more
                        snapShot_.reset();
                    }
                }
                else if (isRefresh_ && isRefreshMsg)
                {
                    // test the isRefresh here because we may get an update between requesting the refresh and receiving the response
                    mamaMsg_addI32(msg_, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);


                    // in the subscription stream
                    // send the message to each of the listeners
                    NotifyListenersRefreshMessage(msg_, subscription_);
                    isRefresh_= false;
                    subscription_.reset();


                }
                else
                {
                    // its just a regular subscription
                    // send the message to each of the listeners
                    NotifyListenersMessage(msg_, msgType);
                }

            }
            else
            {
                ReportDecodeFailure("rsslDecodeMap()", ret );
                //t42log_warn("rsslDecodeMap() failed with return code: %d\n", ret);
                // return RSSL_RET_SUCCESS otherwise it will shut down the thread
                return RSSL_RET_SUCCESS;
            }



            break;
        }

    case RSSL_MC_STATUS:

        if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
        {
            rsslStateToString(&tempBuffer, &msg->statusMsg.state);
            t42log_debug("%s    %s\n\n", Symbol().c_str(), tempBuffer.data);

            SetRsslState(&msg->statusMsg.state);
        }

        break;


    default:
        t42log_warn("Received Unhandled Item Msg Class: %d\n", msg->msgBase.msgClass);
        break;
    }

    return RSSL_RET_SUCCESS;
}

void UPASubscription::setLineTime()
{
    return;

}

void UPASubscription::SendStatusMsg(mamaMsgStatus secStatus)
{

    NotifyListenersStatus(secStatus);
}

void UPASubscription::NotifyListenersMessage( mamaMsg msg, mamaMsgType msgType )
{
    SubscriptionResponseListenersVector_t listenersSnap;
    {
        T42Lock l(&subscriptionLock_);
        // take a snap of the vector in case anything gets removed on the other thread
        listenersSnap = listeners_;
    }

    SubscriptionResponseListenersVector_t::iterator it = listenersSnap.begin();
    while(it != listenersSnap.end() )
    {
        RMDSBridgeSubscription_ptr_t sub = *it;
        it++;

        sub->OnMessage(msg, msgType);
    }
}


void UPASubscription::NotifyListenersRefreshMessage(mamaMsg msg, RMDSBridgeSubscription_ptr_t sub, bool bookMessage)
{
    // send an in-stream snapshot to all the listeners. The subscription that requested the refresh gets a msg type initial and
    // the rest get a message type recap.

    // We send a recap to all the other subscriptions on this stream in case the platform has conflated any updates into the snapshot.
    SubscriptionResponseListenersVector_t listenersSnap;
    {
        T42Lock l(&subscriptionLock_);
        // take a snap of the vector in case anything gets removed on the other thread
        listenersSnap = listeners_;
    }

    SubscriptionResponseListenersVector_t::iterator it = listenersSnap.begin();
    while(it != listenersSnap.end() )
    {
        // have to update the message type in here as this is where we know whether we are sending it to an existing subscription or one we have added to the stream
        RMDSBridgeSubscription_ptr_t listener = *it;
        it++;

        mamaMsgType msgType = MAMA_MSG_TYPE_UNKNOWN;
        if(sub == listener)
        {
            // Original/New subscriber
            msgType = bookMessage ? MAMA_MSG_TYPE_BOOK_INITIAL : MAMA_MSG_TYPE_INITIAL;
        }
        else
        {
            // Existing/Duplicate subscriber
            if (sendRecap_)
            {
                // Only send to existing subscribers if configured.
                // This is to prevent too many RECAPS for cases where there are lots of duplicate symbols.
                // TODO This may also not deliver updates that are conflated into the RECAP.
                msgType = bookMessage ? MAMA_MSG_TYPE_BOOK_RECAP : MAMA_MSG_TYPE_RECAP;
            }
        }

        if (msgType != MAMA_MSG_TYPE_UNKNOWN)
        {
            mamaMsg_updateU8(msg_, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, msgType);
            listener->OnMessage(msg, msgType);
        }
    }
}


void UPASubscription::NotifyListenersStatus( mamaMsgStatus statusCode )
{
    SubscriptionResponseListenersVector_t listenersSnap;
    {
        T42Lock l(&subscriptionLock_);
        // take a snap of the vector in case anything gets removed on the other thread
        listenersSnap = listeners_;
    }

    SubscriptionResponseListenersVector_t::iterator it = listenersSnap.begin();
    while(it != listenersSnap.end() )
    {
        RMDSBridgeSubscription_ptr_t sub = *it;
        it++;

        sub->OnStatusMessage(statusCode);
    }

}

void UPASubscription::NotifyListenersError( mama_status statusCode )
{
    /*    T42Lock l(&subscriptionLock_);
    SubscriptionResponseListenersVector_t::iterator it = listeners_.begin()*/;

    SubscriptionResponseListenersVector_t listenersSnap;
    {
        T42Lock l(&subscriptionLock_);
        // take a snap of the vector in case anything gets removed on the other thread
        listenersSnap = listeners_;
    }

    SubscriptionResponseListenersVector_t::iterator it = listenersSnap.begin();
    while(it != listenersSnap.end() )
    {
        RMDSBridgeSubscription_ptr_t sub = *it;
        it++;
        sub->OnError(statusCode);
    }
}

void UPASubscription::NotifyListenersQuality(mamaQuality quality, short cause)
{
    SubscriptionResponseListenersVector_t listenersSnap;
    {
        T42Lock l(&subscriptionLock_);
        // take a snap of the vector in case anything gets removed on the other thread
        listenersSnap = listeners_;
    }

    SubscriptionResponseListenersVector_t::iterator it = listenersSnap.begin();
    while (it != listenersSnap.end())
    {
        RMDSBridgeSubscription_ptr_t sub = *it;
        it++;
        sub->OnQuality(quality, cause);
    }
}

void UPASubscription::AddListener( RMDSBridgeSubscription_ptr_t listener )
{
    T42Lock l(&subscriptionLock_);
    listeners_.push_back(listener);
}

void UPASubscription::RemoveListener( RMDSBridgeSubscription_ptr_t listener )
{
    T42Lock l(&subscriptionLock_);
    SubscriptionResponseListenersVector_t::iterator it;

    for(it = listeners_.begin(); it != listeners_.end(); it ++)
    {

        if ((*it).get() == listener.get())
        {
            listeners_.erase(it);
            break;
        }
    }
}

void UPASubscription::RemoveListener( RMDSBridgeSubscription * listener )
{
    T42Lock l(&subscriptionLock_);
    SubscriptionResponseListenersVector_t::iterator it;

    for(it = listeners_.begin(); it != listeners_.end(); it ++)
    {

        if ((*it).get() == listener)
        {
            listeners_.erase(it);
            break;
        }
    }
}


// Request an image for this subscription
void UPASubscription::RequestImage(RMDSBridgeSubscription_ptr_t sub)
{
    T42Lock l(&subscriptionLock_);
    // We obtain a new image by making a refresh request to the rmds
    Refresh(consumer_, sub);
}



const RsslUInt32 DecodeFailureReportInterval = 60 * 1000;  // 1 minute

void UPASubscription::ReportDecodeFailure( const char * location, RsslRet errCode )
{

    RsslUInt32 now = utils::time::GetMilliCount();
    // first decide whether to log a report
    if (numDecodeFailuresLast_  != 0 )
    {
        // its not the first so check the time since the last
        if (timeLastReport_ != 0 && (now - timeLastReport_) < DecodeFailureReportInterval )
        {
            ++numDecodeFailures_;
            // don't report now
            return;
        }
    }

    // so its either the first instance of a decode failure or is longer than the report interval
    if (numDecodeFailures_ == 0)
    {
        // first time
        t42log_warn("Rssl message decode error for %s (%s) : error code %d \n", symbol_.c_str(), location, errCode);
    }
    else
    {
        // report count and interval
        RsslUInt64 errorCount = numDecodeFailures_ - numDecodeFailuresLast_;
        RsslUInt32 interval = (now - timeLastReport_)/1000;
        t42log_warn("Rssl message decode error for %s occurred %d times in last %d s \n", symbol_.c_str(), errorCount, interval);
    }

    // and reset
    numDecodeFailuresLast_ = ++numDecodeFailures_;
    timeLastReport_ = now;
}




void UPASubscription::SetDomain( UPASubscriptionType domain )
{
    // map the domain. we have separate enums for the source property and the subscription property. This is historic. It might be useful to keep them separate

    if(domain == SubscriptionTypeUnknown)
    {
        // in this initial implementation we are just looking at the initial letter - 's' for book by price, 'b' for book by order otherwise just treat as MarketPrice
        if (symbol_[0] == 's')
        {
            symbol_ = symbol_.substr(1,symbol_.length()-1);
            subscriptionType_ = SubscriptionTypeMarketByPrice;
        }
        else if (symbol_[0] == 'b')
        {
            symbol_ = symbol_.substr(1,symbol_.length()-1);
            subscriptionType_ = SubscriptionTypeMarketByOrder;
        }
        else
        {
            symbol_ = symbol_;
            subscriptionType_ = SubscriptionTypeMarketPrice;
        }
    }
    else
    {
        subscriptionType_ = domain;
    }

}

void UPASubscription::CompleteSnapshot()
{
    // snapshot is completed. release stream
    UPAStreamManager &mgr = consumer_->StreamManager();
    mgr.ReleaseStreamId(streamId_);

    // and clear down mama message
    mamaMsg_clear(msg_);

}


// virtual stubs for enhanced functions
void UPASubscription::ProcessMFeedMessage(RsslMsg * msg)
{

    // just report that mfeed is not supported once
    if(!reportedMFeedNotSupported_)
    {
        t42log_warn("Processing of marketFeed messages requires the Tick42 Enhanced version of the bridge - please contact support@tick42.com\n");
        reportedMFeedNotSupported_ = true;
    }
    ReportDecodeFailure("DecodeFieldEntry()", RSSL_RET_UNSUPPORTED_DATA_TYPE );
}


void UPASubscription::ProcessAnsiMessage(RsslMsg * msg)
{

    // just report that ansi is not supported once
    if(!reportedAnsiNotSupported_)
    {
        t42log_warn("Processing of ANSI messages requires the Tick42 Enhanced version of the bridge - please contact support@tick42.com\n");
        reportedAnsiNotSupported_ = true;
    }
    ReportDecodeFailure("DecodeFieldEntry()", RSSL_RET_UNSUPPORTED_DATA_TYPE );
}


// wrappers for the rssl message processing functions
RsslRet UPASubscription::ProcessMarketPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = InternalProcessMarketPriceResponse(msg, dIter);
    mamaMsg_clear(msg_);
    return ret;
}

RsslRet UPASubscription::ProcessMarketByOrderResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = InternalProcessMarketByOrderResponse(msg, dIter);
    mamaMsg_clear(msg_);
    return ret;
}

RsslRet UPASubscription::ProcessMarketByPriceResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslRet ret = InternalProcessMarketByPriceResponse(msg, dIter);
    mamaMsg_clear(msg_);
    return ret;
}

char UPASubscription::ExtractSideCode(RsslBuffer &mapKey)
{
    // key is price as a string appended with 'a' or 'b' for the side
    // These are seen in LSE data
    // 227.15B
    // 222.60B:UBSL
    // 231.80A+A
    // 231.80A
    // 231.80A:UBSL
    // 231.90A:UBSL
    // 232.90A:PEEL
    // 221.95B:SCAP
    // 233.30A:SCAP
    // 223.25B:WINS
    // These are seen in TSE data
    // .<price>A. for Limit Orders ASK side
    // .<price>B. for Limit Orders BID side
    // .A-M.  for Market Orders ASK side
    // .B-M.  for Market Orders BID side
    // .<price>A+C.  for .OnClose. limit order ASK side
    // .<price>B+C.  for .OnClose. limit order BID side
    // .A-M+C.  for .OnClose. Market Orders ASK side
    // .B-M+C.  for .OnClose.Market Orders BID side
    // So, get the first char after the price and use that
    // Default to the last char, then look for an A or B
    char side = mapKey.data[mapKey.length-1];
    int numLen = 0;
    for (unsigned int i = 0; i < mapKey.length; ++i) {
        if (::toupper(mapKey.data[i]) == 'A' ||
            ::toupper(mapKey.data[i]) == 'B') {
                side = mapKey.data[i];
                break;
        }
        ++numLen;
    }

    return side;
}

bool UPASubscription::FindListener(RMDSBridgeSubscription * listener, RMDSBridgeSubscription_ptr_t & listenerPtr)
{
    T42Lock l(&subscriptionLock_);
    SubscriptionResponseListenersVector_t::iterator it;

    RMDSBridgeSubscription_ptr_t ret;


    for(it = listeners_.begin(); it != listeners_.end(); it ++)
    {

        if ((*it).get() == listener)
        {
            listenerPtr = *it;
            return true;
        }
    }
    return false;
}

void MAMACALLTYPE UPASubscription::SubscriptionDestroyCb(mamaQueue queue,void *closure)
{
    // Should queue it back on to the bridge's queue
    RMDSBridgeSubscriptionClosure * cl = (RMDSBridgeSubscriptionClosure *)closure;
    RMDSBridgeSubscription_ptr_t sub = cl->GetPtr();

    mamaQueue_enqueueEvent(sub->Queue(), RMDSBridgeSubscription:: SubscriptionDestroyCb, closure);

}

void UPASubscription::QueueSubscriptionDestroy(RMDSBridgeSubscription_ptr_t & sub)
{
    if(0 != consumer_)
    {
        // create closure to carry a boost shared pointer to this on the queue and ensure it doesn't get deleted while queued
        RMDSBridgeSubscriptionClosure * closure = new RMDSBridgeSubscriptionClosure(sub);
        mamaQueue_enqueueEvent(consumer_->RequestQueue(),  UPASubscription::SubscriptionDestroyCb, (void*) closure);
    }

}

UPASubscription::RMDSBridgeSubscriptionClosure::~RMDSBridgeSubscriptionClosure()
{
    sub_.reset();
}
