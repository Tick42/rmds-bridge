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

class StatisticsLogger;
typedef boost::shared_ptr<StatisticsLogger> StatisticsLogger_ptr_t;

// The statistics logger collects and logs some basic performance metrics to a csv file

class StatisticsLogger
{
public:

	// static factory
	static StatisticsLogger_ptr_t GetStatisticsLogger();

	StatisticsLogger();
	virtual ~StatisticsLogger();

	void Initialise();

	bool Start();

	bool Stop();

	void Run();

	// stats interface
	void IncSubscribed()
	{
		++totalSubscriptions_;
	}

	void IncSubscriptionsSucceeded()
	{
		++totalSubscriptionsSucceeded_;
	}

	void IncSubscriptionsFailed()
	{
		++totalSubscriptionsFailed_;
	}

	void IncIncomingMessageCount()
	{
		++incomingMessageCount_;
	}

   void SetPendingOpens(int value)
   {
      pendingOpens_ = value;
   }

   void SetOpenItems(int value)
   {
      openItems_ = value;
   }     

   void SetPendingCloses(int value)
   {
      pendingCloses_ = value;
   }

	void SetRequestQueueLength(int value)
	{
		requestQueueLength_ = value;
	}

   static void PauseUpdates();
   static void ResumeUpdates();

private:
   bool buildRemoveNames(const boost::filesystem::path &directory
      , std::list<boost::filesystem::path> &removeNames);
   void deleteOldFiles();
   bool waitForInterval();

private:

	bool enabled_;
	int maxAgeDays_;
	int interval_;

	// upa consumer thread
	wthread_t loggerThread_; 

	volatile bool runThread_;

	// Pathname arithmetic
	boost::filesystem::path logFilename_;
   
	// manage time intervals
	RsslUInt64 lastSampleTime_;
	RsslUInt64 lastMessageCount_;
	RsslUInt64 lastSubscriptions_;
	RsslUInt64 lastSubscriptionsSucceeded_;
	RsslUInt64 lastSubscriptionsFailed_;

	// some basic stats
	RsslUInt64 incomingMessageCount_;
	RsslUInt64 totalSubscriptions_;
	RsslUInt64 totalSubscriptionsSucceeded_;
	RsslUInt64 totalSubscriptionsFailed_;

	RsslUInt64 requestQueueLength_;
	RsslUInt64 pendingOpens_;
	RsslUInt64 openItems_;
	RsslUInt64 pendingCloses_;

};



