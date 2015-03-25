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
#include "UPALogin.h"
#include "UPAMessage.h"
#include "UPAConsumer.h"

#include <utils/t42log.h>
#include <utils/os.h>

#define RSSL_CONSUMER 0
#define RSSL_PROVIDER 1

// could make this configurable
const int LoginStreamId = 1;

// Cache the result of gethostname() to reduce overhead
std::string UPALogin::hostName_;

RTR_C_INLINE void clearLoginReqInfo(UPALogin::RsslLoginRequestInfo* loginReqInfo)
{
	loginReqInfo->StreamId = 0;
	loginReqInfo->Username[0] = '\0';
	loginReqInfo->ApplicationId[0] = '\0';
	loginReqInfo->ApplicationName[0] = '\0';
	loginReqInfo->Position[0] = '\0';
	loginReqInfo->Password[0] = '\0';
	loginReqInfo->ProvidePermissionProfile = 0;
	loginReqInfo->ProvidePermissionExpressions = 0;
	loginReqInfo->SingleOpen = 0;
	loginReqInfo->AllowSuspectData = 0;
	loginReqInfo->InstanceId[0] = '\0';
	loginReqInfo->Role = 0;
	loginReqInfo->DownloadConnectionConfig = 0;
	loginReqInfo->UPAChannel = 0;
	loginReqInfo->IsInUse = RSSL_FALSE;
}

RTR_C_INLINE void initLoginReqInfo(UPALogin::RsslLoginRequestInfo* loginReqInfo)
{
	clearLoginReqInfo(loginReqInfo);
	loginReqInfo->StreamId = 0;
	loginReqInfo->Username[0] = '\0';
	loginReqInfo->ApplicationId[0] = '\0';
	loginReqInfo->ApplicationName[0] = '\0';
	loginReqInfo->Position[0] = '\0';
	loginReqInfo->Password[0] = '\0';
	loginReqInfo->ProvidePermissionProfile = 1;
	loginReqInfo->ProvidePermissionExpressions = 1;
	loginReqInfo->SingleOpen = 1;
	loginReqInfo->AllowSuspectData = 1;
	loginReqInfo->InstanceId[0] = '\0';
	loginReqInfo->Role = 0;
	loginReqInfo->DownloadConnectionConfig = 0;
}

/*
 * Clears the login response information.
 * loginRespInfo - The login response information to be cleared
 */
RTR_C_INLINE void clearLoginRespInfo(UPALogin::RsslLoginResponseInfo* loginRespInfo)
{
	loginRespInfo->StreamId = 0;
	loginRespInfo->Username[0] = '\0';
	loginRespInfo->ApplicationId[0] = '\0';
	loginRespInfo->ApplicationName[0] = '\0';
	loginRespInfo->Position[0] = '\0';
	loginRespInfo->ProvidePermissionProfile = 0;
	loginRespInfo->ProvidePermissionExpressions = 0;
	loginRespInfo->SingleOpen = 0;
	loginRespInfo->AllowSuspectData = 0;
	loginRespInfo->SupportPauseResume = 0;
	loginRespInfo->SupportOptimizedPauseResume = 0;
	loginRespInfo->SupportOMMPost = 0;
	loginRespInfo->SupportViewRequests = 0;
	loginRespInfo->SupportBatchRequests = 0;
	loginRespInfo->SupportStandby = 0;
	loginRespInfo->isSolicited = RSSL_FALSE;
}

UPALogin::UPALogin(bool provider)
	: provider_(provider)
{
	// Set some defaults

	userName_ = "";
	appID_ = "256";
	password_ = "password";
	instanceId_= "instance1";

	isClosed_ = RSSL_FALSE;
	isClosedRecoverable_ = RSSL_FALSE;
	isSuspect_ = RSSL_TRUE;
	isNotEntitled_ = RSSL_FALSE;
	role_ = RDM_LOGIN_ROLE_CONS;

	clearLoginReqInfo(&loginRequestInfo_);

	UPAChannel_ = 0;
}


UPALogin::~UPALogin(void)
{
	if(!provider_)
	{
		CloseLoginStream(UPAChannel_);
	}
}

void UPALogin::ConfigureEntitlements(TransportConfig_t *pConfig)
{
    // Set up the DACS user name and application ID from the
    // configuration
    // We get the username and appid from mama but can override from the config file
    // If mama cant provide them then (and they are not overridden from properties,
    // we get the use name from the OS and set the appid to 255

    // first place to override is the mama.entitlement.altuserid, unless it is disabled
    const char *userName = 0;
    const char *useAltUserId = mama_getProperty("mama.entitlement.usealtuserid");
    if((useAltUserId == 0) || (mama_bool_t)properties_GetPropertyValueAsBoolean(useAltUserId))
    {
        // The configuration has not disabled the altuserid property
        userName = mama_getProperty("mama.entitlement.altuserid");
    }
    if(userName != 0)
    {
        userName_ = userName;
    }

    if(userName_.empty())
    {
        // now try the bridge config
        userName_ = pConfig->getString("user", Default_UserName);
        if(userName_.empty())
        {
            // now try from the mama infrastructure
            const char* mamaUserName = 0;
            mama_status stat = mama_getUserName(&mamaUserName);
            if((stat == MAMA_STATUS_OK) && (mamaUserName != 0))
            {
                userName_ = mamaUserName;
            }
            else
            {
                // finally just get it from the OS
                // get the current user name
                utils::os::getUserName(userName_);
            }
        }
    }


    // Give priority to an appid set by the mama layer
    const char* mamaAppid = 0;
    mama_status stat = mama_getApplicationName(&mamaAppid);
    // sanity check its a number - first digit is good enough
    if(stat == MAMA_STATUS_OK && mamaAppid != 0 &&  isdigit(mamaAppid[0]))
    {
        appID_ = mamaAppid;
    }
    else
    {
		// if thats not present then user appid from the config file
		appID_ = pConfig->getString("appid", Default_Appid);
		if(appID_.empty())
		{
			// and failing that...
			//the usual TR default
			appID_ = "255";
		}
    }
    
}

void UPALogin::AddListener( LoginResponseListener * pListener )
{
	listeners_.push_back(pListener);
}

bool UPALogin::SendLoginRequest()
{

	RsslRet ret;
	RsslError error;
	RsslBuffer* UPAMsgBuff = 0;

	RsslUInt32 ipAddress = 0;
	char hostName[256], positionStr[32];
	RsslBuffer  hostNameBuf;

	//initialize login request info to default values 
	RsslLoginRequestInfo loginReqInfo;
	initLoginReqInfo(&loginReqInfo);


	if(UPAChannel_ == 0)
	{
		mama_log(MAMA_LOG_LEVEL_WARN,"UPALogin::SendLoginRequest - No Channel");
		return false;
	}

	//get a buffer for the login request 
	UPAMsgBuff = rsslGetBuffer(UPAChannel_, MAX_MSG_SIZE, RSSL_FALSE, &error);

	if (UPAMsgBuff != NULL)
	{
		// populate the login request information
		 //StreamId 
		loginReqInfo.StreamId = LoginStreamId;

		 //Username 
		snprintf(loginReqInfo.Username, 128, "%s", userName_.c_str());

		 //ApplicationId 
		snprintf(loginReqInfo.ApplicationId, 128, "%s", appID_.c_str());

		 //ApplicationName 
		snprintf(loginReqInfo.ApplicationName, 128, "%s", appName_.c_str());

		 //Position build the DACS position string from the hostname
		if (gethostname(hostName, sizeof(hostName)) != 0)
		{
			sprintf(hostName, "localhost");
		}
		hostNameBuf.data = hostName;
		hostNameBuf.length = sizeof(hostName);
		if (rsslHostByName(&hostNameBuf, &ipAddress) == RSSL_RET_SUCCESS)
		{
			rsslIPAddrUIntToString(ipAddress, positionStr);
			strcat(positionStr, "/net");
			snprintf(loginReqInfo.Position, 128, "%s", positionStr);
		}
		else
		{
			sprintf(loginReqInfo.Position, "localhost");
		}

		// the password
		snprintf(loginReqInfo.Password, 128, "%s", password_.c_str());

		// InstanceId
		snprintf(loginReqInfo.InstanceId, 128, "%s", instanceId_.c_str());

		//if provider, change role and single open from default values//
		if (role_ == RSSL_PROVIDER)
		{
			 // if this provider supports SingleOpen behavior 
			loginReqInfo.SingleOpen = singleOpen_;

			 //provider role 
			loginReqInfo.Role = RSSL_PROVIDER; 
		}
		//keep default values for all others 

		//encode login request
		if ((ret = EncodeLoginRequest(&loginReqInfo, UPAMsgBuff)) != RSSL_RET_SUCCESS)
		{
			rsslReleaseBuffer(UPAMsgBuff, &error); 
			t42log_error("encodeLoginRequest() failed with return code: %d\n", ret);
			return false;
		}

		//send login request
		if (SendUPAMessage(UPAChannel_, UPAMsgBuff) != RSSL_RET_SUCCESS)
		{
			t42log_error("sendMessage failed with return code: %d\n", ret);
			return false;
		}

	}
	else
	{
		t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
		return false;
	}

	return true;

}

std::string ToString(const RsslState *pState)
{
   return std::string(pState->text.data);
}

const char *UPALogin::GetHostName(RsslUInt32 *length)
{
   if (hostName_.length() == 0)
   {
      char buffer[256];
      if (gethostname(&buffer[0], sizeof(buffer) / sizeof(buffer[0])) != 0)
      {
         hostName_ = "localhost";
      }
      else
      {
         hostName_ = &buffer[0];
      }
   }
   if (length != 0)
   {
      *length = static_cast<RsslUInt32>(hostName_.length());
   }
   return hostName_.c_str();
}

RsslRet UPALogin::processLoginResponse(RsslChannel* chnl, RsslMsg* msg, RsslDecodeIterator* dIter)
{
	RsslRet ret;
	RsslState *pState = 0;
	
	// temp buffer for recovering rssl state as string
	RsslBuffer buff;
	char tempData[1024];
	buff.data = tempData;
	buff.length = 1024;

	switch(msg->msgBase.msgClass)
	{
	case RSSL_MC_REFRESH:
		/* first clear response info structure */
		clearLoginRespInfo(&loginResponseInfo_);
		/* decode login response */
		if ((ret = DecodeLoginResponse(&loginResponseInfo_, msg, dIter)) != RSSL_RET_SUCCESS)
		{
			t42log_error("decodeLoginResponse() failed with return code: %d\n", ret);
			NotifyListeners(&loginResponseInfo_, buff.data);

			return ret;
		}

		t42log_info("Received Login Response for Username: %s\n", loginResponseInfo_.Username);

		/* get state information */
		pState = &msg->refreshMsg.state;
		rsslStateToString(&buff, pState);
		t42log_info("	%s\n\n", buff.data);

		/* call login success callback if login okay and is solicited */
		if (loginResponseInfo_.isSolicited && pState->streamState == RSSL_STREAM_OPEN && pState->dataState == RSSL_DATA_OK)
		{
			isClosed_ = RSSL_FALSE;
			isClosedRecoverable_ = RSSL_FALSE;
			isSuspect_ = RSSL_FALSE;
			NotifyListeners(&loginResponseInfo_, ToString(pState));
		}
		else /* handle error cases */
		{
			if (pState->streamState == RSSL_STREAM_CLOSED_RECOVER)
			{
				t42log_error("Login stream is closed recover\n");
				isClosedRecoverable_ = RSSL_TRUE;
				NotifyListeners(&loginResponseInfo_, ToString(pState));
				return RSSL_RET_FAILURE;
			}
			else if (pState->streamState == RSSL_STREAM_CLOSED)
			{
				const char * errMsg = "Login attempt failed (stream closed)";
				t42log_error("\n%s\n",errMsg);
				NotifyListeners(&loginResponseInfo_, errMsg);

				isClosed_ = RSSL_TRUE;
				return RSSL_RET_FAILURE;
			}
			else if (pState->streamState == RSSL_STREAM_OPEN && pState->dataState == RSSL_DATA_SUSPECT)
			{
				const char * errMsg = "Login stream is suspect";
				t42log_error("\n%s\n",errMsg);
				NotifyListeners(&loginResponseInfo_, errMsg);

				isSuspect_ = RSSL_TRUE;
				return RSSL_RET_FAILURE;
			}
		}
		break;

	case RSSL_MC_STATUS:
		t42log_info("Received Login StatusMsg\n");
		if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
		{
			/* get state information */
			pState = &msg->statusMsg.state;
			rsslStateToString(&buff, pState);
			t42log_info("	%s\n\n", buff.data);
			/* handle error cases */
			if (pState->streamState == RSSL_STREAM_CLOSED_RECOVER)
			{
				const char * errMsg = "Login stream is closed recover";
				t42log_error("\n%s\n",errMsg);
				NotifyListeners(&loginResponseInfo_, errMsg);

				isClosedRecoverable_ = RSSL_TRUE;
				return RSSL_RET_FAILURE;
			}
			else if (pState->streamState == RSSL_STREAM_CLOSED)
			{
				const char * errMsg = "Login stream is closed";
				t42log_error("\n%s\n",errMsg);
				NotifyListeners(&loginResponseInfo_, errMsg);

				if (pState->code == RSSL_SC_USER_UNKNOWN_TO_PERM_SYS)
				{
					isNotEntitled_ = RSSL_TRUE;

					t42log_warn("Login rejected by DACS for user %s from %s with appid %s\n", userName_.c_str(), appName_.c_str(), appID_.c_str());
				}
				isClosed_ = RSSL_TRUE;
				return RSSL_RET_FAILURE;
			}
			else if (pState->streamState == RSSL_STREAM_OPEN && pState->dataState == RSSL_DATA_SUSPECT)
			{
				const char * errMsg = "Login stream is suspect";
				t42log_error("\n%s\n",errMsg);
				NotifyListeners(&loginResponseInfo_, errMsg);

				isSuspect_ = RSSL_TRUE;
				return RSSL_RET_FAILURE;
			}
		}
		break;

	case RSSL_MC_UPDATE:
		{
			t42log_info("Received Login Update\n");
			break;
		}

	case RSSL_MC_CLOSE:
		{
			const char * errMsg =  "Received Login Close";
			t42log_error("\n%s\n",errMsg);
			NotifyListeners(&loginResponseInfo_, errMsg);
	
			isClosed_ = RSSL_TRUE;
			return RSSL_RET_FAILURE;
			break;
		}
	default:
		{
			char errMsgBuff[64];
			sprintf(errMsgBuff, "Received Unhandled Login Msg Class: %d", msg->msgBase.msgClass);
			t42log_error("\n%s\n",errMsgBuff);
			NotifyListeners(&loginResponseInfo_, errMsgBuff);
	
			return RSSL_RET_FAILURE;
			break;
		}
	}
	return RSSL_RET_SUCCESS;
}


bool UPALogin::QueueLogin(mamaQueue queue)
{

	mama_status status = mamaQueue_enqueueEvent(queue,  UPAConsumer::LoginRequestCb, (void*) this);
	if(status != MAMA_STATUS_OK)
	{
		t42log_error("Failed to enqueue login request status = %d", status);
		return false;
	}

	return true;
}

bool UPALogin::QueueLogin(mamaQueue queue, mamaQueueEventCB callback)
{
	mama_status status = mamaQueue_enqueueEvent(queue, callback, (void*) this);
	if(status != MAMA_STATUS_OK)
	{
		t42log_error("Failed to enqueue login request status = %d", status);
		return false;
	}

	return true;
}

void UPALogin::NotifyListeners( RsslLoginResponseInfo *pResponseInfo, const std::string &extraInfo)
{
	vector<LoginResponseListener*>::iterator it = listeners_.begin();

	while(it != listeners_.end() )
	{
		(*it)->LoginResponse(pResponseInfo, extraInfo);
		it++;
	}

}



/*
 * Close the login stream.  Note that closing login stream
 * will automatically close all other streams at the provider.
 * Returns success if close login stream succeeds or failure
 * if it fails.
 * chnl - The channel to send a login close to
 */
RsslRet UPALogin::CloseLoginStream(RsslChannel* chnl)
{
	RsslRet ret;
	RsslError error;
	RsslBuffer* msgBuf = 0;

	/* get a buffer for the login close */
	msgBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error);

	if (msgBuf != NULL)
	{
		/* encode login close */
		if ((ret = EncodeLoginClose(msgBuf, LoginStreamId)) != RSSL_RET_SUCCESS)
		{
			rsslReleaseBuffer(msgBuf, &error); 
			t42log_error("encodeLoginClose() failed with return code: %d\n", ret);
			return ret;
		}

		/* send close */
		if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
			return RSSL_RET_FAILURE;
	}
	else
	{
		t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
		return RSSL_RET_FAILURE;
	}

	return RSSL_RET_SUCCESS;
}

RsslRet UPALogin::EncodeLoginRequest(RsslLoginRequestInfo* loginReqInfo, RsslBuffer* msgBuf)
{
	RsslRet ret = 0;
	RsslRequestMsg msg = RSSL_INIT_REQUEST_MSG;
	RsslMsgKey key = RSSL_INIT_MSG_KEY;
	RsslElementEntry	element = RSSL_INIT_ELEMENT_ENTRY;
	RsslElementList	elementList = RSSL_INIT_ELEMENT_LIST;
	RsslBuffer applicationId, applicationName, position, password, instanceId;
	RsslEncodeIterator encodeIter;

	RsslUInt64 d;

	 //clear encode iterator 
	rsslClearEncodeIterator(&encodeIter);

	 //set-up message 
	msg.msgBase.msgClass = RSSL_MC_REQUEST;
	msg.msgBase.streamId = loginReqInfo->StreamId;
	msg.msgBase.domainType = RSSL_DMT_LOGIN;
	msg.msgBase.containerType = RSSL_DT_NO_DATA;
	msg.flags = RSSL_RQMF_STREAMING;


	 //set msgKey members 
	msg.msgBase.msgKey.flags = RSSL_MKF_HAS_ATTRIB | RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME;
	 //Username 
	msg.msgBase.msgKey.name.data = loginReqInfo->Username;
	msg.msgBase.msgKey.name.length = (RsslUInt32)strlen(loginReqInfo->Username);
	msg.msgBase.msgKey.nameType = RDM_LOGIN_USER_NAME;
	msg.msgBase.msgKey.attribContainerType = RSSL_DT_ELEMENT_LIST;


	 //encode message 
	if((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
		return ret;
	}
	rsslSetEncodeIteratorRWFVersion(&encodeIter, UPAChannel_->majorVersion, UPAChannel_->minorVersion);
	 //since our msgKey has opaque that we want to encode, we need to use rsslEncodeMsgInit 
	 //rsslEncodeMsgInit should return and inform us to encode our key opaque 
	if ((ret = rsslEncodeMsgInit(&encodeIter, (RsslMsg*)&msg, 0)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeMsgInit() failed with return code: %d\n", ret);
		return ret;
	}
	
	 //encode our msgKey opaque 
	 //encode the element list 
	rsslClearElementList(&elementList);
	elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
	if ((ret = rsslEncodeElementListInit(&encodeIter, &elementList, 0, 3)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementListInit() failed with return code: %d\n", ret);
		return ret;
	}
	 //ApplicationId 
	applicationId.data = (char*)loginReqInfo->ApplicationId;
	applicationId.length = (RsslUInt32)strlen(loginReqInfo->ApplicationId);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_APPID;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &applicationId)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //ApplicationName 
	applicationName.data = (char*)loginReqInfo->ApplicationName;
	applicationName.length = (RsslUInt32)strlen(loginReqInfo->ApplicationName);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_APPNAME;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &applicationName)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //Position 
	position.data = (char*)loginReqInfo->Position;
	position.length = (RsslUInt32)strlen(loginReqInfo->Position);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_POSITION;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &position)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //Password 
	password.data = (char*)loginReqInfo->Password;
	password.length = (RsslUInt32)strlen(loginReqInfo->Password);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_PASSWORD;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &password)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //ProvidePermissionProfile 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_PROV_PERM_PROF;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->ProvidePermissionProfile)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //ProvidePermissionExpressions 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_PROV_PERM_EXP;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->ProvidePermissionExpressions)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //SingleOpen 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SINGLE_OPEN;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->SingleOpen)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //AllowSuspectData 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_ALLOW_SUSPECT_DATA;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->AllowSuspectData)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //InstanceId 
	instanceId.data = (char*)loginReqInfo->InstanceId;
	instanceId.length = (RsslUInt32)strlen(loginReqInfo->InstanceId);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_INST_ID;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &instanceId)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //Role 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_ROLE;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->Role)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	 //DownloadConnectionConfig 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_DOWNLOAD_CON_CONFIG;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginReqInfo->DownloadConnectionConfig)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}


	// this is "undocumented" rmds behavior. If we add this element to the login, then for all subscriptions on this channel
	// the rmds will not convert market feed messages to rssl - it packs the marketfeed message in an rssl message element with an undocumented type.
	// Handling these messages requires the Tick42 Enhanced version of the bridge. Please contact support@tick42.com for details.
	if (disableDataConversion_)
	{
		 //DisableDataConversion 
		element.dataType = RSSL_DT_UINT;
		element.name.length =21;
		element.name.data = (char*)"DisableDataConversion";
		
		d = RSSL_TRUE;
		if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &d)) < RSSL_RET_SUCCESS)
		{
			t42log_error("rsslEncodeElementEntry() failed with return code: %d\n", ret);
			return ret;
		}
	}

	/* complete encode element list */
	if ((ret = rsslEncodeElementListComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeElementListComplete() failed with return code: %d\n", ret);
		return ret;
	}

	 //complete encode key 
	 //rsslEncodeMsgKeyAttribComplete finishes our key opaque, so it should return and indicate
	 //  for us to encode our container/msg payload 
	if ((ret = rsslEncodeMsgKeyAttribComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeMsgKeyAttribComplete() failed with return code: %d\n", ret);
		return ret;
	}

	 //complete encode message 
	if ((ret = rsslEncodeMsgComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeMsgComplete() failed with return code: %d\n", ret);
		return ret;
	}
	msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

	return RSSL_RET_SUCCESS;
}


RsslRet UPALogin::DecodeLoginResponse(RsslLoginResponseInfo* loginRespInfo, RsslMsg* msg, RsslDecodeIterator* dIter)
{
	RsslRet ret = 0;
	RsslMsgKey* key = 0;
	RsslElementList	elementList;
	RsslElementEntry	element;

	 //set stream id 
	loginRespInfo->StreamId = msg->msgBase.streamId;

	 //set isSolicited flag if a solicited refresh 
	if (msg->msgBase.msgClass == RSSL_MC_REFRESH)
	{
		if (msg->refreshMsg.flags & RSSL_RFMF_SOLICITED)
		{
			loginRespInfo->isSolicited = RSSL_TRUE;
		}
	}

	 //get key 
	key = (RsslMsgKey *)rsslGetMsgKey(msg);

	 //get Username 
	if (key)
	{
		if (key->name.length < MAX_LOGIN_INFO_STRLEN)
		{
			strncpy(loginRespInfo->Username, key->name.data, key->name.length);
			loginRespInfo->Username[key->name.length] = '\0';
		}
		else
		{
			strncpy(loginRespInfo->Username, key->name.data, MAX_LOGIN_INFO_STRLEN - 1);
			loginRespInfo->Username[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
		}
	}
	else 
	{
		loginRespInfo->Username[0] = '\0';
	}

	 //decode key opaque data 
	if ((ret = rsslDecodeMsgKeyAttrib(dIter, key)) != RSSL_RET_SUCCESS)
	{
		t42log_error("rsslDecodeMsgKeyAttrib() failed with return code: %d\n", ret);
		return ret;
	}

	 //decode element list 
	if ((ret = rsslDecodeElementList(dIter, &elementList, NULL)) == RSSL_RET_SUCCESS)
	{
		 //decode each element entry in list 
		while ((ret = rsslDecodeElementEntry(dIter, &element)) != RSSL_RET_END_OF_CONTAINER)
		{
			if (ret == RSSL_RET_SUCCESS)
			{
				 //get login response information 
				 //AllowSuspectData 
				if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_ALLOW_SUSPECT_DATA))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->AllowSuspectData);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //ApplicationId 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_APPID))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginRespInfo->ApplicationId, element.encData.data, element.encData.length);
						loginRespInfo->ApplicationId[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginRespInfo->ApplicationId, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginRespInfo->ApplicationId[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				 //ApplicationName 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_APPNAME))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginRespInfo->ApplicationName, element.encData.data, element.encData.length);
						loginRespInfo->ApplicationName[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginRespInfo->ApplicationName, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginRespInfo->ApplicationName[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				 //Position 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_POSITION))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginRespInfo->Position, element.encData.data, element.encData.length);
						loginRespInfo->Position[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginRespInfo->Position, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginRespInfo->Position[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				 //ProvidePermissionExpressions 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_PROV_PERM_EXP))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->ProvidePermissionExpressions);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //ProvidePermissionProfile 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_PROV_PERM_PROF))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->ProvidePermissionProfile);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SingleOpen 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SINGLE_OPEN))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SingleOpen);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportOMMPost 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_POST))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportOMMPost);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportPauseResume 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_PR))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportPauseResume);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportStandby 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_STANDBY))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportStandby);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportBatchRequests 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_BATCH))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportBatchRequests);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportViewRequests 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_VIEW))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportViewRequests);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
				 //SupportOptimizedPauseResume 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SUPPORT_OPR))
				{
					ret = rsslDecodeUInt(dIter, &loginRespInfo->SupportOptimizedPauseResume);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
						return ret;
					}
				}
			}
			else
			{
				t42log_error("rsslDecodeElementEntry() failed with return code: %d\n", ret);
				return ret;
			}
		}
	}
	else
	{
		t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
		return ret;
	}

	return RSSL_RET_SUCCESS;
}


RsslRet UPALogin::EncodeLoginClose(RsslBuffer* msgBuf, RsslInt32 streamId)
{
	RsslRet ret = 0;
	RsslCloseMsg msg = RSSL_INIT_CLOSE_MSG;
	RsslEncodeIterator encodeIter;

	 //clear encode iterator 
	rsslClearEncodeIterator(&encodeIter);

	 //set-up message 
	msg.msgBase.msgClass = RSSL_MC_CLOSE;
	msg.msgBase.streamId = streamId;
	msg.msgBase.domainType = RSSL_DMT_LOGIN;
	msg.msgBase.containerType = RSSL_DT_NO_DATA;
	
	 //encode message 
	if((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
		return ret;
	}
	rsslSetEncodeIteratorRWFVersion(&encodeIter, UPAChannel_->majorVersion, UPAChannel_->minorVersion);
	if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&msg)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslEncodeMsg() failed with return code: %d\n", ret);
		return ret;
	}

	msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

	return RSSL_RET_SUCCESS;
}

RsslUInt32 UPALogin::StreamId() const
{
	return LoginStreamId;
}


RsslRet UPALogin::SendLoginRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslLoginRejectReason reason)
{
	RsslError error;
	RsslBuffer* msgBuf = 0;

	// create a buffer to encode the message
	msgBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error);

	if (msgBuf != NULL)
	{
		 //encode the rejection with the reason
		if (EncodeLoginRequestReject(chnl, streamId, reason, msgBuf) != RSSL_RET_SUCCESS)
		{
			rsslReleaseBuffer(msgBuf, &error);
			t42log_warn("UPALogin::SendLoginRequestReject - EncodeLoginRequestReject() failed\n"); 
			return RSSL_RET_FAILURE;
		}

		// and send the message
		if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
			return RSSL_RET_FAILURE;
	}
	else
	{
		t42log_warn("rsslGetBuffer(): Failed <%s>\n", error.text);
		return RSSL_RET_FAILURE;
	}

	return RSSL_RET_SUCCESS;
}


RsslRet UPALogin::EncodeLoginRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslLoginRejectReason reason, RsslBuffer* msgBuf)
{


	char stateText[MAX_LOGIN_INFO_STRLEN];
	RsslEncodeIterator encodeIter;

	 // initialise an iterator
	rsslClearEncodeIterator(&encodeIter);

	// initialise the message header
	RsslStatusMsg msg = RSSL_INIT_STATUS_MSG;	
	msg.msgBase.msgClass = RSSL_MC_STATUS;
	msg.msgBase.streamId = streamId;
	msg.msgBase.domainType = RSSL_DMT_LOGIN;
	msg.msgBase.containerType = RSSL_DT_NO_DATA;
	msg.flags = RSSL_STMF_HAS_STATE;
	msg.state.streamState = RSSL_STREAM_CLOSED_RECOVER;
	msg.state.dataState = RSSL_DATA_SUSPECT;
	switch(reason)
	{
	case MaxLoginRequestsReached:
		msg.state.code = RSSL_SC_TOO_MANY_ITEMS;
		sprintf(stateText, "Login request rejected for stream id %d - max request count reached", streamId);
		msg.state.text.data = stateText;
		msg.state.text.length = (RsslUInt32)strlen(stateText) + 1;
		break;
	case NoUserName:
		msg.state.code = RSSL_SC_USAGE_ERROR;
		sprintf(stateText, "Login request rejected for stream id %d - request does not contain user name", streamId);
		msg.state.text.data = stateText;
		msg.state.text.length = (RsslUInt32)strlen(stateText) + 1;
		break;
	default:
		break;
	}

	// encode the message into the buffer
	RsslRet ret = 0;	
	if((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
	{
		t42log_warn(" UPALogin::EncodeLoginRequestReject - rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
		return ret;
	}
	rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
	if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&msg)) < RSSL_RET_SUCCESS)
	{
		t42log_warn(" UPALogin::EncodeLoginRequestReject - rsslEncodeMsg() failed with return code: %d\n", ret);
		return ret;
	}

	msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

	return RSSL_RET_SUCCESS;
}

RsslRet UPALogin::DecodeLoginRequest( RsslLoginRequestInfo* loginReqInfo, RsslMsg* msg, RsslDecodeIterator* dIter, RsslMsgKey* requestKey )
{

	RsslRet ret = 0;
	RsslElementList	elementList;
	RsslElementEntry	element;

	 // streamID
	loginReqInfo->StreamId = msg->requestMsg.msgBase.streamId;

	// username
	if (requestKey->name.length < MAX_LOGIN_INFO_STRLEN)
	{
		strncpy(loginReqInfo->Username, requestKey->name.data, requestKey->name.length);
		loginReqInfo->Username[requestKey->name.length] = '\0';
	}
	else
	{
		strncpy(loginReqInfo->Username, requestKey->name.data, MAX_LOGIN_INFO_STRLEN - 1);
		loginReqInfo->Username[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
	}

	 // decode (but ignore) key attribute data
	if ((ret = rsslDecodeMsgKeyAttrib(dIter, requestKey)) < RSSL_RET_SUCCESS)
	{
		t42log_info("rsslDecodeMsgKeyAttrib() failed with return code: %d\n", ret);
		return ret;
	}

	 //decode the element list - this is a randomly popluated set if fields inthe login request
	if ((ret = rsslDecodeElementList(dIter, &elementList, NULL)) == RSSL_RET_SUCCESS)
	{
		// walk the element list
		while ((ret = rsslDecodeElementEntry(dIter, &element)) != RSSL_RET_END_OF_CONTAINER)
		{
			// in each case truncate the string if it is too long
			if (ret == RSSL_RET_SUCCESS)
			{
				// Appid
				if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_APPID))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginReqInfo->ApplicationId, element.encData.data, element.encData.length);
						loginReqInfo->ApplicationId[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginReqInfo->ApplicationId, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginReqInfo->ApplicationId[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				// App name
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_APPNAME))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginReqInfo->ApplicationName, element.encData.data, element.encData.length);
						loginReqInfo->ApplicationName[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginReqInfo->ApplicationName, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginReqInfo->ApplicationName[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				// Position
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_POSITION))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginReqInfo->Position, element.encData.data, element.encData.length);
						loginReqInfo->Position[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginReqInfo->Position, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginReqInfo->Position[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				// password
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_PASSWORD))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginReqInfo->Password, element.encData.data, element.encData.length);
						loginReqInfo->Password[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginReqInfo->Password, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginReqInfo->Password[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				// ProvidePermissionProfile 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_PROV_PERM_PROF))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->ProvidePermissionProfile);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() for ProvidePermissionProfile  failed with return code: %d\n", ret);
						return ret;
					}
				}
				// ProvidePermissionExpressions 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_PROV_PERM_EXP))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->ProvidePermissionExpressions);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() for ProvidePermissionExpressions failed with return code: %d\n", ret);
						return ret;
					}
				}
				// SingleOpen 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_SINGLE_OPEN))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->SingleOpen);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() failed for SingleOpen  with return code: %d\n", ret);
						return ret;
					}
				}
				 //AllowSuspectData 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_ALLOW_SUSPECT_DATA))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->AllowSuspectData);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() failed for AllowSuspectData with return code: %d\n", ret);
						return ret;
					}
				}
				// InstanceId 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_INST_ID))
				{
					if (element.encData.length < MAX_LOGIN_INFO_STRLEN)
					{
						strncpy(loginReqInfo->InstanceId, element.encData.data, element.encData.length);
						loginReqInfo->InstanceId[element.encData.length] = '\0';
					}
					else
					{
						strncpy(loginReqInfo->InstanceId, element.encData.data, MAX_LOGIN_INFO_STRLEN - 1);
						loginReqInfo->InstanceId[MAX_LOGIN_INFO_STRLEN - 1] = '\0';
					}
				}
				// Role 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_ROLE))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->Role);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() failed for Role with return code: %d\n", ret);
						return ret;
					}
				}
				// DownloadConnectionConfig 
				else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_DOWNLOAD_CON_CONFIG))
				{
					ret = rsslDecodeUInt(dIter, &loginReqInfo->DownloadConnectionConfig);
					if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
					{
						t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeUInt() failed for DownloadConnectionConfig with return code: %d\n", ret);
						return ret;
					}
				}
			}
			else
			{
				t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeElementEntry() failed with return code: %d\n", ret);
				return ret;
			}
		}
	}
	else
	{
		t42log_warn("UPALogin::DecodeLoginRequest - rsslDecodeElementList() failed with return code: %d\n", ret);
		return ret;
	}

	return RSSL_RET_SUCCESS;
}

RsslRet UPALogin::SendLoginResponse( RsslChannel* chnl, RsslLoginResponseInfo * loginRespInfo )
{
	RsslError error;
	RsslBuffer* msgBuf = 0;

	 //get a buffer for the login response 
	msgBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error);

	if (msgBuf != NULL)
	{
		// encode the login response message
		if (EncodeLoginResponse(chnl, loginRespInfo, msgBuf) != RSSL_RET_SUCCESS)
		{
			rsslReleaseBuffer(msgBuf, &error); 
			t42log_warn("UPALogin::sendLoginResponse - encodeLoginResponse() failed\n");
			return RSSL_RET_FAILURE;
		}

		 //send login response 
		if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
			return RSSL_RET_FAILURE;
	}
	else
	{
		t42log_warn("UPALogin::sendLoginResponse - rsslGetBuffer(): Failed <%s>\n", error.text);
		return RSSL_RET_FAILURE;
	}

	return RSSL_RET_SUCCESS;
}

RsslRet UPALogin::EncodeLoginResponse( RsslChannel* chnl, RsslLoginResponseInfo* loginRespInfo, RsslBuffer* msgBuf )
{
	// set up an encode iterator
	RsslEncodeIterator encodeIter;
	rsslClearEncodeIterator(&encodeIter);

	// set-up message & temp strings for message text fields
	char hostName[256];
	char stateText[MAX_LOGIN_INFO_STRLEN];
	RsslRefreshMsg msg = RSSL_INIT_REFRESH_MSG;
	msg.msgBase.msgClass = RSSL_MC_REFRESH;
	msg.msgBase.domainType = RSSL_DMT_LOGIN;
	msg.msgBase.containerType = RSSL_DT_NO_DATA;
	msg.flags = RSSL_RFMF_HAS_MSG_KEY | RSSL_RFMF_SOLICITED | RSSL_RFMF_REFRESH_COMPLETE | RSSL_RFMF_CLEAR_CACHE;
	msg.state.streamState = RSSL_STREAM_OPEN;
	msg.state.dataState = RSSL_DATA_OK;
	msg.state.code = RSSL_SC_NONE;
	sprintf(stateText, "Login accepted by host ");
	if (gethostname(hostName, sizeof(hostName)) != 0)
	{
		sprintf(hostName, "localhost");
	}
	strcat(stateText, hostName);
	msg.state.text.data = stateText;
	msg.state.text.length = (RsslUInt32)strlen(stateText);

	// StreamId 
	msg.msgBase.streamId = loginRespInfo->StreamId;

	// set msgKey members 
	msg.msgBase.msgKey.flags = RSSL_MKF_HAS_ATTRIB | RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME;
	// Username 
	msg.msgBase.msgKey.name.data = loginRespInfo->Username;
	msg.msgBase.msgKey.name.length = (RsslUInt32)strlen(loginRespInfo->Username);
	msg.msgBase.msgKey.nameType = RDM_LOGIN_USER_NAME;
	msg.msgBase.msgKey.attribContainerType = RSSL_DT_ELEMENT_LIST;

	// Mow can encode the message
	RsslRet ret = 0;
	if((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
		return ret;
	}
	rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
	// since our msgKey has opaque that we want to encode, we need to use rsslEncodeMsgInit 
	// rsslEncodeMsgInit should return and inform us to encode our key opaque 
	if ((ret = rsslEncodeMsgInit(&encodeIter, (RsslMsg*)&msg, 0)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeMsgInit() failed with return code: %d\n", ret);
		return ret;
	}
	
	// encode our msgKey opaque 
	// encode the element list 
	RsslElementList	elementList = RSSL_INIT_ELEMENT_LIST;
	RsslElementEntry	element = RSSL_INIT_ELEMENT_ENTRY;
	elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
	if ((ret = rsslEncodeElementListInit(&encodeIter, &elementList, 0, 0)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementListInit() failed with return code: %d\n", ret);
		return ret;
	}

	// ApplicationId 
	RsslBuffer applicationId;
	applicationId.data = (char*)loginRespInfo->ApplicationId;
	applicationId.length = (RsslUInt32)strlen(loginRespInfo->ApplicationId);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_APPID;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &applicationId)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// ApplicationName 
	RsslBuffer applicationName;
	applicationName.data = (char*)loginRespInfo->ApplicationName;
	applicationName.length = (RsslUInt32)strlen(loginRespInfo->ApplicationName);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_APPNAME;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &applicationName)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// Position 
	RsslBuffer position;
	position.data = (char*)loginRespInfo->Position;
	position.length = (RsslUInt32)strlen(loginRespInfo->Position);
	element.dataType = RSSL_DT_ASCII_STRING;
	element.name = RSSL_ENAME_POSITION;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &position)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// ProvidePermissionProfile 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_PROV_PERM_PROF;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->ProvidePermissionProfile)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// ProvidePermissionExpressions 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_PROV_PERM_EXP;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->ProvidePermissionExpressions)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SingleOpen 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SINGLE_OPEN;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SingleOpen)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// AllowSuspectData 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_ALLOW_SUSPECT_DATA;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->AllowSuspectData)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportPauseResume 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_PR;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportPauseResume)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportOptimizedPauseResume 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_OPR;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportOptimizedPauseResume)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportOMMPost 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_POST;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportOMMPost)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportViewRequests 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_VIEW;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportViewRequests)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportBatchRequests 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_BATCH;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportBatchRequests)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}
	// SupportStandby 
	element.dataType = RSSL_DT_UINT;
	element.name = RSSL_ENAME_SUPPORT_STANDBY;
	if ((ret = rsslEncodeElementEntry(&encodeIter, &element, &loginRespInfo->SupportStandby)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementEntry() failed with return code: %d\n", ret);
		return ret;
	}

	// complete encode element list 
	if ((ret = rsslEncodeElementListComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeElementListComplete() failed with return code: %d\n", ret);
		return ret;
	}

	// complete encode key 
	// rsslEncodeMsgKeyAttribComplete finishes our key opaque, so it should return and indicate
	//   for us to encode our container/msg payload 
	if ((ret = rsslEncodeMsgKeyAttribComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeMsgKeyAttribComplete() failed with return code: %d\n", ret);
		return ret;
	}

	// complete encoded message 
	if ((ret = rsslEncodeMsgComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
	{
		t42log_warn("UPALogin::EncodeLoginResponse - rsslEncodeMsgComplete() failed with return code: %d\n", ret);
		return ret;
	}
	msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);

	return RSSL_RET_SUCCESS;

}


