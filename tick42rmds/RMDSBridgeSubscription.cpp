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
#include "UPAMamaCommonFields.h"
#include "RMDSSubscriber.h"
#include "RMDSBridgeSubscription.h"
#include "RMDSSource.h"

#include "UPASubscription.h"
#include "utils/t42log.h"

RMDSBridgeSubscription::RMDSBridgeSubscription(void)
{
}

RMDSBridgeSubscription::RMDSBridgeSubscription( const std::string& sourceName, const std::string& symbol, mamaTransport transport, mamaQueue queue,
    mamaMsgCallbacks callback, mamaSubscription subscription, void* closure, bool logRmdsValues)
    : transport_(transport), queue_(queue), callback_(callback), subscription_(subscription), closure_(closure), logRmdsValues_(logRmdsValues), 
        sourceName_(sourceName), symbol_(symbol), gotImage_(false), isShutdown_(false) 

{

}

RMDSBridgeSubscription::~RMDSBridgeSubscription(void)
{
}

void RMDSBridgeSubscription::SendStatusMessage( mamaMsgStatus secStatus )
{
    if (isShutdown_)
    {
        // just don't bother making the callback
        return;
    }
    // create and send a message containing the (failed) status
    mamaMsg msg;
    mamaMsg_createForPayload(&msg, MAMA_PAYLOAD_TICK42RMDS);

    mamaMsg_addI32(msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_SEC_STATUS);
    mamaMsg_addI32(msg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, secStatus);

    const CommonFields &commonFields = UpaMamaCommonFields::CommonFields();
    mamaMsg_addString(msg, commonFields.wIssueSymbol.mama_field_name.c_str(),commonFields.wIssueSymbol.mama_fid, symbol_.c_str());
    mamaMsg_addString(msg, commonFields.wSymbol.mama_field_name.c_str(), commonFields.wSymbol.mama_fid, symbol_.c_str());

    mama_status status = MAMA_STATUS_OK;
    try
    {
        status = mamaSubscription_processMsg(subscription_, msg);
    }
    catch (...)
    {
        mamaMsg_destroy(msg);    
        t42log_error("RMDSBridgeSubscription::OnMessage - caught exception calling mamaSubscription_processMsg for %s", symbol_.c_str());
    }


    if (MAMA_STATUS_OK != status)
    {
        mama_log (MAMA_LOG_LEVEL_ERROR,
            "RMDSBridgeSubscription::OnMessage: "
            "mamaSubscription_processMsg() failed. [%d]",
            status);
    }

    mamaMsg_destroy(msg);    
}

// SubscriptionResponseListener implementation

void RMDSBridgeSubscription::OnMessage( mamaMsg msg, mamaMsgType msgType )
{
    if (isShutdown_ || ((0 != source_) && source_->IsPausedUpdates()))
    {
        // just don't bother with making the callback
        return;
    }

    // send the message
    mama_status status = MAMA_STATUS_OK;

    if (msgType == MAMA_MSG_TYPE_INITIAL || msgType == MAMA_MSG_TYPE_REFRESH || msgType == MAMA_MSG_TYPE_RECAP || msgType == MAMA_MSG_TYPE_BOOK_INITIAL || msgType == MAMA_MSG_TYPE_BOOK_RECAP)
    {
        gotImage_ = true;
    }
    else if (msgType == MAMA_MSG_TYPE_SEC_STATUS)
    {
        // Let status go through
    }
    else
    {
        if (!gotImage_)
        {
            t42log_debug("Ignoring Update type %d with no image for %s : %s \n", msgType, symbol_.c_str(), sourceName_.c_str() );
            return;
        }
    }

    try
    {
        status = mamaSubscription_processMsg(subscription_, msg);
    }
    catch (...)
    {
        t42log_error("RMDSBridgeSubscription::OnMessage - caught exception calling mamaSubscription_processMsg for %s", symbol_.c_str());
    }

    if (MAMA_STATUS_OK != status)
    {
        mama_log (MAMA_LOG_LEVEL_ERROR,
            "RMDSBridgeSubscription::OnMessage: "
            "mamaSubscription_processMsg() failed. [%d]",
            status);
    }
}

void RMDSBridgeSubscription::OnError( mama_status statusCode )
{
    if (isShutdown_)
    {
        // just dont bother with making the callback
        return;
    }

    char * subject = (char*)alloca(1024);
    sprintf(subject,"%s.%s",SourceName().c_str(), Symbol().c_str());

    try
    {
        callback_.onError(subscription_, statusCode, 0, subject, closure_);
    }
    catch (...)
    {
        t42log_error("RMDSBridgeSubscription::OnError - caught exception calling mama subscription onError for %s", symbol_.c_str());
    }

}

void RMDSBridgeSubscription::OnStatusMessage( mamaMsgStatus statusCode )
{
    SendStatusMessage(statusCode);
}

void RMDSBridgeSubscription::OnQuality(mamaQuality quality, short cause)
{
    try
    {
        if (callback_.onQuality)
        {
            callback_.onQuality(subscription_, quality, Symbol().c_str(), cause, NULL, closure_);
        }
    }
    catch (...)
    {
        t42log_error("RMDSBridgeSubscription::OnQuality - caught exception calling mama subscription OnQuality for %s", symbol_.c_str());
    }
}

void MAMACALLTYPE RMDSBridgeSubscription::SubscriptionDestroyCb(mamaQueue queue,void *closure)
{

    UPASubscription::RMDSBridgeSubscriptionClosure * cl = (UPASubscription::RMDSBridgeSubscriptionClosure *)closure;
    RMDSBridgeSubscription_ptr_t sub = cl->GetPtr();
    wombat_subscriptionDestroyCB destroyCb = sub->Callback().onDestroy;
    mamaSubscription parent = sub->Subscription();

//    sub->RemoveSubscription(pSubscription);


    //Invoke the subscription callback to inform that the bridge has been
    //destroyed.
    if (NULL != destroyCb)
        (*(wombat_subscriptionDestroyCB)destroyCb)(parent, sub->Closure());

    sub.reset();
    delete cl;

}

bool RMDSBridgeSubscription::Open( UPAConsumer_ptr_t consumer )
{
    if (UpaSubscription_->GetSubscriptionState() == UPASubscription::SubscriptionStateInactive)
    {
        UpaSubscription_->Open(consumer);
    }

    return true;
}

void RMDSBridgeSubscription::Shutdown()
{
    // this will block any new callback calls
    isShutdown_= true;
}





//////////////////////////////
//
// Snapshot implementation
//
///////////////////////////////
RMDSBridgeSnapshot::RMDSBridgeSnapshot( SnapshotReply_ptr_t snap, bool logRMDSValues)
    : reply_(snap), logRMDSValues_(logRMDSValues)
{

}

void RMDSBridgeSnapshot::OnMessage( mamaMsg msg )
{

    reply_->OnMsg(msg);

    subscription_->CompleteSnapshot();
}

void RMDSBridgeSnapshot::OnError( mama_status statusCode )
{

}

void RMDSBridgeSnapshot::OnStatusMessage( mamaMsgStatus statusCode )
{

}

std::string RMDSBridgeSnapshot::SourceName() const
{
    return reply_->SourceName();
}

std::string RMDSBridgeSnapshot::Symbol() const
{
    return reply_->Symbol();
}
