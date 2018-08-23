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
#ifndef __TRANSPORTCONFIG_T_H__
#define __TRANSPORTCONFIG_T_H__

#include <utils/properties.h>

// Defaults for configuration

static const char * Default_Host = "127.0.0.1";
static const char * Default_Port = "14002";
static const char * Default_Fieldmap = "";
static const bool   Default_PassUnmappedFields = true;
static mama_fid_t   Default_FieldOffset = 0;
static const char * Default_UserName = "";
static const char * Default_Appid = "";
static const char * Default_PubType = "post";
static const char * Default_OnStreamPost = "off";
static bool Default_UseSeqNum = false;
static const char * Default_dictsource = "WOMBAT";
static const bool Default_queuedictionary = true;
static const bool Default_disabledataconversion = false;
static const char * Default_retrysched = "0,3(3),10(3),30(6)";
static const int Default_maxdisp = 1000;
static const int Default_maxPending = 1000;
static const char * Default_useCallbacks = "0";
static const char * Default_sendAckMessage = "1";
static const bool Default_asyncMessaging = false;
static const int Default_maxMessageSize = 4096;
static const int Default_waitTimeForSelect = 100000;

/* Thin wrapper class that reflects mama.properties in type safe way.
 * Calling any of the get_<field name> will result with the value of the related filed in mama.properties that ends with that name.
 * For example: get_host results with a value of mama.tick42rmds.transport.rmds_tport.url
 */
class TransportConfig_t
{
    const std::string TPORT_PREFIX;
    std::string transportName_;
    utils::properties properties_;
    struct flag_t //a helper class for the macro above, that default a boolean member to false, that way all cached properties have a corresponding flag that says if they were loaded or not
    {
        bool flag_;
        flag_t() : flag_(false) {}
    };
public:
    TransportConfig_t(std::string transportName) : TPORT_PREFIX("mama.tick42rmds.transport"), transportName_(transportName) {}

    TransportConfig_t &operator=(const TransportConfig_t &rhs)
    {
        if (this!=&rhs)
        {
            transportName_ = rhs.transportName_;
            properties_ = rhs.properties_;
        }
        return *this;
    }
    TransportConfig_t(const TransportConfig_t &rhs) : TPORT_PREFIX(rhs.TPORT_PREFIX)
    {
        *this = rhs;
    }

    inline bool exists(const std::string &name) const
    {
        return utils::propertyExists(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + name);
    }

    inline bool getBool(const std::string &name, bool default_value=false) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + name, default_value);
    }

    inline int getInt(const std::string &name, int default_value=0) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + name, default_value);
    }

    inline std::string getString(const std::string &name, const std::string &default_value="") const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + name, default_value);
    }

    inline uint16_t getUint16(const std::string &name, uint16_t default_value=0) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + name, default_value);
    }

    inline bool existServiceProperty(const std::string &service, const std::string &name) const
    {
        return utils::propertyExists(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + service + std::string(".") + name);
    }

    inline bool getServicePropertyBool(const std::string &service, const std::string &name, bool default_value=false) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + service + std::string(".") + name, default_value);
    }

    inline int getServicePropertyInt(const std::string &service, const std::string &name, int default_value=0) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + service + std::string(".") + name, default_value);
    }

    inline std::string getServicePropertyString(const std::string &service, const std::string &name, const std::string &default_value="") const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + service + std::string(".") + name, default_value);
    }

    inline uint16_t getServicePropertyUint16(const std::string &service, const std::string &name, uint16_t default_value=0) const
    {
        return properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + service + std::string(".") + name, default_value);
    }

private:
    bool cached_property_disabledataconversion_;
    flag_t is_loaded_property_disabledataconversion_;
public:
    inline bool get_disabledataconversion ()
    {
        if (!is_loaded_property_disabledataconversion_.flag_)
        {
            is_loaded_property_disabledataconversion_.flag_=true;
            cached_property_disabledataconversion_ = properties_.get(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + "disabledataconversion", Default_disabledataconversion);
        }
        return cached_property_disabledataconversion_;
    }
    inline bool default_disabledataconversion ()
    {
        return Default_disabledataconversion;
    }
    inline bool exist_disabledataconversion ()
    {
        std::string dummy;
        return utils::getProperty(TPORT_PREFIX + std::string(".") + transportName_ + std::string(".") + "disabledataconversion", dummy);
    }
};


typedef boost::shared_ptr<TransportConfig_t> TransportConfig_ptr_t;

#undef FIELD_GETTER

#endif //__TRANSPORTCONFIG_T_H__

