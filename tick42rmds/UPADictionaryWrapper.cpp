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
#include "UPADictionaryWrapper.h"

namespace {

/* Last Error buffer size */
static const size_t sizeErrTxt = 256;

}

UPADictionaryWrapper::UPADictionaryWrapper() 
{
    size_t sizeErrTxt = 256;
    lastErrTxt_ = (char *)malloc(sizeErrTxt);
    memset(lastErrTxt_, 0, sizeErrTxt);
    lastErrorTextBuffer_.length = (RsslUInt32)sizeErrTxt-1; //suitable for ASCIIZ buffers
    lastErrorTextBuffer_.data = (char*)lastErrTxt_;

    memset(&dictionary_,0, sizeof(dictionary_));
    rsslClearDataDictionary(&dictionary_);
}

UPADictionaryWrapper::~UPADictionaryWrapper()
{
    try {
        clear();
        //free buffers
        free(lastErrTxt_);
    } catch(...) {
        ; 
    }
}


load_status_t UPADictionaryWrapper::LoadFieldDictionary(std::string filename)
{

    if (rsslLoadFieldDictionary(filename.length() == 0 ? DictionaryFileName : filename.c_str(), &dictionary_, &lastErrorTextBuffer_) >= 0)
    {
        fieldDictionaryStatus_.loaded = true;
        fieldDictionaryStatus_.loaded_from_file = true;
    }

    return fieldDictionaryStatus_;
}

load_status_t UPADictionaryWrapper::LoadFieldDictionary()
{
    return LoadFieldDictionary(DictionaryFileName);
}

load_status_t UPADictionaryWrapper::LoadEnumTypeDictionary(std::string filename)
{
    if (rsslLoadEnumTypeDictionary(filename.length() == 0 ? EnumTableFileName : filename.c_str(), &dictionary_, &lastErrorTextBuffer_) >= 0)
    {
        enumTypeDictionaryStatus_.loaded = true;
        enumTypeDictionaryStatus_.loaded_from_file = true;
    }

    return enumTypeDictionaryStatus_;
}

load_status_t UPADictionaryWrapper::LoadEnumTypeDictionary()
{
    return LoadEnumTypeDictionary(EnumTableFileName);
}

bool UPADictionaryWrapper::clear()
{
    bool result = false;
    if (dictionary_.isInitialized)
    {
        rsslDeleteDataDictionary(&dictionary_);
        rsslClearDataDictionary(&dictionary_);
        memset(lastErrTxt_, 0, sizeErrTxt);
        result = true;
    }
    return result;
}
