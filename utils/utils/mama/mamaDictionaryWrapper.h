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
#ifndef __UTILS_MAMA_MAMADICTIONARYWRAPPER_H_
#define __UTILS_MAMA_MAMADICTIONARYWRAPPER_H_

#include <string>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <utils/mama/types.h>
#include <utils/mama/mamaFieldDescriptorWrapper.h>

namespace utils { namespace mama {

class mamaDictionaryWrapper_fail_init : public std::runtime_error
{
public:
	mamaDictionaryWrapper_fail_init() : std::runtime_error("Failed to initialize mamaDictionaryWrapper object") {}
	virtual ~mamaDictionaryWrapper_fail_init() throw() {}
};

class mamaDictionaryWrapper;
typedef boost::shared_ptr<mamaDictionaryWrapper> mamaDictionaryWrapper_ptr_t;
/**
 * Thin wrapper with reference count around a mamaDictionary resource
 *
 */
class mamaDictionaryWrapper
{
	mamaDictionary dictionary_;
	mutable int *references_;
	bool valid_reference_state;
public:
	mamaDictionaryWrapper(void);
	mamaDictionaryWrapper(std::string path);
	mamaDictionaryWrapper &operator=(const mamaDictionaryWrapper &rhs);
	mamaDictionaryWrapper(const mamaDictionaryWrapper &rhs);
	~mamaDictionaryWrapper(void);
	inline int use_count() const {return *references_;}
	inline bool valid() const {return (valid_reference_state && references_ != NULL && *references_ != 0);}

	const mamaDictionary &get() const {return dictionary_;}
	mamaDictionary &get() {return dictionary_;}
	mama_status populateFromFile(std::string path);
	returned<mama_fid_t> getMaxFid() const;
	returned<size_t> getSize() const;
	returned<mamaFieldDescriptorRef> getFieldDescriptorByName(const char *fname) const 
	{
		returned<mamaFieldDescriptorRef> result;
		result.status = mamaDictionary_getFieldDescriptorByName (dictionary_, 	&result.value.get(), fname);
		return result;
	}
	returned<mamaFieldDescriptorRef> getFieldDescriptorByName(std::string fname) const 
	{
		returned<mamaFieldDescriptorRef> result;
		result.status = mamaDictionary_getFieldDescriptorByName (dictionary_, 	&result.value.get(), fname.c_str());
		return result;
	}
	returned<mamaFieldDescriptorRef> getFieldDescriptorByFid(mama_fid_t fid) const 
	{
		returned<mamaFieldDescriptorRef> result;
		result.status = mamaDictionary_getFieldDescriptorByFid (dictionary_, &result.value.get(), fid);
		return result;
	}
	returned<mamaFieldDescriptorRef> getFieldDescriptorByIndex(unsigned short index) const 
	{
		returned<mamaFieldDescriptorRef> result;
		result.status = mamaDictionary_getFieldDescriptorByIndex (dictionary_, &result.value.get(), index);
		return result;
	}
	returned<mamaFieldDescriptorRef> createFieldDescriptor(mama_fid_t fid, const char* name, mamaFieldType type)
	{
		returned<mamaFieldDescriptorRef> result;
		result.status =  mamaDictionary_createFieldDescriptor (dictionary_, fid, name, type, &result.value.get());
		return result;
	}
	returned<mamaFieldDescriptorRef> createFieldDescriptor(mama_fid_t fid, std::string name, mamaFieldType type)
	{
		returned<mamaFieldDescriptorRef> result;
		result.status =  mamaDictionary_createFieldDescriptor (dictionary_, fid, name.c_str(), type, &result.value.get());
		return result;
	}
	returned<bool> hasDuplicates()
	{
		returned<bool> result;
		int tmp;
		result.status = mamaDictionary_hasDuplicates (dictionary_, &tmp);
		result.value = tmp == 0;
		return result;
	}

private:
	void ReleaseOwnership();
};

} /*namespace utils*/ } /*namespace mama*/
#endif //__UTILS_MAMA_MAMADICTIONARYWRAPPER_H_
