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
#include "UPAConsumer.h"
#include "RMDSSource.h"
#include "RMDSSubscriber.h"

#include <utils/t42log.h>

#ifdef ENABLE_TICK42_ENHANCED
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
#include "enhanced/T42Enh_UPASubscription.h"
#endif

using namespace utils::thread;

RMDSSource::RMDSSource( const std::string & serviceName, const RsslUInt64 serviceId, UPAConsumer_ptr_t consumer )
    : serviceName_(serviceName)
   , serviceId_(serviceId)
   , consumer_(consumer)
   , pausedUpdates_(false)
{
    // sort out the default domain for this service
    std::string configDomain = consumer_->GetOwner()->Config()->getServicePropertyString(serviceName,"domain","any");

    // so we need to look at the property value ane set the enum type accordingly

    if (::strcasecmp("any", configDomain.c_str()) == 0)
    {
        sourceDomain_ = UPASubscription::SubscriptionTypeUnknown;
        t42log_info("%s %s", serviceName.c_str(), "any");
    }
    else if (::strcasecmp("mp", configDomain.c_str()) == 0)
    {
        sourceDomain_ = UPASubscription::SubscriptionTypeMarketPrice;
        t42log_info("%s %s", serviceName.c_str(), "mp");
    }
    else if (::strcasecmp("mbp", configDomain.c_str()) == 0)
    {
        sourceDomain_ = UPASubscription::SubscriptionTypeMarketByPrice;
        t42log_info("%s %s", serviceName.c_str(), "mbp");
    }
    else if (::strcasecmp("mbo", configDomain.c_str()) == 0)
    {
        sourceDomain_ = UPASubscription::SubscriptionTypeMarketByOrder;
        t42log_info("%s %s", serviceName.c_str(), "mbo");
    }
    else
    {
        // if it doesnt match, set to any
        sourceDomain_ =  UPASubscription::SubscriptionTypeUnknown;
        t42log_info("%s %s", serviceName.c_str(), "(none)");
    }


}

RMDSSource::~RMDSSource()
{
}

//////////////////////////////////////////////////////////////////////////
// Default factory function for creating subscription objects
// Overridden in the T42 enhancement pack
UPASubscription_ptr_t RMDSSource::CreateSubscription(const std::string &symbol, bool logRmdsValues)
{
#ifdef ENABLE_TICK42_ENHANCED
   return UPASubscription_ptr_t(new T42Enh_UPASubscription(ServiceName(), symbol, logRmdsValues));
#else
    return UPASubscription_ptr_t(new UPASubscription(ServiceName(), symbol, logRmdsValues));
#endif
}

//////////////////////////////////////////////////////////////////////////
//
bool RMDSSource::AddSubscription( UPASubscription_ptr_t sub )
{
    T42Lock lock(&subscriptionMapLock_);

   if (sub != 0)
   {
       subscriptions_[sub->Symbol()] = sub;
      return true;
   }
   return false;
}

bool RMDSSource::FindSubscription(const std::string & symbol, UPASubscription_ptr_t & sub )
{
    T42Lock lock(&subscriptionMapLock_);
    SubscriptionMap_t::iterator it = subscriptions_.find(symbol);

    if (it == subscriptions_.end())
    {
        // don't have this
        return false;
    }

    sub = it->second;
    return true;
}

bool RMDSSource::RemoveSubscription( const std::string & symbol )
{
    // just look up on the symbol name and remove
    T42Lock lock(&subscriptionMapLock_);

    SubscriptionMap_t::iterator it = subscriptions_.find(symbol);

    if (it != subscriptions_.end())
    {
        //printf("remove RMDSBridgeSubscription from source %s, 0x%x\n", symbol.c_str(), it->second.get());
        it->second->Close();
        subscriptions_.erase(it);

        return true;
    }

    t42log_warn("RMDSSource::RemoveSubscription - subscription for %s:%s not found", serviceName_.c_str(), symbol.c_str())    ;
    return false;

}

bool RMDSSource::SetStale()
{
    // set all the subscriptions stale
    T42Lock lock(&subscriptionMapLock_);

    SubscriptionMap_t::iterator it = subscriptions_.begin();

    while(it != subscriptions_.end())
    {
        UPASubscription_ptr_t s = it->second;
        s->SetStale(NULL);
        ++it;
    }

    return true;
}

bool RMDSSource::SetLive()
{
    // set all the subscriptions live
    T42Lock lock(&subscriptionMapLock_);

    SubscriptionMap_t::iterator it = subscriptions_.begin();

    while(it != subscriptions_.end())
    {
        UPASubscription_ptr_t s = it->second;
        s->SetLive();
        ++it;
    }
    return true;
}

void RMDSSource::SetState( ServiceState newState )
{
    bool oldState = state_.state;

    state_ = newState;

    if (oldState && !newState.state)
    // was up and now down
    {
        SetStale();
    }

    // cant rely on the TREP to always send image / live status when the source recovers. non-interactive sources may not
    // so force the live state (and status message) on each subscription

    if (!oldState && newState.state)
    // was down and now up
    {
        SetLive();
    }
    // other combinations oldstate=Up + newstate = up, oldstate =down + newstate = down and oldstate=down and new state = up can be ignored
    // the last of these - service recovery should result in new image and status from rmds
}

bool RMDSSource::ReSubscribe()
{
    // set all the subscriptions stale
    T42Lock lock(&subscriptionMapLock_);

    SubscriptionMap_t::iterator it = subscriptions_.begin();

    while(it != subscriptions_.end())
    {
        UPASubscription_ptr_t s = it->second;
        s->ReSubscribe();
        ++it;
    }
    return true;
}

