/*
* Utils: Tick42 Middleware Utilities
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
#include <utils/parse.h>

namespace utils { namespace parse {

using boost::lexical_cast;
using boost::bad_lexical_cast;

bool parse_retrysched_item( const std::string &retrysched_item, retrysched_t &result, const retrysched_t &default_item)
{
	std::vector<std::string> tokens;
    char * pch;
    char * str = strdup(retrysched_item.c_str());

    bool status = false;

    std::vector<std::string> delimiters;
    delimiters.push_back("(");
    delimiters.push_back(")");
    delimiters.push_back("");
    pch = strtok (str,delimiters[0].c_str());

    for (int i=0; pch != NULL && i < 3; ++i)
    {
        tokens.push_back(pch);
        pch = strtok (NULL, i == 0 ? delimiters[1].c_str() : delimiters[2].c_str());
    }

    size_t tokens_size = tokens.size();

    if ( tokens_size <= 2 && tokens_size > 0)
    {
			unsigned int delay;
			unsigned int retry;

        if (tokens_size==1) //for example <number> only
        {
            try
            {
                delay = lexical_cast<unsigned int>(tokens[0]);
                if (delay >= std::numeric_limits<unsigned int>::min() && delay <= std::numeric_limits<unsigned int>::max())
                {
                    result.delay = delay;
                    result.retry = default_item.retry;
                    status = true;
                }

            }
            catch (const bad_lexical_cast &)
            {
                status = false;
            }
        }
        else if (tokens_size==2) // for example <number>(<number>)
        {
            try
            {
                delay = lexical_cast<unsigned int>(tokens[0]);
                retry = lexical_cast<unsigned int>(tokens[1]);

                if (delay >= std::numeric_limits<unsigned int>::min() && delay <= std::numeric_limits<unsigned int>::max() &&
                    retry >= std::numeric_limits<unsigned int>::min() && retry <= std::numeric_limits<unsigned int>::max() )
                {
                    result.delay = delay;
                    result.retry = retry;
                    status = true;
                }
            }
            catch (const bad_lexical_cast &)
            {
                status = false;
            }
        }
    }

    if (str)
        free(str);

    return status;
}

bool parse_host_item( const std::string &host_item, host_t &result, const host_t &default_item)
{
	std::vector<std::string> tokens;
    char * pch;
    char * str = strdup(host_item.c_str());

    bool status = false;

    pch = strtok (str,":");

    for (int i=0; pch != NULL && i < 3; ++i)
    {
        tokens.push_back(pch);
        pch = strtok (NULL, ":");
    }
    size_t tokens_size = tokens.size();

    if ( tokens_size <= 2 && tokens_size > 0)
    {
        if (tokens_size==1)
        {
            result.host = tokens[0];
            result.port = default_item.port; //default
            status = true;
        }
        else if (tokens_size==2)
        {
            result.host = tokens[0];
            result.port = tokens[1];
            status = true;
        }
    }

    if (str)
        free(str);

    return status;
}
} /*namespace utils*/ } /*namespace parse*/
