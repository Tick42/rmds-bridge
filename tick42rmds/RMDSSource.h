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
#ifndef __RMDS_SOURCE_H__
#define __RMDS_SOURCE_H__

#include "UPASubscription.h"
#include "RMDSBridgeSubscription.h"
#include <utils/thread/lock.h>

struct ServiceState
{
	bool state;
	bool acceptConnections;
	ServiceState() : state(false), acceptConnections(false) {}
	ServiceState(bool state, bool acceptConnections) : state(state), acceptConnections(acceptConnections) {}
	inline ServiceState &operator =(const ServiceState &rhs)
	{
		if (this != &rhs)
		{
			state = rhs.state;
			acceptConnections = rhs.acceptConnections;
		}
		return *this;
	}
	ServiceState(const ServiceState &rhs)
	{
		*this = rhs;
	}
	bool operator==(const ServiceState &rhs) const
	{
		return (this == &rhs) || (state == rhs.state && acceptConnections == rhs.acceptConnections);
	}
};


//////////////////////////////////////////////////////////////////////////
//
// represents an RMDS subscription source
//
// Its created from the RMDS source directory update (i.e. dynamically from data delivered by the RMDS) and tracks the state of the RMDS source
//
// all the subscribed items are managed through the source so that state changes on the source can be reflected onto the individual items

class RMDSSource
{
public:
	RMDSSource(const std::string & serviceName, const RsslUInt64 serviceId, UPAConsumer_ptr_t consumer);
	virtual ~RMDSSource(void);

	// properties
	std::string ServiceName() const { return serviceName_; }
	RsslUInt64 ServiceId() const { return serviceId_; }

	ServiceState State() const { return state_; }

	// add/remove subscriptions
    virtual UPASubscription_ptr_t CreateSubscription(const std::string &symbol, bool logRmdsValues);
	bool AddSubscription(UPASubscription_ptr_t sub);
	bool RemoveSubscription( const std::string & symbol);

	bool FindSubscription(const std::string & symbol, UPASubscription_ptr_t & sub);

	// manage the state
	void SetState(ServiceState state);

	bool SetStale();
	bool SetLive();
	bool ReSubscribe();

	// pause / resume updates
	bool IsPausedUpdates() const
	{
		return pausedUpdates_;
	}
	void PauseUpdates()
	{
		pausedUpdates_ = true;
	}
	void ResumeUpdates()
	{
		pausedUpdates_ = false;
	}

	// OMM domain for the source
	// This is either set by config or implied by the symbol name
	UPASubscription::UPASubscriptionType SourceDomain() const { return sourceDomain_; }


private:
	RsslUInt64 serviceId_;
	std::string serviceName_;

    bool pausedUpdates_;

	ServiceState state_;

	UPAConsumer_ptr_t consumer_;

	typedef std::map<std::string, UPASubscription_ptr_t> SubscriptionMap_t;
	SubscriptionMap_t subscriptions_;

	typedef std::list<UPASubscription_ptr_t> SubscriptionList_t;

	// park any bad subscriptions here to keep the ref count up until mama destroys them
	SubscriptionList_t badSubscriptionsPark_;

	mutable utils::thread::lock_t subscriptionMapLock_;

	UPASubscription::UPASubscriptionType sourceDomain_;


};

#endif //__RMDS_SOURCE_H__