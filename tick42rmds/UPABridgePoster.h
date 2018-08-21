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
#pragma once
#if !defined(__UPABRIDGEPOSTER_H__)
#define __UPABRIDGEPOSTER_H__

#include <list>
#include "RMDSSubscriber.h"
#include "transportconfig.h"
#include "UPAMamaFieldMap.h"


// Reply structure for posted messages
//
// This isnt really part of mama but RMDS will respond to posted messages with an ACK/NAK. If the posted messages is sent from an inbox
// then the bridge will send an ACK/NAK reply to the inbox.
//
// There are proposals for a callback mechanism in the mama publisher for addressing this behaviour. When this is available the bridge
// can be modified to use it
class PublisherPostMessageReply
{
public:
    PublisherPostMessageReply( const std::string & replyAddr, mamaInbox inbox)
        : replyAddr_(replyAddr), inbox_(inbox)
    {}

    mamaInbox Inbox() const
    {return inbox_;}

    std::string ReplyAddr() const
    {return replyAddr_;}

private:
    std::string replyAddr_;
    mamaInbox inbox_;
};

typedef boost::shared_ptr<PublisherPostMessageReply> PublisherPostMessageReply_ptr_t;

// The UPABridgePublisher represents the generic mamaPublisher
// derived class UPABridgePoster implements this as an RMDS posted message and UPABridgePublisherItem implements as a provider message (interactive and non iteractive)

class UPABridgePublisher : public boost::enable_shared_from_this<UPABridgePublisher>
{
public:
    UPABridgePublisher(const std::string &root, const std::string &sourceName, const std::string &symbol, mamaTransport transport, mamaPublisher Parent);
    virtual ~UPABridgePublisher();

    static UPABridgePublisher_ptr_t CreatePublisher(const std::string & root, const std::string& sourceName, const std::string& symbol, mamaTransport transport, mamaPublisher parent);

    virtual bool Initialise(const UPABridgePublisher_ptr_t& shared_ptr);
    virtual void Shutdown();

    // Accessors
    mamaTransport Transport() const {return transport_;}
    std::string Symbol() const {return symbol_;}
    std::string Subject() const {return subject_;}
    std::string Root() const {return root_;}

    mama_status BuildSendSubject ();

    virtual mama_status PublishMessage(mamaMsg msg);
    virtual mama_status PublishMessage(mamaMsg msg, const PublisherPostMessageReply_ptr_t& reply);

    void setUserCallbacks(const mamaPublisherCallbacks& userCallbacks, void* closure);

    void raiseOnCreate();
    void raiseOnDestroy();
    void raiseOnError(mama_status status, const char* info);

protected:
    std::string root_;
    std::string sourceName_;
    std::string symbol_;
    std::string subject_;

    mamaTransport transport_;
    mamaPublisher parent_;
    RsslUInt32 pubId_;

    UPABridgePublisher_ptr_t sharedPtr_;

    mamaPublisherCallbacks userCallbacks_;
    void* userCallbacksClosure_;
};

// queued item for dispatching poster message
class PostMessageRequest
{
public:
    PostMessageRequest(const UPABridgePoster_ptr_t& poster, mamaMsg msg, const PublisherPostMessageReply_ptr_t& reply = PublisherPostMessageReply_ptr_t())
        : poster_(poster), msg_(msg), reply_(reply)
    {}

    UPABridgePoster_ptr_t Poster() const {return poster_;}

    mamaMsg Message() const {return msg_;}

    const PublisherPostMessageReply_ptr_t& Reply() const {return reply_;}

private:
    UPABridgePoster_ptr_t  poster_;
    mamaMsg msg_;
    PublisherPostMessageReply_ptr_t reply_;
};

// queued item for dispatching publisher message
class PublishMessageRequest
{
public:
    PublishMessageRequest(const UPABridgePublisherItem_ptr_t& publisher, mamaMsg msg)
        : publisher_(publisher), msg_(msg)
    {
    }

    UPABridgePublisherItem_ptr_t Publisher() const
    {
        return publisher_;
    }

    mamaMsg Message() const
    {
        return msg_;
    }


private:
    UPABridgePublisherItem_ptr_t  publisher_;
    mamaMsg msg_;
};

typedef struct FieldListClosure
{
    UPABridgePublisher * publisher;
    RsslFieldList * fieldList;
    RsslEncodeIterator * itEncode;
} FieldListClosure_t;

class UPABridgePoster : public UPABridgePublisher
{
public:

    // static factory
    static UPABridgePublisher_ptr_t CreatePoster(const std::string &root, const std::string &sourceName,
        const std::string& symbol, mamaTransport transport, mamaPublisher parent,
        const RMDSSubscriber_ptr_t& subscriber, const TransportConfig_t& config);

    UPABridgePoster(const std::string &root, const std::string &sourceName, const std::string &symbol,
        mamaTransport transport, mamaPublisher Parent, const RMDSSubscriber_ptr_t& subscriber);
    virtual ~UPABridgePoster();

    virtual void Shutdown();

    virtual mama_status PublishMessage(mamaMsg msg);

    virtual mama_status PublishMessage(mamaMsg msg, const PublisherPostMessageReply_ptr_t& reply);

        // request post message
    static void MAMACALLTYPE PostMessageRequestCb(mamaQueue, void * closure);
    bool RequestPostMessage(mamaQueue requestQueue, PostMessageRequest * req);
    bool DoPostMessage(mamaMsg msg, const PublisherPostMessageReply_ptr_t& reply);

    void AlwaysOnStream(const char *val)
    {
        alwaysOnStream_ = val;
    }

    const char *AlwaysOnStream() const
    {
        return alwaysOnStream_.c_str();
    }

   bool UseSeqNum() const
   {
      return useSeqNum_;
   }

    // iterate the mama message
    static void MAMACALLTYPE mamaMsgIteratorCb(const mamaMsg msg, const mamaMsgField field, void* closure);

    // process field - called into from callback
    void EncodeField(RsslFieldList * fieldList, RsslEncodeIterator * itEncode, const mamaMsgField field);

    RsslRet ProcessAck(RsslMsg* msg, RsslDecodeIterator* dIter, PublisherPostMessageReply* reply);

protected:
   RsslRet PostStatusMessage(RsslChannel *chnl, mamaMsgStatus msgStatus, mamaMsg msg);
   RsslRet EncodeStatusMessage(RsslEncodeIterator &itEncode, mamaMsgStatus msgStatus, mamaMsg msg);
   RsslBuffer *AllocateEncoder(RsslChannel *chnl, RsslEncodeIterator &itEncode);
   RsslRet StartEncoding(RsslEncodeIterator &itEncode, RsslMsg *rsslMsg);
   RsslRet FinishEncoding(RsslEncodeIterator &itEncode);
   RsslRet SetState(RsslState &state, mamaMsgStatus msgStatus, mamaMsg msg);
   RsslRet SetMsgKey(RsslMsgKey &msgKey);
   RsslRet SetPostUserInfo(RsslPostUserInfo &postUserInfo);

private:

    virtual bool Initialise(const UPABridgePoster_ptr_t& shared_ptr, const TransportConfig_t& config);
    void PrintMsg(RsslBuffer* buffer);

    RMDSSubscriber_ptr_t subscriber_;

   // Should we add

   // If true, then copies the Mama SeqNum from the incoming post to RSSL
   bool useSeqNum_;

    // if true then it is an error if there is not stream for this source/symbol
    std::string alwaysOnStream_;

    // true to post on stream, false to post off stream
    bool postOnStream_;

    UPASubscription_ptr_t onStreamSubscription_;

    RsslUInt32 streamId_;

    RsslUInt32 serviceId_;

    mamaMsgStatus lastMsgStatus_;
    std::string statusText_;

    // cant get service id till the subscriber is live, so flag when we have it
    bool gotServiceId_;

    // insert postId into message so that the ack can be matched to the send
    RsslUInt32 nextPostId_;

    bool BuildPostMessage(RsslChannel * chnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg, const PublisherPostMessageReply_ptr_t& reply);

    bool EncodeMessageData(RsslChannel * chnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg);

    RsslRet EncodeInt32Field(RsslChannel * chnl, RsslBuffer * rsslMessagebuffer, RsslInt32 val);
    RsslRet EncodeUInt32Field(RsslChannel * chnl, RsslBuffer * rsslMessagebuffer, RsslUInt32 val);
    RsslRet EncodeInt64Field(RsslChannel * chnl, RsslBuffer * rsslMessagebuffer, RsslInt64 val);
    RsslRet EncodeUInt64Field(RsslChannel * chnl, RsslBuffer * rsslMessagebuffer, RsslUInt64 val);


    mamaDictionary mamaDictionary_;
    UpaMamaFieldMap_ptr_t upaFieldMap_;
    RsslDataDictionary * rmdsDictionary_;

    UPABridgePoster_ptr_t sharedPtr_;

    RsslRealHints MamaPrecisionToRsslHint(mamaPricePrecision p,  uint16_t fid);
    mamaMsgStatus RsslNakCode2MamaMsgStatus(RsslUInt8 nakCode);
    mama_status RsslNakCode2MamaStatus(RsslUInt8 nakCode);

    mutable utils::thread::lock_t inFlightListLock_;

    typedef std::list<RsslUInt32> PostIDList_t;
    PostIDList_t inFlightPosts_;

    bool useCallbacks_;
    bool sendAckMessages_;
    unsigned int maxMessageSize_;
};

class UPABridgePublisherItem :
    public UPABridgePublisher
{
public:

        // static factory
    static UPABridgePublisherItem_ptr_t CreatePublisherItem(const std::string &root, const std::string &sourceName,
                                                            const std::string &symbol, mamaTransport transport, mamaPublisher parent,
                                                            const RMDSPublisherBase_ptr_t& RMDSPublisher, TransportConfig_t config, bool interactive);

    UPABridgePublisherItem(const std::string &root, const std::string &sourceName, const std::string &symbol, mamaTransport transport,
                           mamaPublisher parent, const RMDSPublisherBase_ptr_t& RMDSPublisher)
        : UPABridgePublisher(root, sourceName, symbol, transport, parent), RMDSPublisher_(RMDSPublisher)
    {
    }

    virtual ~UPABridgePublisherItem()
    {
    }

    virtual mama_status PublishMessage(mamaMsg msg);
    void ProcessRecapMessage(mamaMsg msg);

    virtual void Shutdown();

private:
    virtual bool Initialise(const UPABridgePublisherItem_ptr_t& shared_ptr, const TransportConfig_t& config, bool interactive);

    UPABridgePublisherItem_ptr_t sharedPtr_;

    // move the message onto the provider thread
    // request publish message
    static void MAMACALLTYPE PublishMessageRequestCb(mamaQueue, void * closure);
    bool RequestPublishMessage(mamaQueue requestQueue, PublishMessageRequest * req);
    mama_status DoPublishMessage(mamaMsg msg);


    RMDSPublisherBase_ptr_t RMDSPublisher_;

    UPAPublisherItem_ptr_t UPAItem_;

    static void MAMACALLTYPE InboxOnMessageCB(mamaMsg msg, void *closure);
    void InboxOnMessage(mamaMsg msg);
    mamaInbox inbox_;
};

#endif // __UPABRIDGEPOSTER_H__
