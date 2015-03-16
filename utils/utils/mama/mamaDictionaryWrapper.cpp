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
#include <utils/mama/mamaDictionaryWrapper.h>

namespace utils { namespace mama {

mamaDictionaryWrapper::mamaDictionaryWrapper( void ) 
	: references_(NULL)
	, dictionary_(NULL)
	, valid_reference_state(false)
{
	if (mamaDictionary_create (	&dictionary_) != MAMA_STATUS_OK)
		throw mamaDictionaryWrapper_fail_init();
	references_ = new int(0);
	*references_=1;
	valid_reference_state =true;
}

mamaDictionaryWrapper::mamaDictionaryWrapper( std::string path )
	: references_(NULL)
	, dictionary_(NULL)
	, valid_reference_state(false)
{
	references_ = new int(0);
	if (mamaDictionary_create (	&dictionary_) != MAMA_STATUS_OK)
		throw mamaDictionaryWrapper_fail_init();
	*references_=1;
	if (mamaDictionary_populateFromFile (
		dictionary_,
		path.c_str()) != MAMA_STATUS_OK)
			throw mamaDictionaryWrapper_fail_init();
	valid_reference_state =true;
}

mamaDictionaryWrapper::mamaDictionaryWrapper( const mamaDictionaryWrapper &rhs )
	: references_(rhs.references_)
	, dictionary_(rhs.dictionary_)
	, valid_reference_state(false)
{
	if (this != &rhs)
	{
		++(*references_);
		valid_reference_state =true;
	}
	else
		valid_reference_state =false; //you can't construct from yourself!
}

mamaDictionaryWrapper::~mamaDictionaryWrapper( void )
{
	ReleaseOwnership();
	valid_reference_state = false;
}

mamaDictionaryWrapper &mamaDictionaryWrapper::operator=( const mamaDictionaryWrapper &rhs )
{
	if (this != &rhs)
	{
		//increase ownership reference count only when the dictionaries on both sides are different
		//there is no point increasing reference count when they both holds the same dictionary
		//that will create a memory leak!
		if (this->dictionary_ != rhs.dictionary_)
		{
			ReleaseOwnership();
			valid_reference_state = false;
			this->dictionary_ = rhs.dictionary_;
			this->references_ = rhs.references_;
			if (references_)
				++(*references_); 
			valid_reference_state =true;
		}
	}
	return *this;
}

mama_status mamaDictionaryWrapper::populateFromFile( std::string path )
{
	return mamaDictionary_populateFromFile (dictionary_, path.c_str());
}

returned<mama_fid_t> mamaDictionaryWrapper::getMaxFid() const
{
	returned<mama_fid_t> result;
	result.status = mamaDictionary_getMaxFid (dictionary_, &result.value);
	return result;
}

returned<size_t> mamaDictionaryWrapper::getSize() const
{
	returned<size_t> result;
	result.status = mamaDictionary_getSize (dictionary_, &result.value);
	return result;
}

void mamaDictionaryWrapper::ReleaseOwnership()
{
	if (valid_reference_state && references_)
	{
		--(*references_);
		if (*references_==0)
		{
			try
			{
				delete references_;
				references_ = NULL;
				mama_status res = MAMA_STATUS_OK;
				res = mamaDictionary_destroy (dictionary_);
			}
			catch(...)
			{
				mama_log(MAMA_LOG_LEVEL_ERROR, "Could not destroy mamaDictionaryWrapper");
			}
			dictionary_ = NULL;
		}
		else
		{
			references_ = NULL;
			dictionary_ = NULL;
		}
	}

}

} /*namespace utils*/ } /*namespace mama*/
