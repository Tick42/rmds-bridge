/*
* MamaMsgWrapper: The Reuters RMDS Bridge for OpenMama
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

#include "MamaOpaqueWrapper.h"


MamaOpaqueWrapper::MamaOpaqueWrapper(const void * pData, size_t length)
    :length_(length)
{
    // for the moment, unless we need to do anything more sophisticated, lets just use malloc and free.
    data_ = malloc(length);
    memcpy(data_, pData, length);
}

MamaOpaqueWrapper::~MamaOpaqueWrapper()
{
    free(data_);
}
