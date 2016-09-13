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
#ifndef __UTILS_TIME_H__
#define __UTILS_TIME_H__

#include <wombat/port.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/timeb.h>
#endif 


namespace utils { namespace time {

/**
 *  @brief An inline replacement for GetTickCount
 *  @return current tick count in millisecond
 */
inline int32_t GetMilliCount()
{
#ifdef WIN32
    return (int32_t)GetTickCount();
#else
    // Something like GetTickCount but portable
    // It rolls over every ~ 12.1 days (0x100000/24/60/60)
    // Use GetMilliSpan to correct for rollover
    timeb tb;
    ftime( &tb );
    int32_t nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
    return nCount;
#endif
}

/**
 *  @brief yield the timespan in millisecond between the parameter and now. times should be given only from using the GetMilliSpan
 *  @param start: [input] should be a timestamp taken before
 *  @return elapsed time since start in millisecond
 */
inline int32_t GetMilliSpan( int32_t start )
{
#ifdef WIN32
    int32_t span = GetTickCount() - start;
    if ( span < 0 )
    span += 0x100000 * 1000;
    return span;
#else
    int32_t span = GetMilliCount() - start;
    if ( span < 0 )
    span += 0x100000 * 1000;
    return span;
#endif
}

/**
 *  @brief yield the timespan in millisecond between the parameter and now. times should be given only from using the GetMilliSpan
 *  @param now: [input] should be a timestamp taken now
 *  @param start: [input] should be a timestamp taken before
 *  @return elapsed time since start in millisecond
 */
inline int32_t GetMilliSpan( int32_t now, int32_t start)
{
    int32_t span = now - start;
    if ( span < 0 )
    span += 0x100000 * 1000;
    return span;
}

} /*namespace utils*/ } /*namespace time*/
#endif //__UTILS_TIME_H__
