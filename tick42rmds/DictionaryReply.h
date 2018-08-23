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
#ifndef __DICTIONARYREPLY_H__
#define __DICTIONARYREPLY_H__

#include <boost/shared_ptr.hpp>

/*
 * DictionaryReply_t is an abstract class providing a Send method. Concrete instances wrap the response to a data dictionary subscription
)
 */
class DictionaryReply_t
{
public:
    virtual mama_status Send() =0;
    virtual ~DictionaryReply_t()
    {
        //printf("destructor DictionaryReply_t");
    } //<- in case there are some bound variables need cleaning.
};

typedef boost::shared_ptr<DictionaryReply_t> DictionaryReply_ptr_t;

#endif //__DICTIONARYREPLY_H__
