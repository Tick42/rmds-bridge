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

#include "StatisticsLogger.h"

#include "utils/properties.h"
#include "utils/time.h"
#include "utils/os.h"
#include "utils/t42log.h"
#include "utils/threadMonitor.h"

// get rid of this unwanted definition from the mama port.h. we need to be able to call fstream::close
#undef close

using namespace std;
using namespace boost;

static StatisticsLogger_ptr_t gLogger;

StatisticsLogger::StatisticsLogger()
{
    lastMessageCount_ = 0;
    incomingMessageCount_ = 0;
    lastSubscriptions_ = 0;
    lastSubscriptionsSucceeded_ = 0;
    lastSubscriptionsFailed_ = 0;
    totalSubscriptions_ = 0;
    totalSubscriptionsFailed_ = 0;
    totalSubscriptionsSucceeded_ = 0;
    maxAgeDays_ = 5;
    maxAgeDays_ = 5;
    requestQueueLength_ = 0;
    pendingOpens_ = 0;
    openItems_ = 0;
    pendingCloses_ = 0;
    loggerThread_ = 0;
    runThread_ = false;
    lastSampleTime_ = 0;
    queueEventsCount_ = 0;
}

const StatisticsLogger_ptr_t& StatisticsLogger::GetStatisticsLogger()
{
    // create one on first call
    if (gLogger.get() == 0)
    {
        gLogger = StatisticsLogger_ptr_t(new StatisticsLogger());
        gLogger->Initialise();
        gLogger->Start();
    }

    return gLogger;
}

void StatisticsLogger::Initialise()
{
    utils::properties config;

    logFilename_ = config.get("mama.tick42rmds.statslogger.logfile", "stats.csv");
    string logIntervalProp =  config.get("mama.tick42rmds.statslogger.interval", "10");
    interval_ = atoi(logIntervalProp.c_str());
    string logAgeProp =  config.get("mama.tick42rmds.statslogger.maxAgeDays", "5");
    maxAgeDays_ = atoi(logAgeProp.c_str());

    // work on the basis that an interval of 0 disables stats logging
    enabled_ = interval_ != 0;
}

static void * threadFuncStatistics(void * p)
{
    // The thread monitor outputs debug when the thread starts and stops
    utils::os::ThreadMonitor mon("RMDS-StatisticsLogger");

    StatisticsLogger * logger = (StatisticsLogger *) p;
    logger->Run();

    return 0;
}

bool StatisticsLogger::Start()
{
    if (!enabled_)
    {
        t42log_info("Statistics logging is not enabled");
        return false;
    }

    utils::thread::T42Lock lock(&cs_);

    if (!runThread_)
    {
        runThread_ = true;
        return (wthread_create( &loggerThread_, 0, threadFuncStatistics, this) == 0);
    }

    return true;
}

bool StatisticsLogger::Stop()
{
    if (!runThread_)
    {
        return true;
    }

    utils::thread::T42Lock lock(&cs_);

    if (!runThread_)
    {
        return true;
    }

    runThread_ = false;
    if (0 != loggerThread_)
    {
        wthread_join(loggerThread_, NULL);
        wthread_destroy(loggerThread_);
        loggerThread_ = 0;
    }

    return true;
}

// Stop the statistics logger when we pause updates, in case this is a
// precursor to shutting down the bridge completely. leaving the thread running
// creates a hazard when it calls sleep and it may crash while Mama destroys itself
void StatisticsLogger::PauseUpdates()
{
   if ((0 != gLogger) && gLogger->runThread_)
   {
      gLogger->Stop();
   }
}

// Restart the statistics logger thread after it was paused
void StatisticsLogger::ResumeUpdates()
{
   if ((0 != gLogger) && !gLogger->runThread_)
   {
      gLogger->Start();
   }
}

bool StatisticsLogger::waitForInterval()
{
   int index = 0;
   while (runThread_ && (index++ < interval_))
   {
      // Sleep in 1s increments so we can check if the thread has been stopped
      // mama port.h converts between Linux-seconds and Windows-milliseconds
      sleep(1);
   }
   // OK to carry on
   return runThread_;
}

bool StatisticsLogger::buildRemoveNames(const filesystem::path &directory
   , list<filesystem::path> &removeNames)
{
   time_t now = time(0);
   struct tm *tmToday = localtime(&now);
   filesystem::directory_iterator it(directory), end;
   for (; end != it; ++it)
   {
      filesystem::path filename = *it;

      // Check the format of this entry to see if it
      // matches the name of the statistics log file
      if (!filename.has_stem())
      {
         continue;
      }

      // Extract the date segment from the filename
      filesystem::path stem = filename.stem();
      if (!stem.has_extension())
      {
         continue;
      }
      string dateExtension = stem.extension().string();
      if (9 != dateExtension.size())
      {
         continue;
      }

      // The date segment should be ".YYYYMMDD"
      struct tm tmFile;
      memset(&tmFile, 0, sizeof(tmFile));
      int parsed = sscanf(dateExtension.c_str(), ".%04d%02d%02d"
         , &tmFile.tm_year, &tmFile.tm_mon, &tmFile.tm_mday);
      tmFile.tm_year -= 1900;
      --tmFile.tm_mon;
      if ((3 != parsed)
         || (2 < abs(tmToday->tm_year - tmFile.tm_year))
         || (0 > tmFile.tm_mon) || (11 < tmFile.tm_mon)
         || (1 > tmFile.tm_mday) || (31 < tmFile.tm_mday)
         )
      {
         continue;
      }

      // check the filename against the pattern given in the
      // mama.properties file, by excluding the date segment
      filesystem::path logfilePattern = filename.parent_path() / stem.replace_extension(filename.extension());
      if (logfilePattern != logFilename_)
      {
         // Ignore files that do not match the format:
         // <path><name>.YYYYMMDD.<ext>
         continue;
      }

      // If the file is too old, then we queue it for deletion
      // once we have finished with the directory_iterator
      time_t fileTime = mktime(&tmFile);
      const int secondsPerDay = 86400;
      if (maxAgeDays_ < ((now - fileTime) / secondsPerDay))
      {
         removeNames.push_back(filename);
      }
   }
   return 0 < removeNames.size();
}

void StatisticsLogger::deleteOldFiles()
{
   filesystem::path directory = logFilename_.parent_path();
   try
      {
      if (0 == directory.compare(filesystem::path()))
      {
         directory = filesystem::current_path();
      }
      if (filesystem::is_directory(directory))
      {
         list<filesystem::path> removeNames;
         if (buildRemoveNames(directory, removeNames))
         {
            list<filesystem::path>::iterator it = removeNames.begin()
               , end = removeNames.end();
            while (end != it)
            {
               t42log_info("deleting old statistics log file %s", it->string().c_str());
               if (!filesystem::remove(*it))
               {
                  t42log_warn("failed to delete file %s", it->string().c_str());
               }
               ++it;
            }
         }
      }
   }
   catch (...)
      {
      t42log_warn("Exception when deleting old statistics log files");
      }
}

void StatisticsLogger::Run()
{
   // Delete stats files that are old
   deleteOldFiles();

   char buffer[1024];
   const size_t BUFFERSIZE = (sizeof(buffer) / sizeof(buffer[0])) - 1;
   bool failed_new_file = false;
   RsslBuffer b;
   RsslDateTime last = RSSL_INIT_DATETIME, now;
   filesystem::path logPath;

   for (; runThread_; runThread_ = waitForInterval())
    {
      // If the date has changed, then we build a new filename
      rsslDateTimeLocalTime(&now);
      if (last.date.day != now.date.day)
      {
         last.date = now.date;

         // Create a timestamp to go between the filename and the extension
         string extension = logFilename_.extension().string();
         snprintf(&buffer[0], BUFFERSIZE, ".%04d%02d%02d%s"
            , now.date.year, now.date.month, now.date.day, extension.c_str());

         // Rebuild the pathname, using the date stamp
         logPath = logFilename_;
         logPath.replace_extension(&buffer[0]);
      }

      // Does the log file already exist?
      bool newLogFile = !filesystem::exists(logPath);

      // Try to open the log file, incorporating the date
      ofstream logFile;
      if (!failed_new_file)
         logFile.open(logPath.c_str(), ios_base::out | ios_base::app);

      if (!logFile.is_open())
      {


          if (!failed_new_file)
          {
              if (newLogFile)
                  failed_new_file =true;
              t42log_warn("Failed to open statistics log file %s", logPath.string().c_str());
          }
         // try again later
         continue;
      }

      // Output the header, if it's a new log file
      if (newLogFile)
      {
         sprintf(buffer,"Time,Updates (Total),Updates (Last),Update Rate"
            ",Subscriptions (Total),Subscriptions (Successful)"
            ",Subscriptions (Failed),Request Queue Length,Pending Opens,Open Items,Events In Queues");
         logFile << buffer << endl;
      }

        b.data = buffer;
        b.length = BUFFERSIZE;
        rsslDateTimeToString(&b, RSSL_DT_DATETIME, &now);
        string strDate(b.data, b.length);

        // Now format the stats into a string and write to the file
        RsslUInt64 ticks = utils::time::GetMilliCount();
        RsslUInt64 interval = ticks - lastSampleTime_;
        RsslUInt64 intervalMessages = incomingMessageCount_ - lastMessageCount_;
        double updateRate = ((double)intervalMessages * 1000) / interval;

        snprintf(buffer, BUFFERSIZE, "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", strDate.c_str()
         , (int)incomingMessageCount_, (int)intervalMessages, int(updateRate + 1)
         , (int)totalSubscriptions_, (int)totalSubscriptionsSucceeded_
         , (int)totalSubscriptionsFailed_, (int)requestQueueLength_
         , (int)pendingOpens_, (int)openItems_, (int)queueEventsCount_ );

        logFile << buffer << endl;
        logFile.close();

        lastMessageCount_ = incomingMessageCount_;
        lastSampleTime_ = ticks;
    }

    t42log_debug("Exit Statistics logger thread\n");
}
