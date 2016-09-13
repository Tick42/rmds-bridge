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
#include "RMDSConnectionConfig.h"


using namespace utils::parse;

RMDSConnectionConfig::RMDSConnectionConfig(const std::string &hosts, const std::string &retrysched)
    : retries_(0)
    , valid_(false)
{
    try
    {
        // parse hosts and retrysched properties and create vectors counterparts
        hosts_ = CreateHostVector(hosts);
        retryscheds_ = CreateRetryschedVector(retrysched);

        // Retry schedule values are optional, and if missing they should default to 0,3(3),10(3),30(6)
        if (retryscheds_.empty())
        {
            t42log_error("Failed to parse retrysched property [%s] Defaults to 0,3(3),10(3),30(6)", retrysched.c_str());
            SetDefaultRetryschedValues();
        }

        // if parsed successfully then initialize dependents (iterators for examples)
        if (!hosts_.empty())
        {
            itCurrHost_ = hosts_.begin();
            itCurrRetrysched_ = retryscheds_.begin();
            retries_ = itCurrRetrysched_->retry;

            valid_ = true;
        }
        else //if hosts is empty, should fail
        {
            if (hosts_.empty())
                t42log_error("Failed to parse hosts property [%s]", hosts.c_str() );
        }
    }
    catch(...)
    {
        t42log_error("Failed to parse properties:\nhosts[%s]:\nretrysched[%s]", hosts.c_str(), retrysched.c_str());
        valid_ = false;
    }
}

void RMDSConnectionConfig::Next()
{
    // retry timings:
    // 1. first changes the current host,
    // 1.1 once finished all hosts wrap around to the beginning of hosts and take the next
    //     retry configuration
    // 2 retry the same timing as configured in the current retry-schedule configuration
    if (retries_ == 1)
    {
        ++itCurrHost_;
        if (itCurrHost_ == hosts_.end())
        {
            itCurrHost_ = hosts_.begin();
            retrysched_vector_t ::const_iterator testEnd = itCurrRetrysched_;
            ++testEnd;
            if (testEnd != retryscheds_.end())
                ++itCurrRetrysched_;
        }
        retries_ = itCurrRetrysched_->retry;
    }
    else
        --retries_;
}

void RMDSConnectionConfig::SetDefaultRetryschedValues()
{
    retryscheds_.push_back(retrysched_t(0,0));
    retryscheds_.push_back(retrysched_t(3,3));
    retryscheds_.push_back(retrysched_t(10,3));
    retryscheds_.push_back(retrysched_t(30,6));
}

    host_vector_t RMDSConnectionConfig::CreateHostVector(const std::string &hosts)
    {
        const host_t default_item("","14002");
        std::string skipped;
        host_vector_t result = parse_to_vector(hosts, default_item, utils::parse::parse_host_item,skipped);
        if (!skipped.empty())
        {
            t42log_warn("Warning: hosts property skipped items: %s", skipped.c_str());
        }
        return result;
    }

    retrysched_vector_t RMDSConnectionConfig::CreateRetryschedVector(const std::string &retrysched)
    {
        retrysched_t default_item(0,1);
        std::string skipped;
        retrysched_vector_t result = parse_to_vector(retrysched, default_item, utils::parse::parse_retrysched_item,skipped);
        if (!skipped.empty())
        {
            t42log_warn("Warning: retrysched property skipped items: %s", skipped.c_str());
        }
        return result;
    }
