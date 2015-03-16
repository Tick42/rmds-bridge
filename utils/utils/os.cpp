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
#include <utils/os.h>
#include <utils/thread/lock.h>
#include <utils/mama/types.h>
namespace /*anonymous*/
{
	utils::thread::lock_t globalUserNameStringLock; // see implementation of mama_getUserName for why the lock is needed
} /*namespace anonymous*/

namespace utils { namespace os {

bool getUserName(std::string &username)
{
	utils::thread::T42Lock synchronized(&globalUserNameStringLock);
	const char*     username_    =   NULL;
	mama_status     status      =   MAMA_STATUS_OK;

	status=mama_getUserName(&username_);
	if (MAMA_STATUS_OK == status)
	{
		username = username_;
		return true;
	}
	return false;
}


#  if defined(_WIN32)
// A function that names the thread for Microsoft Visual Studio
// to display when debugging
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // must be 0x1000
   LPCSTR szName; // pointer to name (in user addr space)
   DWORD dwThreadID; // thread ID (-1=caller thread)
   DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

void setThreadName(int threadID, const char *threadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = (DWORD)threadID;
   info.dwFlags = 0;

   // the exception is used to hook the name into the visual studio debugger
   __try
   {
	  RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR*)&info );
   }
   __except(EXCEPTION_CONTINUE_EXECUTION)
   {
   }
}
#else
void setThreadName(int, const char *)
{
}
#endif // defined(_WIN32)

} /*namespace utils*/ } /*namespace os*/