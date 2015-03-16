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
#include <utils/threadMonitor.h>
#include <utils/os.h>
#include <utils/t42log.h>

namespace utils { namespace os {

   ThreadMonitor::ThreadMonitor(const char *name)
   {
      threadName_ = name;
      t42log_debug("thread '%s' is starting\n", threadName_.c_str());
      setThreadName(wGetCurrentThreadId(), threadName_.c_str());
   }

   ThreadMonitor::~ThreadMonitor()
   {
      t42log_debug("thread '%s' is exiting\n", threadName_.c_str());
      threadName_.append("*");
      setThreadName(wGetCurrentThreadId(), threadName_.c_str());
   }

} /*namespace os*/ } /*namespace utils*/
