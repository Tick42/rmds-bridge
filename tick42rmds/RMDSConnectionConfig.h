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
#ifndef __RMDSHOSTSRECONNECTCONFIG_H__
#define __RMDSHOSTSRECONNECTCONFIG_H__

#include <utils/parse.h>


/** 
 * @brief Creates a back off reconnection pattern to iterate on for each reconnect event
 *	      The connection retries are defined as a comma separated list of delays(in seconds) and the number of retires at each delay
 *        The default schedule is 0,3(3),10(3),30(6)
 *        would retry both hosts immediately then 3 times with a 3s delay, 3 times with a 10s delay then 6 times with a 30s delay
 */


typedef std::vector<utils::parse::host_t> host_vector_t; 
typedef std::vector<utils::parse::retrysched_t> retrysched_vector_t; 

class RMDSConnectionConfig
{
public:
	/** 
	 * @brief Constructor initializes the hosts and retry-schedule 
	 * @hosts get a CSV string of the following syntax <string>{:number}. example localhost:4,192.168.1.2
	 * @retrysched get a CSV string of the following syntax <number>{(number)}. example: 5(2),3(4),5,6(2),7 the last optional number in braket, in case given, will be practically ignore.
	 */
	RMDSConnectionConfig(const std::string &hosts, const std::string &retrysched);
	/** 
	 * @brief states whether the object is valid or not
	 */
	inline bool Valid() {return valid_;}

	/** 
	 * @brief Set next reconnection configuration
	 */
	void Next();

	/** 
	 * @brief Configured host address to connect to
	 */
	inline std::string Host() const
	{
		return itCurrHost_->host;
	}

	/** 
	 * @brief Configured port number to connect to
	 */
	inline std::string Port() const
	{
		return itCurrHost_->port;
	}

	/** 
	 * @brief Delay in seconds
	 */
	inline unsigned int Delay() const
	{
		return itCurrRetrysched_->delay;
	}


	/** 
	 * @brief Set default values for retrysched (retry schedule) which are: 0,3(3),10(3),30(6)
	 */
	void SetDefaultRetryschedValues();



	unsigned int NumHosts() const
	{
		return (unsigned int)hosts_.size();
	}

private:

	host_vector_t CreateHostVector(const std::string &hosts);
	retrysched_vector_t CreateRetryschedVector(const std::string &retrysched);


	host_vector_t hosts_; // hosts configuration vector
	retrysched_vector_t retryscheds_; // retry-schedule configuration vector
	host_vector_t::const_iterator itCurrHost_; //Current host configuration
	retrysched_vector_t ::const_iterator itCurrRetrysched_; //Current retry-schedule configuration

	unsigned int retries_; //holds how many times should stick to the current retry-schedule configuration
	bool valid_; //states whether the object is valid or not

};

#endif //__RMDSHOSTSRECONNECTCONFIG_H__

