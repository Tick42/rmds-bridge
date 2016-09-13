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
#ifndef __UTILS_PARSE_H__
#define __UTILS_PARSE_H__

#include <stdexcept>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <utils/parse.h>
#include <utils/t42log.h>

namespace utils { namespace parse {

/**
 * @brief retry schedule values, represented in the retrysched property as <delay>(<retries>)
 */
struct retrysched_t
{
    unsigned int delay;
    unsigned int retry;
    retrysched_t() : delay(0), retry(1) {}
    retrysched_t(unsigned int delay, unsigned int retry) : delay(delay), retry(retry){}
    retrysched_t(const retrysched_t &rhs) : delay(rhs.delay), retry(rhs.retry) {}
    retrysched_t &operator=(const retrysched_t &rhs)
    {
        if (this != &rhs)
        {
            this->delay = rhs.delay;
            this->retry = rhs.retry;
        }
        return *this;
    }
};

/**
 * @brief Takes a string of the following template either <delay>(<retry>) or <delay> only and set a structure of both
 * @param restrysched_item the string value to parse
 * @param result an out parameter with as parsed result
 * @param default_item holds default values. currently only the retry is taken
 * @return true on success
 */
bool parse_retrysched_item(const std::string &retrysched_item, retrysched_t &result, const retrysched_t &default_item);

/**
 * @brief host and port value, represented in the hosts property as host:port
 */
struct host_t
{
    std::string host;
    std::string port;
    host_t() : port("80"){} //just arbitrary port picked
    host_t(const std::string &host, std::string port) : host(host), port(port) {}
    host_t(const host_t &rhs) : host(rhs.host) , port(rhs.port) {}
    host_t &operator=(const host_t &rhs)
    {
        if (this != &rhs)
        {
            this->host = rhs.host;
            this->port = rhs.port;
        }
        return *this;
    }
};

/**
 * @brief Takes a string of the following template either <host>:<port> or <host> only and set a structure of both
 * @param host_item the string value to parse
 * @param result an out parameter with as parsed result
 * @param default_item holds default values. currently only the port is taken
 * @return true on success
 */
bool parse_host_item(const std::string &host_item, host_t &result, const host_t &default_item);

/**
 * @brief Generic CSV Parser that parse a string that represent a vector of items where all items have the same type ItemType
 * @param str input string
 * @param parse_item function pointer for a parser that parse string into ItemType (most probably a structure but can be any type)
 * @return vector of ItemType
 */
template <typename ItemType> std::vector<ItemType> parse_to_vector(const std::string str, const ItemType &default_item, bool (*parse_item)( const std::string &, ItemType &, const ItemType &), std::string &skipped_items)
{
    using namespace boost;
    typedef tokenizer< escaped_list_separator<char> > tokenizer_t;
    std::vector<ItemType> result;

    tokenizer_t tokens(str);
    for (tokenizer_t::const_iterator tok_cit = tokens.begin(); tok_cit != tokens.end(); ++tok_cit)
    {
        ItemType tmpItem;
        std::string tok;
        tok = *tok_cit;
        trim(tok);

        if (parse_item(tok, tmpItem, default_item))
            result.push_back(tmpItem);
        else
        {
            if (skipped_items.empty())
                skipped_items += "[" + tok + "]";
            else
                skipped_items += ", [" + tok + "]";
        }
    }
    return result;
}


} /*namespace utils*/ } /*namespace parse*/

#endif //__UTILS_PARSE_H__
