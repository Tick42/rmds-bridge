/*
* Utils: Tick42 Middleware Utilities
* Copyright (C) 2013 Tick42 Ltd.
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
#ifndef __UTILS_GLOBAL_PROPERTIES_H__
#define __UTILS_GLOBAL_PROPERTIES_H__

#include <stdexcept>
#include <string>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <mama/mama.h>
#include "utils/t42log.h"

namespace utils {

// Helper templates utils for lexical_cast to handle unsigned cast from string so when there is a negative number it should throw bad_lexical_cast
// Please see boost FAQ for more information why negative number do cast "correctly" as 2 complements to unsigned
    template <bool is_unsigned>
    struct unsigned_checker
    {
        template<typename String_type>
        static inline void do_check(const String_type & str) { }
    };

    template <>
    struct unsigned_checker<true>
    {
        template<typename String_type>
        static inline void do_check(const String_type & str)
        {
            if ( str[0] == '-' ) boost::throw_exception( boost::bad_lexical_cast() );
        }
    };

    template<typename Target, typename Source>
    inline Target forced_lexical_cast(const Source &arg)
    {
        unsigned_checker< boost::is_unsigned<Target>::value >::do_check(arg);
        return boost::lexical_cast<Target>( arg );
    }


/**
 * @brief A wrapper around the properties_Get OpenMAMA API. returns a property value
 * @param: property_name the property name. That property will be cached if exist
 * @param: value is the property if exist
 * @return: true if property exists
 */
bool getProperty(const std::string& property_name, std::string& value);

bool propertyExists(const std::string& property_name);

bool propertyAsBool(const std::string& value);

class properties
{
public:
    /**
     * @brief a wrapper method around the getProperty that returns results at all times. the wrapper caches its result
     * @param: property_name the property name to be cached
     * @param: default_value - the default value in case of failure (failure - is first failed and uncached access to that property)
     */
    template<typename T>
    T get(const std::string &name, T default_value) const
    {
        std::string default_value_str;
        try {
            default_value_str = boost::lexical_cast<std::string>(default_value);
        }catch(...){
            std::stringstream os;
            os << "Could not convert property default value";
            t42log_warn ( os.str().c_str());
        }
        std::string tmpValue;
        if (!getProperty(name, tmpValue))
        {
            std::stringstream os;
            os << "properties::get Property [" << name << "] defaults to: [" << default_value_str << "]";
            t42log_debug ( os.str().c_str());
            return default_value;
        }

        T result = default_value;
        try {
            result = forced_lexical_cast<T>(tmpValue);
        }catch(...){
            std::stringstream os;
            os << "Could not convert property value for " << name << "] defaults to: [" << default_value_str << "]";
            t42log_warn ( os.str().c_str());
        }
        return result;
    }
    bool get(const std::string &name, bool default_value) const
    {
        std::string tmpValue;
        if (!getProperty(name, tmpValue))
        {
            std::stringstream os;
            os << "properties::get Property [" << name << "] defaults to: [" << std::string((default_value) ? "true" : "false")  << "]";
            t42log_debug ( os.str().c_str());
            return default_value;
        }

        return propertyAsBool(tmpValue);
    }

    MamaLogLevel get(const std::string &name, MamaLogLevel default_value) const
    {
        MamaLogLevel result;
        std::string tmpValue;
        if (!getProperty(name, tmpValue))
        {
            std::stringstream os;
            os << "properties::get Property [" << name << "] defaults to: [" << mama_logLevelToString(default_value)  << "]";
            t42log_debug ( os.str().c_str());
            return default_value;
        }
        if (!mama_tryStringToLogLevel(tmpValue.c_str(), &result))
        {
            std::stringstream os;
            os << "Could not convert property value for " << name << "] defaults to: [" << "] defaults to: [" << mama_logLevelToString(default_value)  << "]";
            t42log_warn ( os.str().c_str());
            return default_value;
        }
        return result;
    }

    std::string get(const std::string &name, const std::string &default_value) const
    {
        std::string result;
        if (!getProperty(name, result))
        {
            std::stringstream os;
            os << "properties::get Property [" << name << "] defaults to: [" << default_value << "]";
            t42log_debug ( os.str().c_str());
            return default_value;
        }

        return result;
    }

    std::string get(const std::string &name, const char *default_value) const
    {
        std::string result;
        if (!getProperty(name, result))
        {
            std::stringstream os;
            os << "properties::get Property [" << name << "] defaults to: [" << default_value << "]";
            t42log_debug ( os.str().c_str());
            return default_value;
        }
        return result;
    }
};

} /*namespace utils*/

#endif //__UTILS_GLOBAL_PROPERTIES_H__
