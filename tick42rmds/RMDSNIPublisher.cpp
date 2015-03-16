/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2014 Tick42 Ltd.
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

#include "rmdsBridgeTypes.h"
#include "RMDSNIPublisher.h"

#include "utils/os.h"
#include "utils/t42log.h"
#include "utils/threadMonitor.h"
#include "transportconfig.h"

#include "RMDSPublisherSource.h"
#include "UPALogin.h"
#include "UPANIProvider.h"
#include "RMDSBridgeImpl.h"

using namespace std;

RMDSNIPublisher::RMDSNIPublisher(UPATransportNotifier &notify)
			: RMDSPublisherBase(notify), interfaceName_(0)
{
	connType_ = RSSL_CONN_TYPE_SOCKET;

	publisherState_ = unconnected;
	connected_ = false;
	recovering_ = false;
}


RMDSNIPublisher::~RMDSNIPublisher(void)
{
	if (interfaceName_)
		free(interfaceName_);
}

bool RMDSNIPublisher::Initialize( mamaBridge bridge, mamaTransport transport, const std::string &transport_name )
{

	bridge_ = bridge;
	transport_ = transport;
	transportName_ = transport_name;


	config_ = boost::make_shared<TransportConfig_t>(this->transportName_);

	mama_status status = MAMA_STATUS_OK;
	t42log_debug( "RMDSNIPublisher::Initialize(): Entering.");

	if (MAMA_STATUS_OK !=
		(status =  mamaQueue_create (&upaPublisherQueue_, bridge)))
	{
		t42log_error("RMDSNIPublisher::Initialize: Failed to create upa publisher queue for tranport %s\n", transportName_.c_str());

		return false;
	}


	mamaQueue_setQueueName (upaPublisherQueue_,	"UPA_NIPUBLISHER_QUEUE");

	// Build a field map
	if (!createUpaMamaFieldMap())
	{
		t42log_error("RMDSNIPublisher::Initialize: Failed to create upa field mapfor tranport %s\n", transportName_.c_str());

		return false;
	}



	// read a list of sources from the config

	utils::properties config;
	std::string sourcesProp = "mama.tick42rmds.transport." + transport_name + ".nisource";
	std::string source = config.get(sourcesProp, "");
    boost::algorithm::trim(source);

	if (!source.empty())
	{
		InitialiseSource(source);
	}
	else
	{
		t42log_warn("Failed to initialise non-interactive publisher - no sources specified\n");
		return false;
	}



	bridgeImpl_ = mamaTransportImpl_getBridgeImpl(transport);


	return true;



}

void RMDSNIPublisher::InitialiseSource( const std::string sourceName)
{
	utils::properties config;

    RMDSPublisherSource * newSource = new RMDSPublisherSource(sourceName);

	newSource->InitialiseSource();
	// now read the source properties
	std::string sourcePropBase = "mama.tick42rmds.transport." + transportName_ + "." + sourceName;
	std::string serviceId = config.get(sourcePropBase + ".serviceid", "0");

	int RMDSServiceId = ::atoi(serviceId.c_str());
	if (RMDSServiceId == 0)
	{
		t42log_info("Service %s has missing or zero service id property '%s' \n", sourceName.c_str(), serviceId.c_str());
	}
	else
	{
		newSource->ServiceId(RMDSServiceId);
	}

	publisherSource_ = newSource;
}

static void * threadFunc(void * p)
{
   // The thread monitor outputs debug when the thread starts and stops
   utils::os::ThreadMonitor mon("RMDSNIPublisher");

	RMDSNIPublisher * pOwner = (RMDSNIPublisher *) p;
	pOwner->NIProvider()->Run();

	return 0;

}
bool RMDSNIPublisher::Start()
{

	TransportConfig_t config(transportName_);

	niProvider_ = UPANIProvider_ptr_t(new UPANIProvider(this));
	niProvider_->AddListener(this);

	std::string subscriberTransportName = config.getString("sub_tport");

	if (subscriberTransportName == "")
	{
		subscriberTransportName = transportName_;
		t42log_info(" RMDSPublisher::Start - No value specified in mama.properties for sub transport name using pub transport - %s\n", transportName_.c_str());
	}
	else
	{
		t42log_info(" RMDSPublisher::Start - using subscriber transport name - %s\n", transportName_.c_str());
	}


	// so we need to get hold of a subscriber on the specified transport. This is where we will be sending the newItem messages

	// first get hold of our bridge implementation
	RMDSBridgeImpl* upaBridge = NULL;
	mama_status status;

	if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure(bridge_, (void**) &upaBridge)))
	{
		t42log_warn("Unable to obtain bridgeImpl for publisher transport %s \n",transportName_.c_str());
	}

	// then get correct transport
	RMDSTransportBridge_ptr_t transportBridge = upaBridge->getTransportBridge(subscriberTransportName);

	// finally get the subscriber from the transport

	subscriber_ = transportBridge->Subscriber();

	bool result = wthread_create( &NIProviderThread_, 0, threadFunc, this ) == 0;
	if (result)
	{
		publisherState_ = connecting;
	}
	return result;

	return true;
}

void RMDSNIPublisher::LoginResponse( UPALogin::RsslLoginResponseInfo * pResponseInfo, const std::string &extraInfo )
{

	// Copy current login state
	memcpy(&responseInfo_, pResponseInfo, sizeof(responseInfo_));

	LogResponseInfo(*pResponseInfo);

	if (connected_)
	{
		// now we are connected can send the source directory
		t42log_debug(extraInfo.c_str());
		publisherState_ = sendingsourcedirectory;

		niProvider_->SendSourceDirectory(upaPublisherQueue_);
	}
	else
	{
		notify_.onConnectionFailed(extraInfo);
		publisherState_ = unconnected;
	}
}

// Will create string representation of the important part of RsslLoginResponseInfo
void RMDSNIPublisher::LogResponseInfo(const UPALogin::RsslLoginResponseInfo &responseInfo)
{
	t42log_debug("Got Login Response:" );
	t42log_debug("\tStreamId [%u]", responseInfo.StreamId);
	t42log_debug("\tUsername [%s]", responseInfo.Username);
	t42log_debug("\tApplicationId [%s]", responseInfo.ApplicationId);
	t42log_debug("\tApplicationName [%s]", responseInfo.ApplicationName);
	t42log_debug("\tPosition [%s]", responseInfo.Position);
	t42log_debug("\tProvidePermissionProfile [%ld]", responseInfo.ProvidePermissionProfile);
	t42log_debug("\tProvidePermissionExpressions [%ld]", responseInfo.ProvidePermissionExpressions);
	t42log_debug("\tSingleOpen [%ld]",responseInfo.SingleOpen);
	t42log_debug("\tAllowSuspectData [%ld]", responseInfo.AllowSuspectData);
	t42log_debug("\tSupportPauseResume [%ld]", responseInfo.SupportPauseResume);
	t42log_debug("\tSupportOptimizedPauseResume [%ld]", responseInfo.SupportOptimizedPauseResume);
	t42log_debug("\tSupportOMMPost [%ld]", responseInfo.SupportOMMPost);
	t42log_debug("\tSupportViewRequests [%ld]", responseInfo.SupportViewRequests);
	t42log_debug("\tSupportBatchRequests [%ld]", responseInfo.SupportBatchRequests);
	t42log_debug("\tSupportStandby [%ld]", responseInfo.SupportStandby);
	t42log_debug("\tisSolicited [%s]", responseInfo.isSolicited ? "true" : "false");
}



void RMDSNIPublisher::ConnectionNotification( bool connected, std::string extraInfo )
{
	// connection notification from the upa thread
	t42log_debug("Connected = %s", connected ? "true" : "false");

	bool wasConnected = connected_;
	connected_ = connected;

	if (connected_)
	{

		if (!niProvider_->RequestLogin(upaPublisherQueue_))
		{
			publisherState_ = unconnected;
			notify_.onLoginFailed(extraInfo);
		}
		else
		{
			publisherState_ = loggingin;
		}
	}
	else
	{
		// if it was connected then recover
		if (wasConnected)
		{
			publisherState_ = connecting;
			recovering_ = true;
			notify_.onConnectionDisconnect("disconnected from RMDS");
		}
		else
		{
			publisherState_ = unconnected;
			notify_.onConnectionFailed(extraInfo);
		}

	}

}
