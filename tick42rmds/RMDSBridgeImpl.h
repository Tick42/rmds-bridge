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
#ifndef RMDS_BRIDGE_IMPL_H
#define RMDS_BRIDGE_IMPL_H

#include "transport.h"
class RMDSTransportBridge;

// Simple wrapper around UPASubscriber 

// todo convert upasubcribe to a boost ptr

class RMDSBridgeImpl
{
public:

	RMDSBridgeImpl()
	{
	}

	// add a transport
	void setTransportBridge(const boost::shared_ptr<RMDSTransportBridge>& transportBridge = boost::shared_ptr<RMDSTransportBridge>())
	{
		transports_.push_back(transportBridge);
	}

	//  get the first transport
	 boost::shared_ptr<RMDSTransportBridge> getTransportBridge() const
	{
		// just return the first
		return transports_[0];
	}


	 // get the transport wrapping the specified mama transport
	 boost::shared_ptr<RMDSTransportBridge> getTransportBridge(mamaTransport transport) const
	 {
		boost::shared_ptr<RMDSTransportBridge> ret;

		// find the specified transport
		for(size_t index = 0; index < transports_.size(); index++)
		{
			if (transports_[index].get()->GetTransport() == transport)
			{
				return transports_[index];
			}
		}

		// just return the empty shared ptr if its not found
		return ret;
	 }


	 // get a transport by name
	boost::shared_ptr<RMDSTransportBridge> getTransportBridge(std::string transportName)
	{
		boost::shared_ptr<RMDSTransportBridge> ret;
		// so theres rarely more than 2 or 3 tports so just walk the array to fin the one we want
		for(size_t index = 0; index < transports_.size(); index++)
		{
			if (transports_[index]->Name() == transportName)
			{
				return transports_[index];
			}

		}

		// just return an empty shared ptr if not found
		return ret;
	}


	// get by index
	 boost::shared_ptr<RMDSTransportBridge> getTransportBridge(size_t index) const
	 {
		 return transports_[index];
	 }


	bool hasTransportBridge() const
	{
		return transports_.size() > 0;
	}


	// named transport exists
	bool hasTransportBridge(std::string transportName) const
	{
		// so theres rarely more than 2 or 3 tports so just walk the array to fin the one we want
		for(size_t index = 0; index < transports_.size(); index++)
		{
			if (transports_[index]->Name() == transportName)
			{
				return true;
			}

		}

		return false;
	}


	// how many transports
	size_t NumTransports() const
	{
		return transports_.size();
	}

private:

	// vector of transports created in the bridge
	typedef boost::shared_ptr<RMDSTransportBridge> TransportBridge_ptr_t;
	std::vector<TransportBridge_ptr_t> transports_;

};

#endif