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
#ifndef __RMDS_SOURCES_H__
#define __RMDS_SOURCES_H__

#include "RMDSSource.h"

// Manage collection of RMDS sources
// need to be able to lookup by service ID as that is how we see updates from the UPA API
// need to be able to look up[ by name as that is how we see requests for subscription

class RMDSSources
{
public:
	RMDSSources();

	virtual ~RMDSSources()
	{

	}
	void Initialise(UPAConsumer_ptr_t consumer);
	void Shutdown();

	// create sources
	typedef RMDSSource_ptr_t (*SourceFactory)(RsslUInt64 keyId, std::string keyName, UPAConsumer_ptr_t consumer);
	static RMDSSource_ptr_t DefaultFactory(RsslUInt64 keyId, std::string keyName, UPAConsumer_ptr_t consumer);
	RMDSSource_ptr_t UpdateOrCreate(RsslUInt64 keyId, std::string keyName, ServiceState value, SourceFactory Creator = DefaultFactory);

	// locate sources
	bool Exists(RsslUInt64 keyId) const;
	bool Exists(std::string keyName) const ;
	bool Find(RsslUInt64 keyId, RMDSSource_ptr_t &value) const;
	bool Find(std::string keyName, RMDSSource_ptr_t &value) const;

	// manage state
	bool SetAllStale();
	bool ResubscribeAll();

	// We need to stop updates being sent while closing down
	void PauseUpdates();
	void ResumeUpdates();

	UPAConsumer_ptr_t Consumer() { return consumer_; }
	typedef std::vector< std::pair<std::string, ServiceState> > service_snapshot_t;
	size_t SnapshotNames(service_snapshot_t &names);

private:
	typedef std::map<std::string, RsslUInt64> ServiceNameMap;
	ServiceNameMap serviceNameMap_;

   typedef std::map<RsslUInt64, RMDSSource_ptr_t> services_t;
   services_t servicesMap_;

	UPAConsumer_ptr_t consumer_;

#ifdef ENABLE_TICK42_ENHANCED
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
   void *enhancedTag_;
#endif // ENABLE_TICK42_ENHANCED
};

#endif //__RMDS_SOURCE_H__