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
#include "rmdsBridgeTypes.h"
#include "RMDSBridgeImpl.h"
#include "RMDSSubscriber.h"
#include "UPABridgePoster.h"

#include "inbox.h"
#include "ToMamaFieldType.h"
#include "transport.h"
#include "DictionaryReply.h"
#include "transportconfig.h"
#include "msg.h"


typedef struct _dict_closure_t
{
    mamaMsg msg;
    mamaInbox inbox;

} dict_closure_t;

typedef struct _dictionaryReplyClosure
{
    mamaInbox       inbox;   //the inbox used for sending
    mamaMsg         msg;     //A prepared dictionary message
} dictionaryReplyClosure;

struct Attributes;


#define MAX_SUBJECT_LENGTH    256

// This is the mama dq publisher client management topic
#define MAMA_CM_TOPIC       "__MAMA_CM"

#define UPABridgePub(publisher)((UPABridgePublisher*)(publisher))

#define CHECK_PUBLISHER(publisher) \
    do {  \
    if (UPABridgePub(publisher) == 0) return MAMA_STATUS_NULL_ARG; \
    } while(0)



const char DATA_DICT_TOPIC[] = "DATA_DICT";
char WOMBAT_SUBJECT[256];
bool dictSubjectSet = false;


//=========================================================================
// =                         Mandatory Functions                            =
// =========================================================================

mama_status
    tick42rmdsBridgeMamaPublisher_createByIndex (publisherBridge*  result,
    mamaTransport     tport,
    int               tportIndex,
    const char*       topic,
    const char*       source,
    const char*       root,
    mamaPublisher     parent)
{
    std::string strTopic;
    if (topic != 0)
    {
        strTopic = topic;
    }

    // get hold of the bridge implementation
    mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(tport);
    if (!bridgeImpl) {
        mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeCreateDictionary(): Could not get bridge");
        return MAMA_STATUS_PLATFORM;
    }

    RMDSBridgeImpl*      rmdsBridge = NULL;
    mama_status status;
    if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &rmdsBridge)))
    {
        mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeCreateDictionary(): Could not get rmds bridge object");
        return status;
    }

    const char* tportName;
    mamaTransport_getName(tport, &tportName);

    RMDSTransportBridge_ptr_t bridge = rmdsBridge->getTransportBridge(tportName);

    if (bridge.get() != 0)
    {
        std::string name = bridge->Name();
        TransportConfig_t config(name);
        if (!dictSubjectSet)
        {
            std::string dictionarySource= config.getString("dictsource");

            if (dictionarySource.length() == 0)
            {
                dictionarySource = "WOMBAT";
            }
            sprintf(WOMBAT_SUBJECT,"_MDDD.%s", dictionarySource.c_str());
            t42log_info("using dictionary source %s \n", WOMBAT_SUBJECT);
            dictSubjectSet = true;
            // create the string from properties
        }

        // Get the source name; it could be in the source argument or it could be part of the topic
        std::string srcName;
        std::string topicName;

        if (source != 0 && *source != 0)
        {
            // the source arg is not null or empty
            srcName = source;
            topicName = topic;
        }
        else
        {
            // if we need a source its everything to the left of the "." in the topic
            std::string topicStr(topic);
            size_t pos = topicStr.find_first_of(".");

            if (pos != std::string::npos)
            {
                srcName = std::string(topic,pos);
                topicName = std::string(topic).substr(pos+1, std::string::npos);
            }
        }

        UPABridgePublisher * pub;

        // only want to do this if it's a publisher rather than a subscription request for initial (or data dictionary)
        if (root == NULL)
        {
            utils::properties pubconfig;
            std::string sourcePropBase = "mama.tick42rmds.transport." + std::string(tportName) + "." + srcName;
            std::string pubType = pubconfig.get(sourcePropBase + ".pubtype", "post");

            if (::strcasecmp(MAMA_CM_TOPIC, topic) == 0)
            {
                // it's the client management topic
                pub  = new UPABridgePublisher("", "", topic, tport, parent);
            }
            else if (::strcasecmp("post", pubType.c_str()) == 0)
            {
                // create a "poster"
                RMDSSubscriber_ptr_t sub = bridge->Subscriber();
                UPABridgePublisher_ptr_t p = UPABridgePoster::CreatePoster("", srcName, topic, tport, parent, sub, config);
                pub = p.get();
            }
            else if (::strcasecmp("ni", pubType.c_str()) == 0)
            {
                // create a NI Publisher

                RMDSPublisherBase_ptr_t RMDSNIPub = bridge->NIPublisher();

                if (RMDSNIPub.get() == 0)
                {
                    t42log_error("attempt to create non-interactive publisher on source %s but no bridge publisher created - was post intended but not configured? \n", srcName.c_str());
                    *result = 0;
                    return MAMA_STATUS_PLATFORM;
                }

                UPABridgePublisherItem_ptr_t item = UPABridgePublisherItem::CreatePublisherItem("",  srcName, topicName, tport, parent, RMDSNIPub, config, false);
                pub  = item.get();
            }
            else
            {
                // create an interactive publisher
                RMDSPublisherBase_ptr_t RMDSPub = bridge->Publisher();

                if (RMDSPub.get() == 0)
                {
                    t42log_error("attempt to create publisher on source %s but no bridge publisher created - was post intended but not configured? \n", srcName.c_str());
                    *result = 0;
                    return MAMA_STATUS_PLATFORM;

                }
                UPABridgePublisherItem_ptr_t item = UPABridgePublisherItem::CreatePublisherItem("", srcName, topicName, tport, parent, RMDSPub, config, true);
                pub  = item.get();
            }
        }
        else
        {
            // this publisher is used for internal publishing (dictionary and image requests)
            pub = new UPABridgePublisher(root, srcName, strTopic, tport, parent);
        }

        pub->BuildSendSubject();

        *result = (publisherBridge) pub;
    }

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsBridgeMamaPublisher_send (publisherBridge publisher, mamaMsg msg)
{
    mama_size_t        dataLen=0;
    mama_status        status;
    Attributes* attributes = NULL;

    CHECK_PUBLISHER(publisher);

    UPABridgePublisher * pub = UPABridgePub(publisher);

    status = mamaMsgImpl_getPayloadBuffer (msg, (const void**)&attributes, &dataLen);

    if (attributes == NULL)
        return MAMA_STATUS_INVALID_ARG;


    // log the content of the message
    if (mama_getLogLevel() >= MAMA_LOG_LEVEL_FINEST)
    {
        const char *diagMsg = mamaMsg_toString(msg);
        t42log_debug ("tick42rmdsBridgeMamaPublisher_send(): "
            "Send message. %s\n", diagMsg);
    }

    // extract descriptive stuff from the message
    mama_i32_t msgType = MAMA_MSG_TYPE_UNKNOWN;
    mama_i32_t subscType = -1;
    mama_i32_t subscMsgType = -1;

    mamaMsg_getI32(msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid,&msgType);
    mamaMsg_getI32(msg, MamaFieldSubscriptionType.mName, MamaFieldSubscriptionType.mFid,&subscType);
    mamaMsg_getI32(msg, MamaFieldSubscMsgType.mName, MamaFieldSubscMsgType.mFid,&subscMsgType);

    if (mama_getLogLevel() >= MAMA_LOG_LEVEL_FINEST)
    {

        t42log_debug ("tick42rmdsBridgeMamaPublisher_send(): "
            "subscType =  %d, subscMsgType =  %d, msgType =  %d\n,", subscType,subscMsgType, msgType);

    }


    // if the subscription type field is -1  or MAMA_SUBSC_TYPE_NORMAL then this is not a getinitial request for a subscription for a symbol or dictionary
    // assume it is real pub request. There is some strangeness here because some bridges publish messages with subscriptiontype unknown (which is probably correct) while
    // wmw set it to MAMA_SUBSC_TYPE_NORMAL
    if (subscType <= MAMA_SUBSC_TYPE_NORMAL)
    {
        mama_status status = ((UPABridgePublisher *)publisher)->PublishMessage(msg);
        if (status != MAMA_STATUS_OK) return status;
    }

    mamaMsgType m = (mamaMsgType) msgType;

    return MAMA_STATUS_OK;
}



/**
* tick42rmdsBridgeMamaPublisher_getDictionaryMessage
* Get the underlying message for the data dictionary.
*
*
* @param transport The transport that holds internal dictionary
* @param msg The address of the mamaMsg where the result is to be written
*/
static mama_status
    tick42rmdsBridgeMamaPublisher_getDictionaryMessage(
    mamaTransport  transport,
    mamaMsg         *msg)
{
    mama_status status;

    // get hold of the bridge implementation
    mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(transport);
    if (!bridgeImpl) {
        mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeCreateDictionary(): Could not get bridge");
        return MAMA_STATUS_PLATFORM;
    }

    RMDSBridgeImpl*      rmdsBridge = NULL;
    if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &rmdsBridge)))
    {
        mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeCreateDictionary(): Could not get rmds bridge object");
        return status;
    }
    if (rmdsBridge->hasTransportBridge())
    {
        mamaDictionary  dictionary= NULL;

        // make sure we get the right transport
        boost::shared_ptr<RMDSTransportBridge> bridge = rmdsBridge->getTransportBridge(transport);

        RMDSSubscriber_ptr_t subscriber = bridge->Subscriber();

        t42log_info("Building dictionary for transport %s\n", subscriber->GetTransportName().c_str());

        if (subscriber)
        {
            const UpaMamaFieldMap_ptr_t& upaMamaMap = subscriber->FieldMap();
            if (upaMamaMap)
            {
                utils::mama::mamaDictionaryWrapper d = upaMamaMap->GetCombinedMamaDictionary();
                dictionary = d.get();
            }
        }

        // Prepare the OpenMAMA dictionary part of the message
        mamaMsg dictMsg=NULL;
        mama_status res = mamaDictionary_getDictionaryMessage (dictionary, &dictMsg);

        *msg = dictMsg;
        return res;

    }

    return status;
}


mama_status
    tick42rmdsBridgeMamaPublisher_sendReplyToInbox (publisherBridge   publisher,
    void *           request,
    mamaMsg           reply)
{

    const char*  replyAddr  = NULL;

    CHECK_PUBLISHER(publisher);

    mamaMsg requestMessage = reinterpret_cast<const mamaMsg>(request);

    msgBridge bm;
    mamaMsg_getNativeHandle(requestMessage, (void**) &bm);

    std::string source;
    std::string symbol;
    tick42rmdsBridgeMamaMsgImpl_getReplyTo(bm, source, symbol);

    RMDSBridgeMsgType_t msgType;
    tick42rmdsBridgeMamaMsgImpl_getMsgType(bm, &msgType);

    if (msgType == RMDS_MSG_PUB_NEW_ITEM_REQUEST)
    {
        // give the reply message to the publisher
        UPABridgePublisherItem * upaPublisherItem = (UPABridgePublisherItem*)(publisher);
        upaPublisherItem->PublishMessage(reply);

    }


    return MAMA_STATUS_OK;

}


mama_status
    tick42rmdsBridgeMamaPublisher_destroy (publisherBridge publisher)
{

    UPABridgePublisher * pub = (UPABridgePublisher *)publisher;
    pub->raiseOnDestroy();

    pub->Shutdown(); // just force it to drop its own shared ptr reference

    return MAMA_STATUS_OK;
}



/*
* The callback to run asynchronously as a reply for a dictionary request
*/
void MAMACALLTYPE dictionaryReplyCb(mamaQueue queue,void *closure)
{
    dictionaryReplyClosure *param = (dictionaryReplyClosure*) closure;
    rmdsMamaInbox_send( param->inbox, param->msg);

    // should destroy the message now
    if(MAMA_STATUS_OK != mamaMsg_destroy(param->msg))
    {
        t42log_warn("Failed to destroy dictionary message sent to inbox\n");
    }

    free(closure);
}

mama_status SendMamaDictionaryMessage( UPABridgePublisher * upaPublisherBridge, mamaMsg reply, publisherBridge publisher, const char* replyAddr, mamaInbox inbox, bool queuedictionary )
{
    mama_status status;
    // Create the dictionary part of the dictionary reply message

    CHECK_PUBLISHER(upaPublisherBridge);

    void * pPub = (void*)upaPublisherBridge;

    mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(upaPublisherBridge->Transport());
    if (!bridgeImpl) {
        mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex(): Could not get bridge");
        return MAMA_STATUS_PLATFORM;
    }

    // Prepare mandatory fields needed for the reply
    status = mamaMsg_updateU8(reply, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_INITIAL);
    status = mamaMsg_updateU8(reply, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, MAMA_MSG_STATUS_OK);



    if (queuedictionary)
    {
        dictionaryReplyClosure *queueClosure = (dictionaryReplyClosure*)malloc(sizeof(dictionaryReplyClosure));
        queueClosure->inbox = inbox;
        queueClosure->msg = reply;

        mamaQueue theBridgeQueue = NULL;

        tick42rmdsBridge_getTheMamaQueue (&theBridgeQueue);
        return mamaQueue_enqueueEvent(theBridgeQueue, dictionaryReplyCb, queueClosure);
    }

    // otherwise send it directly
    mama_status ret = rmdsMamaInbox_send( inbox, reply);

    if(MAMA_STATUS_OK != mamaMsg_destroy(reply))
    {
        t42log_warn("Failed to destroy dictionary message sent to inbox\n");
    }

    return ret;
}

class PublisherDictionaryReply : public DictionaryReply_t
{
public:
    PublisherDictionaryReply (UPABridgePublisher * upaPublisherBridge, publisherBridge mamaPublisher, const char* replyAddr, mamaInbox inbox, bool queuedictionary )
        : RMDSPublisher_(upaPublisherBridge)
        , publisher(mamaPublisher)
        , replyAddr(replyAddr)
        , inbox(inbox)
        , queuedictionary_(queuedictionary)
    {
    }

    virtual mama_status Send()
    {
        mamaMsg        reply=NULL;
        mama_status status;

        if (MAMA_STATUS_OK == (status = tick42rmdsBridgeMamaPublisher_getDictionaryMessage(RMDSPublisher_->Transport(), &reply)))
        {
            status = SendMamaDictionaryMessage(RMDSPublisher_, reply, publisher, replyAddr, inbox, queuedictionary_);
        }
        return status;
    }

    virtual ~PublisherDictionaryReply()
    {

    }

private:
    UPABridgePublisher * RMDSPublisher_;
    mamaMsg reply;
    publisherBridge publisher;
    const char* replyAddr;
    mamaInbox inbox;
    bool queuedictionary_;
};




mama_status
    tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex (publisherBridge  publisher,
    int              tportIndex,
    mamaInbox        inbox,
    mamaMsg          msg)
{
    const char* replyAddr = NULL;
    mama_status status;
    if (UPABridgePub(publisher) == 0)
        return MAMA_STATUS_NULL_ARG;

    // get reply address from inbox
    replyAddr = rmdsInboxImpl_getReplySubject(mamaInboxImpl_getInboxBridge(inbox));


    UPABridgePublisher* upaPublisherBridge = (UPABridgePublisher*)(publisher);
    std::string topic = upaPublisherBridge->Symbol();
    std::string subject = upaPublisherBridge->Subject();
    std::string root = upaPublisherBridge->Root();

    // Special treatment for Dictionary request
    if (topic==DATA_DICT_TOPIC && subject == WOMBAT_SUBJECT)
    {

        mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(upaPublisherBridge->Transport());
        if (!bridgeImpl) {
            mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex(): Could not get bridge");
            return MAMA_STATUS_PLATFORM;
        }

        RMDSBridgeImpl*      rmdsBridge = NULL;
        if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &rmdsBridge)))
        {
            mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex(): Could not get rmds bridge object");
            return status;
        }
        if (rmdsBridge->hasTransportBridge())
        {
            boost::shared_ptr<RMDSTransportBridge> bridge = rmdsBridge->getTransportBridge(upaPublisherBridge->Transport());

            TransportConfig_ptr_t config = boost::make_shared<TransportConfig_t>(bridge->Name());
            bool queudictionary = config->getBool("queuedictionary", Default_queuedictionary);
            bridge->setDictionaryReply(boost::make_shared<PublisherDictionaryReply>(upaPublisherBridge, publisher, replyAddr, inbox,  queudictionary));
        }
    }

    if (root == "")
    {
        // no root so it's not an image request or a dictionary request - it's
        // something real to publish build a reply structure so we can send the reply to the inbox
        PublisherPostMessageReply_ptr_t reply(new PublisherPostMessageReply(replyAddr, inbox));
        return upaPublisherBridge->PublishMessage(msg, reply);
    }


    // if the MdSubscMsgType is snapshot then we deal with that

    mama_i32_t subscMsgType = -1;
    mamaMsg_getI32(msg, MamaFieldSubscMsgType.mName, MamaFieldSubscMsgType.mFid,&subscMsgType);


    if ( subscMsgType == MAMA_SUBSC_SNAPSHOT)
    {
        // Need to marshal this to the subscriber to get a snapshot

        // first extract the source
        //we have to unwind the stuff that that the mama infrastructure has built into the subrciption message
        // work on the basis that either will be Root.Source or just Root
        std::vector<std::string> splitVector;
        boost::algorithm::split(splitVector, subject, boost::algorithm::is_any_of(".") );

        std::string source;
        if (splitVector.size() == 1)
        {
            source = splitVector[0];
        }
        else if (splitVector.size() == 2)
        {
            source = splitVector[1];
        }
        else
        {
            t42log_warn("Unable to create snapshot request for %s.%s\n", subject.c_str(), topic.c_str());
            return MAMA_STATUS_BAD_SYMBOL;
        }


        SnapshotReply_ptr_t snap = boost::make_shared<SnapshotReply>(SnapshotReply(publisher, replyAddr, inbox, source, topic));

        mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(upaPublisherBridge->Transport());
        if (!bridgeImpl) {
            mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex(): Could not get bridge");
            return MAMA_STATUS_PLATFORM;
        }
        RMDSBridgeImpl*      rmdsBridge = NULL;
        if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &rmdsBridge)))
        {
            mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex(): Could not get rmds bridge object");
            return status;
        }
        if (rmdsBridge->hasTransportBridge())
        {
            boost::shared_ptr<RMDSTransportBridge> transport = rmdsBridge->getTransportBridge(upaPublisherBridge->Transport());

            transport->SendSnapshotRequest(snap);
        }
        else
        {
            t42log_warn("Unable to get transport \n");
            return MAMA_STATUS_PLATFORM;
        }


        return MAMA_STATUS_OK;
    }

    return tick42rmdsBridgeMamaPublisher_send(publisher, msg);

}


mama_status
    tick42rmdsBridgeMamaPublisher_sendFromInbox (publisherBridge  publisher,
    mamaInbox        inbox,
    mamaMsg          msg)
{
    return tick42rmdsBridgeMamaPublisher_sendFromInboxByIndex (
        publisher, 0, inbox, msg);
}

mama_status
tick42rmdsBridgeMamaPublisher_setUserCallbacks(publisherBridge publisher,
    mamaQueue               queue,
    mamaPublisherCallbacks* cb,
    void*                   closure)
{
    if (UPABridgePub(publisher) == 0)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    UPABridgePublisher* upaPublisherBridge = (UPABridgePublisher*)(publisher);

    upaPublisherBridge->setUserCallbacks(*cb, closure);

    return MAMA_STATUS_OK;
}

//=========================================================================
//=                        Recommended Functions                          =
//=========================================================================

mama_status
    tick42rmdsBridgeMamaPublisher_sendReplyToInboxHandle (publisherBridge  publisher,
    void *           inbox,
    mamaMsg          reply)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}









