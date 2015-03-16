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
#include <utils/t42log.h>

#ifndef DISABLE_DEBUG_FILELINE

namespace Tick42Logging
{
   //////////////////////////////////////////////////////////////////////////
   //
   bool CTick42Logger::once_ = false;
   bool CTick42Logger::logConsole_ = false;
#ifdef _WIN32
   bool CTick42Logger::logODS_ = false;
#endif
   MamaLogLevel CTick42Logger::maxLogLevel_ = MAMA_LOG_LEVEL_OFF;
   utils::thread::lock_t CTick42Logger::lock_;
   size_t CTick42Logger::logBufferSize_ = 4096;

   //////////////////////////////////////////////////////////////////////////
   //
   CTick42Logger::CTick42Logger(const char *filename, size_t lineNumber, MamaLogLevel logLevel)
      : filename_(filename)
      , lineNumber_(lineNumber)
      , logLevel_(logLevel)
   {
      // Initialize the statics, if we haven't already done so
      if (!once_)
      {
         once_ = true;

         const char *val = properties_Get(mamaInternal_getProperties(), "mama.logging.level");
         if ((val == 0) || (mama_tryStringToLogLevel(val, &maxLogLevel_) == 0))
         {
            maxLogLevel_ = MAMA_LOG_LEVEL_OFF;
         }

         val = properties_Get(mamaInternal_getProperties(), "mama.tick42rmds.consolelogging");
         logConsole_ = (0 != val) ? (0 != properties_GetPropertyValueAsBoolean(val)) : false;
#ifdef _WIN32
         // Do we log to the system debugger instead of the mama log file?
         val = properties_Get(mamaInternal_getProperties(), "mama.tick42rmds.ODSloggingOnly");
         logODS_ = (0 != val) ? (0 != properties_GetPropertyValueAsBoolean(val)) : false;
#endif
      }

      enabled_ = (MAMA_LOG_LEVEL_OFF != maxLogLevel_) && (logLevel <= maxLogLevel_);
   }

   //////////////////////////////////////////////////////////////////////////
   //
   void CTick42Logger::operator()(const char *format, ...)
   {
      va_list ap;
      va_start(ap, format);

      // Only process if this logging level is enabled
      if (enabled_)
      {
         char *buf = static_cast<char *>(alloca(logBufferSize_ * sizeof(*buf)));
         int off = snprintf(buf, logBufferSize_ - 1, "%s(%u): ", filename_, lineNumber_);
         off += vsnprintf(buf + off, logBufferSize_ - off, format, ap);

         // Strip one trailing '\n', if there is one
         // If you want a blank line in the debug log file, then include
         // two '\n' sequences at the end of your format string
         if ((0 < off) && (buf[off - 1] == '\n'))
         {
            buf[--off]  = '\0';
         }

#ifdef _WIN32
         if (logODS_)
         {
            buf[off++] = '\r';
            buf[off++] = '\n';
            buf[off] = '\0';
            OutputDebugStringA(buf);
         }
         else
#endif // _WIN32
         {
            mama_log(logLevel_, "%s", buf);
         }

         if (logConsole_)
         {
            utils::thread::T42Lock sync(&lock_);
            fprintf(stdout, "%s\n", buf);
            fflush(stdout);
         }
      }
      va_end(ap);
   }
} // namespace

#else // DISABLE_DEBUG_FILELINE

#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <utils/properties.h>
#include <utils/thread/lock.h>

#include <stdio.h>

#include <mamainternal.h>
#include <mama/mama.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996) //suppress warning for unsafe C libraries
#endif

namespace
{
   inline bool IsInLogLevel(MamaLogLevel log_level)
   {
      static utils::properties properties;
      static MamaLogLevel cfg_log_level;

      static bool once=true;
      if (once)
      {
         once =false;
         cfg_log_level = properties.get("mama.logging.level", MAMA_LOG_LEVEL_OFF);
      }
      return log_level <= cfg_log_level;
   }

   inline bool IsConsoleLogging()
   {
      static utils::properties properties;
      static bool consolelogging;

      static bool once=true;
      if (once)
      {
         once =false;
         consolelogging = properties.get("mama.tick42rmds.consolelogging", false);
      }

      return consolelogging;
   }

   static utils::thread::lock_t ErrorLock;
   static utils::thread::lock_t WarnLock;
   static utils::thread::lock_t InfoLock;
   static utils::thread::lock_t DebugLock;

}
using namespace utils::thread;

extern "C" void t42log_error(const char * format, ...)
{
   char buffer[4096];
   *buffer = 0;

   va_list args;
   va_start (args, format);

   static const MamaLogLevel logLevel = MAMA_LOG_LEVEL_ERROR;
   if (IsInLogLevel(logLevel))
   {
      vsnprintf (buffer,sizeof(buffer)-1,format, args);
      if (IsConsoleLogging())
      {
         T42Lock sync(&ErrorLock);
         fprintf(stdout, "%s", buffer);
         fflush(stdout);
      }
      mama_log(MAMA_LOG_LEVEL_ERROR, "%s", buffer);
   }
   va_end (args);
}

extern "C" void t42log_warn(const char * format, ...)
{
   char buffer[4096];
   *buffer = 0;

   va_list args;
   va_start(args, format);

   static const MamaLogLevel logLevel = MAMA_LOG_LEVEL_WARN;
   if (IsInLogLevel(logLevel))
   {
      vsnprintf (buffer, sizeof(buffer) - 1, format, args);
      if (IsConsoleLogging())
      {
         T42Lock sync(&WarnLock);
         fprintf (stdout, "%s", buffer);
         fflush (stdout);
      }

      mama_log (logLevel, "%s", buffer);
   }

   va_end (args);
}

extern "C" void t42log_info(const char * format, ...)
{
   char buffer[4096];
   *buffer = 0;

   va_list args;
   va_start (args, format);

   static const MamaLogLevel logLevel = MAMA_LOG_LEVEL_NORMAL;
   if (IsInLogLevel(logLevel))
   {
      vsnprintf (buffer, sizeof(buffer) - 1, format, args);
      if (IsConsoleLogging())
      {
         T42Lock sync(&InfoLock);
         fprintf (stdout, "%s", buffer);
         fflush (stdout);
      }

      mama_log (logLevel, "%s", buffer);
   }

   va_end (args);
}

extern "C" void t42log_debug(const char * format, ...)
{
   char buffer[4096];
   *buffer = 0;

   va_list args;
   va_start (args, format);

   static const MamaLogLevel logLevel = MAMA_LOG_LEVEL_FINEST;
   if (IsInLogLevel(logLevel))
   {
      vsnprintf (buffer, sizeof(buffer) - 1, format, args);

      if (IsConsoleLogging())
      {
         T42Lock sync(&DebugLock);

         fprintf (stdout, "%s", buffer);
         fflush (stdout);
      }
      mama_log (logLevel, "%s", buffer);
   }
   va_end (args);
}

#endif // DISABLE_DEBUG_FILELINE
