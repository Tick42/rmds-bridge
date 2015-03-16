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
#ifndef __STDAFX_H__
#define __STDAFX_H__

#define _SCL_SECURE_NO_WARNINGS

// System headers
#include <cstdio>
#include <cstdint>
#include <string>
#include <fstream>
#include <climits>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <ctype.h>
#include <limits>
#include <list>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <memory>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <unistd.h>
#endif

// Boost headers

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/scope_exit.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/operators.hpp>
#include <boost/filesystem.hpp>

// Thompson-Reuters headers

#include "rtr/rsslMessagePackage.h"
#include <rtr/rsslIterators.h>
#include <rtr/rsslMsg.h>
#include <rtr/rsslMsgDecoders.h>
#include <rtr/rsslMsgEncoders.h>
#include <rtr/rsslPostMsg.h>
#include <rtr/rsslRDM.h>
#include <rtr/rsslTransport.h>
#include <rtr/rsslTypes.h>
#include <rtr/rsslDataTypeEnums.h>
#include <rtr/rsslDataDictionary.h>
#include <rtr/rsslDateTime.h>
#include <rtr/rsslTransport.h>
#include <rtr/rsslIterators.h>
#include <rtr/rsslMsgKey.h>
#include <rtr/rsslState.h>
#include <rtr/rsslQos.h>
#include <rtr/rsslReal.h>
#include <rtr/rsslDataDictionary.h>

// Open MAMA headers

#include <bridge.h>
#include <fielddescimpl.h>
#include <inboximpl.h>
#include <mama/mama.h>
#include <mama/io.h>
#include <mamainternal.h>
#include <msgimpl.h>
#include <property.h>
#include <queueimpl.h>
#include <subscriptionimpl.h>
#include <throttle.h>
#include <timers.h>
#include <transportimpl.h>

extern "C"
{
#   include <msgutils.h>
}

#include <wombat/port.h>
#include <wombat/queue.h>
#include <wombat/wUuid.h>

#undef shutdown
#ifdef str
#undef str
#endif

#endif /* __STDAFX_H__ */
