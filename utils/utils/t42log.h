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

#ifndef __UTILS_T42LOG_H__
#define __UTILS_T42LOG_H__

// Logging functions that dump formatted string to both screen and mama log file 
// screen dump is depend on mama.tick42rmds.consolelogging

// t42log_error: ERROR - MAMA_LOGLEVEL_ERROR
// t42log_warn: WARN - MAMA_LOGLEVEL_WARNING
// t42log_info: INFO - MAMA_LOGLEVEL_NORMAL
// t42log_debug: DEBUG - MAMA_LOGLEVEL_FINEST

// all log function follow the same printf format

// Uncomment this line to use the original debug code
//#define DISABLE_DEBUG_FILELINE

#ifndef DISABLE_DEBUG_FILELINE

#include <utils/thread/lock.h>

namespace Tick42Logging
{
   class CTick42Logger
   {
   public:
      bool enabled_;
      const char *filename_;
      size_t lineNumber_;
      MamaLogLevel logLevel_;

      static bool once_;
      static bool logConsole_;
#ifdef _WIN32
      static bool logODS_;
#endif
      static MamaLogLevel maxLogLevel_;
      static utils::thread::lock_t lock_;
      static size_t logBufferSize_;

      //////////////////////////////////////////////////////////////////////////
      //
      CTick42Logger(const char *filename, size_t lineNumber, MamaLogLevel logLevel);
      void operator()(const char *format, ...);
   };
}

#define t42log_debug Tick42Logging::CTick42Logger(__FILE__, __LINE__, MAMA_LOG_LEVEL_FINEST)
#define t42log_info  Tick42Logging::CTick42Logger(__FILE__, __LINE__, MAMA_LOG_LEVEL_NORMAL)
#define t42log_warn  Tick42Logging::CTick42Logger(__FILE__, __LINE__, MAMA_LOG_LEVEL_WARN)
#define t42log_error Tick42Logging::CTick42Logger(__FILE__, __LINE__, MAMA_LOG_LEVEL_ERROR)

#else   // DISABLE_DEBUG_FILELINE

#ifdef __cplusplus
extern "C" {
#endif

void t42log_error(const char * format, ...);
void t42log_warn(const char * format, ...);
void t42log_info(const char * format, ...);
void t42log_debug(const char * format, ...);

#ifdef __cplusplus
};
#endif   // DISABLE_DEBUG_FILELINE

#endif
#endif //__UTILS_T42LOG_H__

