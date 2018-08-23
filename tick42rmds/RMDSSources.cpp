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
#include "RMDSSources.h"
#ifdef ENABLE_TICK42_ENHANCED
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
#include "enhanced/T42Enhanced.h"
#endif // ENABLE_TICK42_ENHANCED

#include <utils/t42log.h>

using namespace utils::thread;

//////////////////////////////////////////////////////////////////////////
//
RMDSSources::RMDSSources()
#ifdef ENABLE_TICK42_ENHANCED
    // Additional functionality provided by the enhanced bridge is available as part of a a support package
    // please contact support@tick42.com
   : enhancedTag_(0)
#endif // ENABLE_TICK42_ENHANCED
{
}

//////////////////////////////////////////////////////////////////////////
// The default factory instantiates an RMDSSource object
//
RMDSSource_ptr_t RMDSSources::DefaultFactory(RsslUInt64 keyId, const std::string& keyName, const UPAConsumer_ptr_t& consumer)
{
   return RMDSSource_ptr_t(new RMDSSource(keyName, keyId, consumer));
}

//////////////////////////////////////////////////////////////////////////
// refresh an existing source
//
RMDSSource_ptr_t RMDSSources::UpdateOrCreate(RsslUInt64 keyId, const std::string& keyName, ServiceState value, SourceFactory Creator)
{
    // No check is done if keyName is already exists for a  different keyId, since it is guaranteed that in one session or running the numbers won't change.
    // However in case of such error the new keyId is mapped to the keyName and the old one is overridden.

    RMDSSource_ptr_t s;
    if (!Find(keyId, s))
    {
        // we don't already have a service on this id so create it
        s = Creator(keyId, keyName, consumer_);
        servicesMap_[keyId] = s;
        serviceNameMap_[keyName] = keyId;
    }

    // and set the state
    s->SetState(value);

    return s;
}

bool RMDSSources::Find(RsslUInt64 keyId, RMDSSource_ptr_t& value) const
{
    services_t::const_iterator it = servicesMap_.find(keyId);
    if (it != servicesMap_.end())
        value = it->second;
    return it != servicesMap_.end();
}


bool RMDSSources::Find(const std::string& keyName, RMDSSource_ptr_t& value) const
{
    ServiceNameMap::const_iterator itId = serviceNameMap_.find(keyName);
    if (itId == serviceNameMap_.end())
        return false;
    return Find(itId->second, value);
}


bool RMDSSources::Exists(RsslUInt64 keyId) const
{
    services_t::const_iterator it = servicesMap_.find(keyId);
    return it != servicesMap_.end();
}

bool RMDSSources::Exists(const std::string& keyName) const
{
    ServiceNameMap::const_iterator itId = serviceNameMap_.find(keyName);
    if (itId == serviceNameMap_.end())
        return false;
    return Exists(itId->second);
}

void RMDSSources::Initialise(const UPAConsumer_ptr_t& consumer)
{
   consumer_ = consumer;

#if defined(ENABLE_TICK42_ENHANCED)
   // Initialize the enhanced library
   // Additional functionality provided by the enhanced bridge is available as part of a a support package
   // please contact support@tick42.com
   enhancedTag_ = T42Enhanced::initialize(consumer->getTransportName(), this);
#endif // ENABLE_TICK42_ENHANCED
}

void RMDSSources::Shutdown()
{
#ifdef ENABLE_TICK42_ENHANCED
   // Shutdown the enhanced library
    // Additional functionality provided by the enhanced bridge is available as part of a a support package
    // please contact support@tick42.com
   if (0 != enhancedTag_)
   {
      T42Enhanced::shutdown(enhancedTag_);
   }
#endif // ENABLE_TICK42_ENHANCED
}

bool RMDSSources::SetAllStale()
{
    services_t::const_iterator itSources = servicesMap_.begin();

    while(itSources != servicesMap_.end())
    {
        itSources->second->SetStale();
        ++itSources;
    }

    return true;

}

bool RMDSSources::ResubscribeAll()
{
    services_t::const_iterator itSources = servicesMap_.begin();

    while(itSources != servicesMap_.end())
    {
        itSources->second->ReSubscribe();
        ++itSources;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
//
void RMDSSources::PauseUpdates()
{
   for (services_t::const_iterator itSources = servicesMap_.begin();
      itSources != servicesMap_.end(); ++itSources)
   {
      itSources->second->PauseUpdates();
   }
}

//////////////////////////////////////////////////////////////////////////
//
void RMDSSources::ResumeUpdates()
{
   for (services_t::const_iterator itSources = servicesMap_.begin();
      itSources != servicesMap_.end(); ++itSources)
   {
      itSources->second->ResumeUpdates();
   }
}

//////////////////////////////////////////////////////////////////////////
// Snapshot the list of services and their current status
//
size_t RMDSSources::SnapshotNames(service_snapshot_t& names)
{
   names.clear();
   for (services_t::iterator itSources = servicesMap_.begin();
      itSources != servicesMap_.end(); ++itSources)
   {
      RMDSSource_ptr_t source = itSources->second;
      names.push_back(make_pair(source->ServiceName(), source->State()));
   }
   return names.size();
}
