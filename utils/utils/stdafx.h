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
#pragma once
#ifndef __STDAFX_INC__
#define __STDAFX_INC__

// System headers

#include <string>
#include <unordered_map>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <stdio.h>

// Boost headers

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#ifdef WIN32
// avoid warnings about posix compliance
#define strdup _strdup
#endif

// Open MAMA headers

#include <mama/mama.h>
#include <mama/dictionary.h>
#include <mama/types.h>
#include <mama/log.h>
#include <property.h>
#include <mamainternal.h>

extern "C"
{
#include <wombat/environment.h>
};

#include <wombat/port.h>
#undef shutdown
#undef min
#undef max

#endif /* __STDAFX_INC__ */
