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
#ifndef __UPACONSUMER_H__
#define __UPACONSUMER_H__

/* defines maximum allowed name length for this application */
#define MAX_ITEM_NAME_STRLEN 128
#define MAX_STREAM_ID_RANGE_PER_DOMAIN 100

#include "UPAMessage.h"
#include "tick42rmdsbridgefunctions.h"
#include "utils/HiResTime.h"
#include "UPAStreamManager.h"
#include "UPADictionary.h"

// for the post manager 
#include "UPAPostManager.h"
#include "transportconfig.h"
#include "RMDSConnectionConfig.h"
#include "ConnectionListener.h"
#include "StatisticsLogger.h"


extern "C"
{
	static RsslBool isInLoginSuspectState;
}

class RMDSSubscriber;
class UPALogin;
class LoginResponseListener;
class ConnectionListener;
class UPASourceDirectory;
class SourceDirectoryResponseListener; 
class UPADictionary;
class DictionaryResponseListener;
class PublishMessageRequest;

// The UPAConsumer is the class that runs the subscribing socket thread that connects to the ADS
// It writes item requests and posted messages
// it reads incoming data and status messages

class UPAConsumer
{
public:
	UPAConsumer(RMDSSubscriber * pOwner);
	~UPAConsumer(void);

	void Run();

	bool IsConnectionConfigValid();


	// The request and callback methods marshall requests from the mama client thread onto the consumer thread through the request queue
	mamaQueue RequestQueue() const { return requestQueue_; }
	// dictionary request and notification
	static void MAMACALLTYPE DictionaryRequestCb(mamaQueue, void * closure);
	bool RequestDictionary( mamaQueue requestQueue);

	// dispatch client request for dictionary subscription
	bool ClientRequestDictionary(mamaQueue requestQueue);
	static void MAMACALLTYPE ClientDictionaryRequestCb(mamaQueue queue, void * closure);

	const UPADictionary *UpaDictionary() const { return upaDictionary_.get(); }
	UPADictionary *UpaDictionary() { return upaDictionary_.get(); }
	// login request and completion notification
	static void MAMACALLTYPE LoginRequestCb(mamaQueue queue,void *closure);
	bool RequestLogin(mamaQueue requestQueue);

	// source directory request and notification
	static void MAMACALLTYPE SourceDirectoryRequestCb(mamaQueue, void * closure);
	bool RequestSourceDirectory(mamaQueue requestQueue);

	RsslUInt32 LoginStreamId() const;

	// connection notifications
	void AddListener( ConnectionListener * pListener );


	// Accessors
	UPAStreamManager & StreamManager()  { return streamManager_; }
	UPAPostManager & PostManager()  { return postManager_; }
	RsslChannel * RsslConsumerChannel() const { return rsslConsumerChannel_; }
	UPASourceDirectory *SourceDirectory() { return sourceDirectory_; }
	UPADictionaryWrapper_ptr_t RsslDictionary()	{return upaDictionary_->RsslDictionary();}
	RMDSSubscriber * GetOwner() { return owner_; }
	bool RequiresConnection() const	{return requiresConnection_;}

   std::string getTransportName();

	// Stats functions

	void StatsSubscribed()
	{
		statsLogger_->IncSubscribed();
	}

	void StatsSubscriptionsSucceeded()
	{
		statsLogger_->IncSubscriptionsSucceeded();
	}

	void StatsSubscriptionsFailed()
	{
		statsLogger_->IncSubscriptionsFailed();
	}

	void WarnMissingFid(RsslFieldId fid);

   void JoinThread(wthread_t thread);

   mamaMsg MamaMsg() const { return msg_; }


private:
	// rssl connection
	fd_set	readfds_;
	fd_set	exceptfds_;
	fd_set	wrtfds_;


	RsslChannel* ConnectToRsslServer(const std::string &hostname, const std::string &port, char* interfaceName, RsslConnectionTypes connType, RsslError* error);

	void RecoverConnection();
	void RemoveChannel(RsslChannel* chnl);
	void InitPingHandler(RsslChannel* chnl);


	RsslChannel *rsslConsumerChannel_;

	RsslRet ReadFromChannel(RsslChannel* chnl);
	void ProcessPings(RsslChannel* chnl);

	RsslBool shouldRecoverConnection_;
	bool requiresConnection_;

	RsslUInt32 pingTimeoutServer_;
	RsslUInt32 pingTimeoutClient_;
	time_t nextReceivePingTime_;
	time_t nextSendPingTime_;


	RsslRet ProcessResponse(RsslChannel* chnl, RsslBuffer* buffer);

	RsslRet ProcessOffStreamResponse(RsslMsg* msg, RsslDecodeIterator* dIter);

	RsslBool receivedServerMsg_;

	RMDSConnectionConfig connectionConfig_;
	char* interfaceName_;
	RsslConnectionTypes connType_;


	RMDSSubscriber * owner_;

	mamaQueue requestQueue_;

	UPALogin * login_;

	UPASourceDirectory * sourceDirectory_;

	boost::shared_ptr<UPADictionary> upaDictionary_;


	RsslBool isInLoginSuspectState_;

	// some basic stats
	RsslUInt64 incomingMessageCount_;
	RsslUInt64 lastMessageCount_;
	RsslUInt32 lastSampleTime_;
	RsslUInt64 totalSubscriptions_;
	RsslUInt64 lastSubscriptions_;
	RsslUInt64 totalSubscriptionsSucceeded_;
	RsslUInt64 lastSubscriptionsSucceeded_;
	RsslUInt64 totalSubscriptionsFailed_;
	RsslUInt64 lastSubscriptionsFailed_;

	StatisticsLogger_ptr_t statsLogger_;

	// Handle connection
	//
	std::vector<ConnectionListener*> listeners_;

	void NotifyListeners(bool connected, std::string extraInfo);

	UPAStreamManager streamManager_;

	UPAPostManager postManager_;

	volatile bool runThread_;

	// manage reconnection
	void WaitReconnectionDelay();
	void LogReconnection();

	// pump incoming events from mama queue
	bool PumpQueueEvents();

	// request throttling
	size_t maxDispatchesPerCycle_;
	size_t maxPendingOpens_;


	// Re-usable message object
	// this is used by all the subscriptions on this thread. We can share this because currently
	// the message is send up to the client synchronously
	mamaMsg msg_;


};

#endif //__UPACONSUMER_H__
