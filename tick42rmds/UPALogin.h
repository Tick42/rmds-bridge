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
#ifndef __UPALOGIN_H__
#define __UPALOGIN_H__

#include "UPAMessage.h"

class UPAConsumer;
class TransportConfig_t;
class LoginResponseListener;

const int MAX_LOGIN_INFO_STRLEN = 128;

using namespace std;

// wrapper around the rssl login message. 
// Encodes a login request (for a subscriber) and decodes it (for a publisher). 
// Decodes a login response (for a subscriber) and encodes it for a publisher
class UPALogin
{

public:
	//login request rejection reasons
	typedef enum
	{
		MaxLoginRequestsReached	= 0,
		NoUserName	= 1
	} RsslLoginRejectReason;

public:
	UPALogin(bool provider);
	~UPALogin(void);

    void ConfigureEntitlements(TransportConfig_t * pConfig);
	void AddListener(LoginResponseListener * pListener);
	bool SendLoginRequest();

	RsslRet processLoginResponse(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter);

	RsslChannel* UPAChannel() const { return UPAChannel_; }
	void UPAChannel(RsslChannel* val) { UPAChannel_ = val; }

	bool QueueLogin(mamaQueue Queue);
	bool QueueLogin(mamaQueue queue, mamaQueueEventCB callback);

	std::string UserName() const { return userName_; }
	std::string AppName() const { return appName_; }
	std::string AppID() const { return appID_; }
	bool ProvidePermissionProfile() const { return providePermissionProfile_; }
	void ProvidePermissionProfile(bool val) { providePermissionProfile_ = val; }
	bool ProvidePermissionExpressions() const { return providePermissionExpressions_; }
	void ProvidePermissionExpressions(bool val) { providePermissionExpressions_ = val; }
	bool SingleOpen() const { return singleOpen_; }
	void SingleOpen(bool val) { singleOpen_ = val; }
	bool AllowSuspectData() const { return allowSuspectData_; }
	void AllowSuspectData(bool val) { allowSuspectData_ = val; }
	std::string InstanceId() const { return instanceId_; }
	void InstanceId(std::string val) { instanceId_ = val; }
	RsslUInt64 Role() const { return role_; }
	void Role(RsslUInt64 val) { role_ = val; }
	bool DownloadConnectionConfig() const { return downloadConnectionConfig_; }
	void DownloadConnectionConfig(bool val) { downloadConnectionConfig_ = val; }
	bool DisableDataConversion() const { return disableDataConversion_; }
	void DisableDataConversion(bool val) { disableDataConversion_ = val; }

	RsslUInt32 StreamId() const;

	// access state
	RsslBool IsClosed() const { return isClosed_; }
	RsslBool IsClosedRecoverable() const { return isClosedRecoverable_; }
	RsslBool IsSuspect() const { return isSuspect_; }
	RsslBool IsNotEntitled() const { return isNotEntitled_; }

	RsslRet SendLoginRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslLoginRejectReason reason);

	 //login response information 
typedef struct
{
	RsslInt32	StreamId;
	char		Username[MAX_LOGIN_INFO_STRLEN];
	char		ApplicationId[MAX_LOGIN_INFO_STRLEN];
	char		ApplicationName[MAX_LOGIN_INFO_STRLEN];
	char		Position[MAX_LOGIN_INFO_STRLEN];
	RsslUInt64	ProvidePermissionProfile;
	RsslUInt64	ProvidePermissionExpressions;
	RsslUInt64	SingleOpen;
	RsslUInt64	AllowSuspectData;
	RsslUInt64	SupportPauseResume;
	RsslUInt64	SupportOptimizedPauseResume;
	RsslUInt64	SupportOMMPost;
	RsslUInt64	SupportViewRequests;
	RsslUInt64	SupportBatchRequests;
	RsslUInt64	SupportStandby;
	RsslBool	isSolicited;
} RsslLoginResponseInfo;

	 //login request information 
typedef struct
{
	RsslInt32	StreamId;
	char		Username[MAX_LOGIN_INFO_STRLEN];
	char		ApplicationId[MAX_LOGIN_INFO_STRLEN];
	char		ApplicationName[MAX_LOGIN_INFO_STRLEN];
	char		Position[MAX_LOGIN_INFO_STRLEN];
	char		Password[MAX_LOGIN_INFO_STRLEN];
	RsslUInt64	ProvidePermissionProfile;
	RsslUInt64	ProvidePermissionExpressions;
	RsslUInt64	SingleOpen;
	RsslUInt64	AllowSuspectData;
	char		InstanceId[MAX_LOGIN_INFO_STRLEN];
	RsslUInt64	Role;
	RsslUInt64	DownloadConnectionConfig;
	RsslChannel*	UPAChannel;
	RsslBool		IsInUse;
} RsslLoginRequestInfo;

// decode incoming login request
RsslRet DecodeLoginRequest(RsslLoginRequestInfo* loginReqInfo, RsslMsg* msg, RsslDecodeIterator* dIter, RsslMsgKey* requestKey);

// send  login response
RsslRet SendLoginResponse(RsslChannel* chnl, RsslLoginResponseInfo * loginRespInfo);

// Get the machine's hostname for DACS and other purposes
// Caches the value after the initial call for performance reasons
static const char *GetHostName(RsslUInt32 *length = 0);


// get the loginRequestInfo
UPALogin::RsslLoginRequestInfo * LoginRequestInfo()
{
	return &loginRequestInfo_;
}

private:
	string userName_;
	string appName_; // Identifies the application sending the Login request or	response message. When present, the application	name in the Login request identifies the OMM Consumer,
	string appID_;	// DACS application ID
	string password_;

   static std::string hostName_;

	// todo decide whether these should be bools or Rssl integer types - for the moment lets stick with bool 

	bool providePermissionProfile_; //When specified on a Login Request, indicates that a consumer desires the permission profile. The permission profile can be used by an application to perform proxy permissioning.
	bool providePermissionExpressions_; //If specified on the Login Request, this indicates a consumer wants permission expression information to be sent with responses. Permission expressions	allow for items to be proxy permissioned by
	bool singleOpen_ ; // Indicates the consumer application wants the provider to drive stream recovery. 0 Indicates that the consumer application will drive stream recovery.
	bool allowSuspectData_; //  Indicates that the consumer application allows for suspect streamState information. 0: Indicates that the consumer application prefers any suspect data result in the stream being closed with an RSSL_STREAM_
	string instanceId_; // InstanceId can be used to differentiate applications running on the same machine.
	bool downloadConnectionConfig_; //  Indicates the user wants to download connection configuration information. 0 (or if absent): Indicates that no connection
	RsslUInt64 role_; // consumer or provider RDM_LOGIN_ROLE_CONS == 0,	RDM_LOGIN_ROLE_PROV == 1. Will always be consumer


	// encode the request

	RsslRet EncodeLoginRequest(RsslLoginRequestInfo* loginReqInfo, RsslBuffer* msgBuf);

	RsslRet EncodeLoginClose(RsslBuffer* msgBuf, RsslInt32 streamId);

	// response

	RsslLoginResponseInfo loginResponseInfo_;

	RsslLoginRequestInfo loginRequestInfo_;

	RsslRet DecodeLoginResponse(RsslLoginResponseInfo* loginRespInfo, RsslMsg* msg, RsslDecodeIterator* dIter);


	RsslRet EncodeLoginRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslLoginRejectReason reason, RsslBuffer* msgBuf);

	RsslRet EncodeLoginResponse(RsslChannel* chnl, RsslLoginResponseInfo* loginRespInfo, RsslBuffer* msgBuf);

	vector<LoginResponseListener*> listeners_;

	void NotifyListeners(RsslLoginResponseInfo *pResponseInfo, const std::string &extraInfo);

	RsslChannel* UPAChannel_;

	// state
	RsslBool isClosed_;
	RsslBool isClosedRecoverable_;
	RsslBool isSuspect_;
	RsslBool isNotEntitled_;

	RsslRet CloseLoginStream(RsslChannel* chnl);

	// provider login has to accept requests and send responses. Consumer is the other way around
	bool provider_;

	// controls whether login request specifies that the RMDS should disable automatic conversion of marketfeed to rssl
	// the default is false which means marketfeed messages get converted to rssl
	bool disableDataConversion_;

};


// abstract base class for responses
class LoginResponseListener
{
public:
	virtual void LoginResponse(UPALogin::RsslLoginResponseInfo * pResponseInfo, const std::string &extraInfo) = 0;
};

#endif //__UPALOGIN_H__