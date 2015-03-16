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
#include "rtr/rsslRDM.h"
#include "RMDSPublisherSource.h"
#include "UPAPublisherItem.h"

RMDSPublisherSource::RMDSPublisherSource( const std::string & name )
	:name_(name)
{
}


RMDSPublisherSource::~RMDSPublisherSource(void)
{
}

bool RMDSPublisherSource::InitialiseSource()
{

	for (int i = 0; i < MaxSourceCapabilities; i++)
	{
		capabilities_[i] = 0;
	}

	// In a future version fo the bridge many of the publisher options could be initialised from configuration
	capabilities_[0] = RSSL_DMT_DICTIONARY;
	capabilities_[1] = RSSL_DMT_MARKET_PRICE;
	//capabilities_[2] = RSSL_DMT_MARKET_BY_ORDER;
	//capabilities_[3] = RSSL_DMT_MARKET_BY_PRICE;
	//capabilities_[4] = RSSL_DMT_SYMBOL_LIST;
	//capabilities_[5] = RSSL_DMT_YIELD_CURVE;


	// init these from config too
	openLimit_ = 0x1000; // 64k
	openWindow_ = 0x1000;

	// There are a number of source properties that if the mama pai allowed it, wouuld make sense to set at runtime
	// load factor could  be set and notified as src directory updates dynamically
	loadFactor_ = 1;


	// set these here until ser find a way of setting them through the mama interface
	serviceState_ = 1;
	acceptingRequests_ = 1;

	status_.streamState = RSSL_STREAM_OPEN;
	status_.dataState = RSSL_DATA_OK;
	status_.code = RSSL_SC_NONE;
	status_.text.data = (char *)"OK";
	status_.text.length = (RsslUInt32)strlen("OK");

	// service link info
	linkName_ = "Tick42 OpenMAMA bridge link"; // could really be anything;
	linkType_ = 1;
	linkState_ = 1;
	linkCode_ = 1;
	linkText_ =  "Link state is up";

	// these are the default dictionary names
	// todo make configurable
	fieldDictionaryName_ = "RWFFld";
	enumTypeDictionaryName_ = "RWFEnum";


	// QOS
	rsslClearQos(&qos_);

	qos_.dynamic = RSSL_FALSE;
	qos_.rate = RSSL_QOS_RATE_TICK_BY_TICK;
	qos_.timeliness = RSSL_QOS_TIME_REALTIME;

	supportOutOfBandSnapshots_= false;
	acceptConsumerStatus_ = false;

	return false;
}


UPAPublisherItem_ptr_t RMDSPublisherSource::GetItem( std::string symbol )
{
	UPAPublisherItem_ptr_t ret;

	PublisherItemMap_t::iterator it = publisherItemMap_.find(symbol);

	if (it != publisherItemMap_.end())
	{
		ret = it->second;
	}

	return ret;
}


// add a new item or add the channel and stream to an existing item. Return the item
UPAPublisherItem_ptr_t RMDSPublisherSource::AddItem( RsslChannel * chnl, RsslUInt32 streamID, string source, string symbol, RsslUInt32 serviceId, RMDSPublisherBase * publisher, bool & isNew )
{
	UPAPublisherItem_ptr_t ret;

	t42log_debug("RMDSPublisherSource::AddItem - %s \n", symbol.c_str());

	PublisherItemMap_t::iterator it = publisherItemMap_.find(symbol);

	if (it != publisherItemMap_.end())
	{
		ret = it->second;


		t42log_debug("RMDSPublisherSource::AddItem - %s - already in map so just add channel \n", symbol.c_str());
		// so we already have an item for this symbol, just add the channel / stream
		ret->AddChannel(chnl, streamID);
		isNew = false;
	}
	else
	{
		t42log_debug("RMDSPublisherSource::AddItem - %s - create new item \n", symbol.c_str());

		ret = UPAPublisherItem::CreatePublisherItem(chnl, streamID, source, symbol, serviceId, publisher);
		publisherItemMap_[symbol] = ret;
		isNew = true;
	}

	return ret;

}

bool RMDSPublisherSource::HasItem( string symbol )
{
	PublisherItemMap_t::iterator it = publisherItemMap_.find(symbol);

	if (it != publisherItemMap_.end())
	{
		return true;
	}
	

	return false;
}

void RMDSPublisherSource::RemoveItem( string symbol )
{
	publisherItemMap_.erase(symbol);
}