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

#include <utils/os.h>
#include <utils/t42log.h>
#include <utils/threadMonitor.h>
#include "transportconfig.h"

#include "RMDSFileSystem.h"
#include "msg.h"

#include "RMDSPublisher.h"
#include "UPAProvider.h"
#include "utils/properties.h"
#include "RMDSPublisherSource.h"
#include "RMDSBridgeImpl.h"

#ifdef ENABLE_TICK42_ENHANCED
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
#include "enhanced//T42Enh_UPAProvider.h"
#endif

using namespace std;

RMDSPublisher::RMDSPublisher(UPATransportNotifier &notify)
    : RMDSPublisherBase(notify)
{
}

RMDSPublisher::~RMDSPublisher(void)
{
}

bool RMDSPublisher::Initialize(mamaBridge bridge, mamaTransport transport, const std::string &transport_name)
{
    bridge_ = bridge;
    transport_ = transport;
    transportName_ = transport_name;

    mama_status status = MAMA_STATUS_OK;
    t42log_debug( "RMDSPublisher::Initialize(): Entering.");

    if (MAMA_STATUS_OK !=
        (status =  mamaQueue_create (&upaPublisherQueue_, bridge)))
    {
        t42log_error("RMDSPublisher::Initialize: Failed to create upa publisher queue for transport %s\n", transportName_.c_str());
        return false;
    }

    mamaQueue_setQueueName (upaPublisherQueue_,	"UPA_PUBLISHER_QUEUE");

    // Build a field map
    if (!createUpaMamaFieldMap())
    {
        t42log_error("RMDSPublisher::Initialize: Failed to create upa field map for transport %s\n", transportName_.c_str());

        return false;
    }

    // The design of RMDS requires that we know the sources that will be mounted in advance
    // and that they match the configuration held on the RMDS that will connect to this
    // publisher.
    // Open MAMA does not support this mechanism directly, so we read the source names
    // from the configuration and it is the responsibility of the user to make sure that
    // the two configurations match and that they also match the source name that is expected
    // by the client application

    utils::properties config;
    std::string sourcesKey = "mama.tick42rmds.transport." + transport_name + ".source";
    std::string source = config.get(sourcesKey, "");
    boost::algorithm::trim(source);

    t42log_debug("Interactive publisher on transport '%s' using source '%s'\n", transport_name.c_str(), source.c_str());
    InitialiseSource(source);

    bridgeImpl_ = mamaTransportImpl_getBridgeImpl(transport);

    mamaQueue defaultQueue;
    tick42rmdsBridge_getTheMamaQueue (&defaultQueue);

    mamaInbox_create(&inbox_, transport_, defaultQueue, RMDSPublisher::InboxOnMessageCB, 0, this);
    return true;
}

static void * threadFuncPublisher(void * p)
{
    // The thread monitor outputs debug when the thread starts and stops
    utils::os::ThreadMonitor mon("RMDSPublisher");

    RMDSPublisher * pOwner = (RMDSPublisher *) p;
    pOwner->Provider()->Run();

    return 0;		
}

bool RMDSPublisher::Start()
{
    TransportConfig_t config(transportName_);

    portNumber_ = config.getString("pubport");

    subscriberTransportName_ = config.getString("sub_tport");

    if (subscriberTransportName_ == "")
    {
        subscriberTransportName_ = transportName_;
        t42log_info(" RMDSPublisher::Start - No value specified in mama.properties for sub transport name using pub transport - %s\n", transportName_.c_str());
    }
    else
    {
        t42log_info(" RMDSPublisher::Start - using subscriber transport name - %s\n", transportName_.c_str());
    }

    t42log_info(" RMDSPublisher::Start - publisher listening on port  - %s\n", portNumber_.c_str());

    // so we need to get hold of a subscriber on the specified transport. This is where we will be sending the newItem messages

    // first get hold of our bridge implementation
    RMDSBridgeImpl* upaBridge = NULL;
    mama_status status;

    if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure(bridge_, (void**) &upaBridge)))
    {
        t42log_warn("Unable to obtain bridgeImpl for publisher transport %s \n",transportName_.c_str());
    }

    // then get correct transport
    RMDSTransportBridge_ptr_t transportBridge = upaBridge->getTransportBridge(subscriberTransportName_);

    // finally get the subscriber from the transport
    subscriber_ = transportBridge->Subscriber();

#ifdef ENABLE_TICK42_ENHANCED
    // use the enhanced provider
    provider_ = UPAProvider_ptr_t(new T42Enh_UPAProvider(this));
#else
    provider_ = UPAProvider_ptr_t(new UPAProvider(this));
#endif

    bool result = wthread_create( &hProviderThread_, 0, threadFuncPublisher, this ) == 0;
    if (result)
    {
        publisherState_ = connecting;
    }
    return result;
}

// initialise the source from the mama properties
int RMDSPublisher::InitialiseSource( const std::string sourceName )
{
    utils::properties config;

    RMDSPublisherSource * newSource = new RMDSPublisherSource(sourceName);

    newSource->InitialiseSource();
    // now read the source properties
    std::string sourcePropBase = "mama.tick42rmds.transport." + transportName_ + "." + sourceName;
    std::string serviceId = config.get(sourcePropBase + ".serviceid", "");

    int RMDSServiceId = ::atoi(serviceId.c_str());
    if (RMDSServiceId == 0)
    {
        t42log_warn("Service %s has invalid or missing service id property '%s' \n", sourceName.c_str(), serviceId.c_str());
    }
    else
    {
        newSource->ServiceId(RMDSServiceId);
    }

    t42log_info("InteractivePublisher on transport '%s' added new source '%s' with service id %d\n", transportName_.c_str(), sourceName.c_str(), RMDSServiceId);
    publisherSource_ = newSource;
    return 0;
}

void RMDSPublisher::SentLoginResponse()
{
    // so at this point we have connected to at least 1 client
    //
    // if we weren't connected before then we are now

    if (publisherState_ != live)
    {
        publisherState_ = live;
        publisherSource_->AcceptingRequests(true);
        publisherSource_->ServiceState(1);
    }
}

bool RMDSPublisher::RequestItem( std::string source, std::string symbol, bool refresh )
{
    // get hold of the subscription for new items
	/// and build a new item reuqest message
    mamaSubscription sub;
    subscriber_->GetNewItemSubscription(source, &sub);

    mamaMsg msg;	
    mamaMsg_createForPayload(&msg, MAMA_PAYLOAD_TICK42RMDS);

    mamaMsgReplyImpl reply;
    reply.mBridgeImpl = bridgeImpl_;
    reply.replyHandle = (void*)inbox_;

    mamaMsgImpl_useBridgePayload(msg, bridgeImpl_);

    msgBridge mb;
    mamaMsgImpl_getBridgeMsg(msg, &mb);
    tick42rmdsBridgeMamaMsgImpl_setMsgType(mb, RMDS_MSG_PUB_NEW_ITEM_REQUEST);
    tick42rmdsBridgeMamaMsgImpl_setReplyTo(mb,source, symbol);

    mamaMsg_addI32(msg, MamaFieldSubscriptionType.mName, MamaFieldSubscriptionType.mFid, MAMA_SUBSC_TYPE_NORMAL );

    mamaMsg_addString(msg, MamaFieldSubscSymbol.mName, MamaFieldSubscSymbol.mFid, symbol.c_str());

    // for the moment just do this as its not clear what the DQPublisherManager is actually doing
    mamaMsg_addI32(msg, MamaFieldSubscMsgType.mName, MamaFieldSubscMsgType.mFid, MAMA_SUBSC_SUBSCRIBE);

    mama_status status = MAMA_STATUS_OK;
    try
    {
        status = mamaSubscription_processMsg(sub, msg);
    }
    catch (...)
    {
        mamaMsg_destroy(msg);	
        t42log_error("RMDSPublisher::RequestItem - caught exception calling mamaSubscription_processMsg for %s\n",symbol.c_str());
    }

    return true;
}

bool RMDSPublisher::CloseItem(std::string source, std::string symbol)
{
    // build an unsubscribe message

    mamaMsg msg;
    mamaMsg_createForPayload(&msg, MAMA_PAYLOAD_TICK42RMDS);

    // get hold of the subscription for new items
    mamaSubscription sub;
    subscriber_->GetNewItemSubscription(source, &sub);

    mamaMsg_addI32(msg, MamaFieldSubscriptionType.mName, MamaFieldSubscriptionType.mFid, MAMA_SUBSC_TYPE_NORMAL );
    mamaMsg_addString(msg, MamaFieldSubscSymbol.mName, MamaFieldSubscSymbol.mFid, symbol.c_str());
    mamaMsg_addI32(msg, MamaFieldSubscMsgType.mName, MamaFieldSubscMsgType.mFid, MAMA_SUBSC_UNSUBSCRIBE);

    mama_status status = MAMA_STATUS_OK;
    try
    {
        status = mamaSubscription_processMsg(sub, msg);
    }
    catch (...)
    {
        mamaMsg_destroy(msg);	
        t42log_error("RMDSPublisher::RequestItem - caught exception calling mamaSubscription_processMsg for %s\n",symbol.c_str());
    }

    return true;
}

void MAMACALLTYPE RMDSPublisher::InboxOnMessageCB(mamaMsg msg, void *closure)
{
    ((RMDSPublisher*)closure)->InboxOnMessage(msg);
}

void RMDSPublisher::InboxOnMessage( mamaMsg msg )
{
}

// empty virtual function overridden by enhanced class
bool RMDSPublisher::SendInsertMessage( mamaMsg msg )
{
    t42log_warn("The enhanced version of the Tick42 RMDS bridge is required to accept posted messages");
    return false;
}

// put this here as we dont have a source file for the publisher base
// this is used by the NI publisher too

bool RMDSPublisherBase::createUpaMamaFieldMap()
{
    bool result = false;

    // A publisher needs to load the RMDS dictionary from files. As it is a source of data, there
    // is no available connection to subscribe to a dictionary. This differs from the "posting" case,
    // where it will contribute data to a source that is hosted elsewhere.

    UPADictionaryWrapper_ptr_t rmdsDict(new UPADictionaryWrapper());
    string fieldFile;
    string enumFile;
    TransportConfig_t config(transportName_);

    // Look for publisher-specific settings from pubfieldfile and pubenumfile and if those do not exist,
    // use the subscriber-specific settings
    std::string dictionaryFileNamePath = GetActualPath(config.getString("pubfieldfile"));
    if (dictionaryFileNamePath.empty())
    {
        dictionaryFileNamePath = GetActualPath(config.getString("fieldfile"));
    }

    if (dictionaryFileNamePath.empty())
    {
        t42log_error("Unable to create dictionary for publisher on transport %s = no dictionary file configured \n", transportName_.c_str());
        return false;
    }

    std::string enumTableFileNamePath = GetActualPath(config.getString("pubenumfile"));
    if (enumTableFileNamePath.empty())
    {
        enumTableFileNamePath = GetActualPath(config.getString("enumfile"));
    }

    if (enumTableFileNamePath.empty())
    {
        t42log_error("Unable to create dictionary for publisher on transport %s = no enum file configured \n", transportName_.c_str());
        return false;
    }

    if (!rmdsDict->LoadFullDictionary(dictionaryFileNamePath, enumTableFileNamePath))
    {
        t42log_error("failed to load RMDS dictionary for publisher on transport %s, using field file = %s and enum file = %s \n", transportName_.c_str(), 
            dictionaryFileNamePath.c_str(), enumTableFileNamePath.c_str());
        return false;
    }

    if (config.exists("fidsoffset"))
    {
        boost::shared_ptr<UpaMamaFieldMapHandler_t> upaMamaFieldMapTmp(
            new UpaMamaFieldMapHandler_t(
            config.getString("fieldmap", Default_Fieldmap),
            config.getUint16("fidsoffset", Default_FieldOffset),
            config.getBool("unmapdfld", Default_PassUnmappedFields),
            config.getString("mama_dict"),
            rmdsDict
            ));
        UpaMamaFieldMap_ = upaMamaFieldMapTmp;
    }
    else
    {
        boost::shared_ptr<UpaMamaFieldMapHandler_t> upaMamaFieldMapTmp (
            new UpaMamaFieldMapHandler_t(
            config.getString("fieldmap", Default_Fieldmap),
            config.getBool("unmapdfld", Default_PassUnmappedFields),
            config.getString("mama_dict"),
            rmdsDict
            ));
        UpaMamaFieldMap_ = upaMamaFieldMapTmp;
    }

    result = UpaMamaFieldMap_ ? true : false;

    if (!result)
    {
    t42log_warn( "Could not create field map!"); //<- should never come here!
    }
    return result;
}
