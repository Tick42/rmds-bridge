/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2014 Tick42 Ltd.
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
#include "UPANIProvider.h"
#include "RMDSNIPublisher.h"

#include <utils/os.h>
#include <utils/time.h>
#include <utils/t42log.h>

UPANIProvider::UPANIProvider(RMDSNIPublisher * owner)
	: shouldRecoverConnection_(RSSL_TRUE)
	, rsslNIProviderChannel_(NULL)
	, receivedServerMsg_ (RSSL_FALSE),
	owner_(owner),
	connectionConfig_(owner->Config()->getString("pubhosts"), owner->Config()->getString("retrysched", Default_retrysched))
{

	isInLoginSuspectState_ = RSSL_FALSE;
	if (owner_->InterfaceName())
	{
		interfaceName_ = ::strdup(owner_->InterfaceName());
	}
	else
	{
		interfaceName_ = NULL;
	}

	connType_ = owner_->ConnType();

	requestQueue_ = owner_->RequestQueue();

	login_ = new UPALogin(false);

	TransportConfig_t config(owner_->GetTransportName());
    login_->ConfigureEntitlements(&config);

	// initialise the login
	login_->AddListener(owner_);

	runThread_ = true;
}


UPANIProvider::~UPANIProvider(void)
{
}

bool UPANIProvider::IsConnectionConfigValid()
{
	return connectionConfig_.Valid();
}

// the thread function is similar to the consumer as it does a mount to the rmds
//
//however the state model is different
void UPANIProvider::Run()
{
	RsslError error;
	struct timeval time_interval;
	RsslRet	retval = 0;

	fd_set useRead;
	fd_set useExcept;
	fd_set useWrt;

	// todo might want to make these configurable
#ifdef _WIN32
	int rcvBfrSize = 65535;
	int sendBfrSize = 65535;
#endif

	int selRet;

	FD_ZERO(&readfds_);
	FD_ZERO(&exceptfds_);
	FD_ZERO(&wrtfds_);

	while(runThread_)
	{
		// attempt to make connection
		while(shouldRecoverConnection_)
		{
			login_->UPAChannel(0);

			// connect to server
			t42log_info("Attempting to connect to server %s:%s...\n", connectionConfig_.Host().c_str(), connectionConfig_.Port().c_str());

			if ((rsslNIProviderChannel_ = ConnectToRsslServer(connectionConfig_.Host(), connectionConfig_.Port(), interfaceName_, connType_, &error)) == NULL)
			{
				t42log_error("Unable to connect to RSSL server: <%s>\n",error.text);
			}
			else
			{
				if (rsslNIProviderChannel_)
				{
					// connected - set up socket fds
					RsslSocket s = rsslNIProviderChannel_->socketId;
					FD_SET(rsslNIProviderChannel_->socketId, &readfds_);
					FD_SET(rsslNIProviderChannel_->socketId, &wrtfds_);
					FD_SET(rsslNIProviderChannel_->socketId, &exceptfds_);
				}
			}

			if (rsslNIProviderChannel_ != NULL && rsslNIProviderChannel_->state == RSSL_CH_STATE_ACTIVE)
				shouldRecoverConnection_ = RSSL_FALSE;

			//Wait for channel to become active. 
			while (rsslNIProviderChannel_ != NULL && rsslNIProviderChannel_->state != RSSL_CH_STATE_ACTIVE && runThread_)
			{
				useRead = readfds_;
				useWrt = wrtfds_;
				useExcept = exceptfds_;

				/* Set a timeout value if the provider accepts the connection, but does not initialize it */
				time_interval.tv_sec = 60;
				time_interval.tv_usec = 0;

				selRet = select(FD_SETSIZE, &useRead, &useWrt, &useExcept, &time_interval);

				// select has timed out, close the channel and attempt to reconnect 
				if (selRet == 0)
				{
					t42log_warn("Channel initialization has timed out, attempting to reconnect...\n");

					if (rsslNIProviderChannel_)
					{
						FD_CLR(rsslNIProviderChannel_->socketId, &readfds_);
						FD_CLR(rsslNIProviderChannel_->socketId, &exceptfds_);
						if (FD_ISSET(rsslNIProviderChannel_->socketId, &wrtfds_))
							FD_CLR(rsslNIProviderChannel_->socketId, &wrtfds_);
					}
					RecoverConnection();
				}
				else
					// Received a response from the provider. 
					if (rsslNIProviderChannel_ && selRet > 0 && (FD_ISSET(rsslNIProviderChannel_->socketId, &useRead) || FD_ISSET(rsslNIProviderChannel_->socketId, &useWrt) || FD_ISSET(rsslNIProviderChannel_->socketId, &useExcept)))
					{
						if (rsslNIProviderChannel_->state == RSSL_CH_STATE_INITIALIZING)
						{
							RsslInProgInfo inProg = RSSL_INIT_IN_PROG_INFO;
							FD_CLR(rsslNIProviderChannel_->socketId,&wrtfds_);
							if ((retval = rsslInitChannel(rsslNIProviderChannel_, &inProg, &error)) < RSSL_RET_SUCCESS)
							{
								// channel init failed, try again
								t42log_warn("channelInactive fd=%d <%s>\n",
									rsslNIProviderChannel_->socketId,error.text);
								RecoverConnection();
								break; 
							}
							else 
							{
								switch ((int)retval)
								{
								case RSSL_RET_CHAN_INIT_IN_PROGRESS:
									if (inProg.flags & RSSL_IP_FD_CHANGE)
									{
										t42log_info("Channel In Progress - New FD: %d  Old FD: %d\n",rsslNIProviderChannel_->socketId, inProg.oldSocket );

										FD_CLR(inProg.oldSocket,&readfds_);
										FD_CLR(inProg.oldSocket,&exceptfds_);
										FD_SET(rsslNIProviderChannel_->socketId,&readfds_);
										FD_SET(rsslNIProviderChannel_->socketId,&exceptfds_);
										FD_SET(rsslNIProviderChannel_->socketId,&wrtfds_);
									}
									else
									{
										t42log_info("Channel %d In Progress...\n", rsslNIProviderChannel_->socketId);
									}
									break;
								case RSSL_RET_SUCCESS:
									{
										// connected
										t42log_info("Channel %d Is Active\n" ,rsslNIProviderChannel_->socketId);
										shouldRecoverConnection_ = RSSL_FALSE;
										RsslChannelInfo chanInfo;

										// log channel info
										if ((retval = rsslGetChannelInfo(rsslNIProviderChannel_, &chanInfo, &error)) >= RSSL_RET_SUCCESS)
										{
											RsslUInt32 i;
											for (i = 0; i < chanInfo.componentInfoCount; i++)
											{
												t42log_info("Connected to %s device.\n", chanInfo.componentInfo[i]->componentVersion.data);
											}
										}

										login_->UPAChannel(rsslNIProviderChannel_);
										mama_log (MAMA_LOG_LEVEL_FINEST, "Provider returned \"Connection Successful\"");
										NotifyListeners(true, "Provider returned \"Connection Successful\"");

									}
									break;
								default:
									{
										// Connection has failed
										char buff[256]={0};
										memset(buff,0,sizeof(buff));
										sprintf(buff, "Bad return value on connection  fd=%d <%s>\n", rsslNIProviderChannel_->socketId,error.text);
										t42log_error("%s\n", buff);
										NotifyListeners(false, buff);
										ExitThread();
									}
									break;
								}
							}
						}
					}
					else
						if (selRet < 0)
						{
							t42log_error("Select error.\n");
							ExitThread();
						}

			}

			// wait a while before retry
			if (shouldRecoverConnection_)
			{
				LogReconnection();
				WaitReconnectionDelay();

			}
		}

		//  WINDOWS: change size of send/receive buffer since it's small by default 
#ifdef _WIN32
		if (rsslNIProviderChannel_ && rsslIoctl(rsslNIProviderChannel_, RSSL_SYSTEM_WRITE_BUFFERS, &sendBfrSize, &error) != RSSL_RET_SUCCESS)
		{
			t42log_error("rsslIoctl(): failed <%s>\n", error.text);
		}
		if (rsslNIProviderChannel_ && rsslIoctl(rsslNIProviderChannel_, RSSL_SYSTEM_READ_BUFFERS, &rcvBfrSize, &error) != RSSL_RET_SUCCESS)
		{
			t42log_error("rsslIoctl(): failed <%s>\n", error.text);
		}
#endif

		 //Initialize ping handler 
		if (rsslNIProviderChannel_) 
			InitPingHandler(rsslNIProviderChannel_);

		int64_t queueCount = 0;
		 //this is the message processing loop 
		while(runThread_)
		{
			// first dispatch some events off the event queue
			PumpQueueEvents();

			useRead = readfds_;
			useExcept = exceptfds_;
			useWrt = wrtfds_;
			time_interval.tv_sec = 1;
			time_interval.tv_usec = 0;

			// look at the socket state
			selRet = select(FD_SETSIZE,&useRead,
				&useWrt,&useExcept,&time_interval);


			if (selRet < 0) // no messages received, continue 
			{
#ifdef _WIN32
				if (WSAGetLastError() == WSAEINTR)
					continue;
#else
				if (errno == EINTR)
				{
					continue;
				}
#endif
			}
			else if (selRet > 0) // messages received
			{
				if ((rsslNIProviderChannel_ != NULL) && (rsslNIProviderChannel_->socketId != -1))
				{
					if ((FD_ISSET(rsslNIProviderChannel_->socketId, &useRead)) ||
						(FD_ISSET(rsslNIProviderChannel_->socketId, &useExcept)))
					{
						if (ReadFromChannel(rsslNIProviderChannel_) != RSSL_RET_SUCCESS)
						{
							// the read failed so attempt to recover
							RecoverConnection();
						}
					}

					// If theres anything to be written flush the write socket
					if (rsslNIProviderChannel_ != NULL &&
						FD_ISSET(rsslNIProviderChannel_->socketId, &useWrt) &&
						rsslNIProviderChannel_->state == RSSL_CH_STATE_ACTIVE)
					{
						if ((retval = rsslFlush(rsslNIProviderChannel_, &error)) < RSSL_RET_SUCCESS)
						{
							t42log_error("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
						}
						else if (retval == RSSL_RET_SUCCESS)
						{
							// and clear the fd
							FD_CLR(rsslNIProviderChannel_->socketId, &wrtfds_);
						}
					}
				}
			}

			// break out of message processing loop if should recover connection 
			if (shouldRecoverConnection_ == RSSL_TRUE)
			{
				LogReconnection();
				WaitReconnectionDelay();
				break;
			}

			// check if its time to process pings
			if (rsslNIProviderChannel_)
			{
				ProcessPings(rsslNIProviderChannel_);
			}


		}
	}



}

RsslChannel* UPANIProvider::ConnectToRsslServer( const std::string &hostname, const std::string &port, char* interfaceName, RsslConnectionTypes connType, RsslError* error )
{
	// todo

	// add implementation of relaible multicast options
	RsslChannel* chnl;
	RsslConnectOptions connectOpts;// = RSSL_INIT_CONNECT_OPTS;		
	//manual initialization for connectOpts based on RSSL_INIT_CONNECT_OPTS - only values that are different than zero
	memset(&connectOpts,0,sizeof(connectOpts));
	connectOpts.pingTimeout=60;
	connectOpts.guaranteedOutputBuffers=50;
	connectOpts.numInputBuffers=10;
	connectOpts.multicastOpts.packetTTL=5;


	connectOpts.guaranteedOutputBuffers = 500;

	connectOpts.connectionInfo.unified.address = const_cast<char *>(hostname.c_str());
	connectOpts.connectionInfo.unified.serviceName = const_cast<char *>(port.c_str());
	connectOpts.connectionInfo.unified.interfaceName = interfaceName;
	connectOpts.connectionType = connType;
	connectOpts.majorVersion = RSSL_RWF_MAJOR_VERSION;
	connectOpts.minorVersion = RSSL_RWF_MINOR_VERSION;
	connectOpts.protocolType = RSSL_RWF_PROTOCOL_TYPE;

	if ( (chnl = rsslConnect(&connectOpts,error)) != 0)
	{
		FD_SET(chnl->socketId,&readfds_);
		FD_SET(chnl->socketId,&exceptfds_);

		t42log_info("Channel IPC descriptor = %d\n", chnl->socketId);
		if (!connectOpts.blocking)
		{	
			if (!FD_ISSET(chnl->socketId,&wrtfds_))
				FD_SET(chnl->socketId,&wrtfds_);
		}
	}

	return chnl;

}

void UPANIProvider::WaitReconnectionDelay()
{
	// wait for the specified time int the reconnection delay sequence.
	// but check every second to see if the thread has been shutdown
	for(unsigned  int i = 0; i < connectionConfig_.Delay(); ++i)
	{
		if (!runThread_)
			return;
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	connectionConfig_.Next();
}

void UPANIProvider::LogReconnection()
{
	t42log_warn("Connection failed will retry in %d second(s)\n", connectionConfig_.Delay());

}

void UPANIProvider::PumpQueueEvents()
{
	// dispatch an event off the queue
	size_t numEvents = 0;

	size_t maxDispatchesPerCycle = 50;
	size_t maxPendingOpens = 500;
	mama_status status = mamaQueue_getEventCount(requestQueue_, &numEvents);
	if ((status == MAMA_STATUS_OK) && (numEvents > 0))
	{

		// although we only really want to throttle subscriptions, for simplicity throttle everything here.  
		size_t eventsDispatched = 0;

		// unlike the consumer there is no max pending opens throttle, so just throttle on max events per cycle
		while (numEvents > eventsDispatched && eventsDispatched < maxDispatchesPerCycle )
		{
			mamaQueue_dispatchEvent(requestQueue_);	
			++eventsDispatched;
		}

		t42log_info("dispatched %d requests \n", eventsDispatched);
	}
}

void UPANIProvider::InitPingHandler( RsslChannel* chnl )
{
	time_t currentTime = 0;

	// get the current time 
	time(&currentTime);

	// set the ping timeout for client and server 
	if ((chnl != NULL) && (chnl->socketId != -1))
	{
		pingTimeoutClient_ = chnl->pingTimeout/3;
		pingTimeoutServer_ = chnl->pingTimeout;
	}
	else
	{
		pingTimeoutClient_ = 20;
		pingTimeoutServer_ = 60;
	}

	// set time to send next ping from client 
	nextSendPingTime_ = currentTime + (time_t)pingTimeoutClient_;

	// set time client should receive next message/ping from server 
	nextReceivePingTime_ = currentTime + (time_t)pingTimeoutServer_;
}

void UPANIProvider::RecoverConnection()
{
	// notify listeners that we are not connected
	mama_log (MAMA_LOG_LEVEL_FINE, "Recovering Connection");
	NotifyListeners(false, "Recovering Connection");

	// reset the channel
	if ((rsslNIProviderChannel_ != NULL) && (rsslNIProviderChannel_->socketId != -1))
	{
		RemoveChannel(rsslNIProviderChannel_);
		rsslNIProviderChannel_ = NULL;
	}
	// and flag a reconnect
	shouldRecoverConnection_ = RSSL_TRUE;
}

void UPANIProvider::RemoveChannel( RsslChannel* chnl )
{
	RsslError error;
	RsslRet ret;

	// tidy up the fds and close the channel
	FD_CLR(chnl->socketId, &readfds_);
	FD_CLR(chnl->socketId, &exceptfds_);
	if (FD_ISSET(chnl->socketId, &wrtfds_))
	{
		FD_CLR(chnl->socketId, &wrtfds_);
	}

	if ((ret = rsslCloseChannel(chnl, &error)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslCloseChannel() failed with return code: %d\n", ret);
	}
}

void UPANIProvider::NotifyListeners( bool connected, std::string extraInfo )
{
	vector<ConnectionListener*>::iterator it = listeners_.begin();

	while(it != listeners_.end() )
	{
		(*it)->ConnectionNotification(connected, extraInfo);
		it++;
	}
}

void UPANIProvider::AddListener( ConnectionListener * pListener )
{
	listeners_.push_back(pListener);
}

RsslRet UPANIProvider::ReadFromChannel( RsslChannel* chnl )
{
	RsslError		error;
	RsslBuffer *msgBuf=0;
	RsslRet	readret;

	if (chnl->socketId != -1 && chnl->state == RSSL_CH_STATE_ACTIVE)
	{
		readret = 1;
		while (readret > 0) // keep reading till notthing left
		{
			if ((msgBuf = rsslRead(chnl,&readret,&error)) != 0)
			{
				if (ProcessResponse(chnl, msgBuf) == RSSL_RET_SUCCESS)	
				{
					/* set flag for server message received */
					receivedServerMsg_ = RSSL_TRUE;
				}
				else
				{
					// this will trigger a reconnection so process response needs to be circumspect about return value
					return RSSL_RET_FAILURE;
				}
			}
			else
			{
				switch (readret)
				{
				case RSSL_RET_CONGESTION_DETECTED:
				case RSSL_RET_SLOW_READER:
				case RSSL_RET_PACKET_GAP_DETECTED:
					{
						if (chnl->state != RSSL_CH_STATE_CLOSED)
						{
							// disconnectOnGaps must be false.  Connection is not closed 
							t42log_error("Read Error: %s <%d>\n", error.text, readret);
							break;
						}
						// if channel is closed, we want to fall through to force a reconnect
					}
				case RSSL_RET_FAILURE:
					{
						t42log_warn("channelInactive fd=%d <%s>\n",
							chnl->socketId,error.text);
						RecoverConnection();
					}
					break;
				case RSSL_RET_READ_FD_CHANGE:
					{
						t42log_info("rsslRead() FD Change - Old FD: %d New FD: %d\n", chnl->oldSocketId, chnl->socketId);
						FD_CLR(chnl->oldSocketId, &readfds_);
						FD_CLR(chnl->oldSocketId, &exceptfds_);
						FD_SET(chnl->socketId, &readfds_);
						FD_SET(chnl->socketId, &exceptfds_);
					}
					break;
				case RSSL_RET_READ_PING: 
					{
						//set flag for server message received 
						receivedServerMsg_ = RSSL_TRUE;
					}
					break;
				default:
					if (readret < 0 && readret != RSSL_RET_READ_WOULD_BLOCK)
					{
						t42log_error("Read Error: %s <%d>\n", error.text, readret);

					}
					break;
				}
			}
		}
	}
	else if (chnl->state == RSSL_CH_STATE_CLOSED)
	{
		t42log_warn("Channel fd=%d Closed.\n", chnl->socketId);
		RecoverConnection();
	}

	return RSSL_RET_SUCCESS;
}

void UPANIProvider::ProcessPings( RsslChannel* chnl )
{
	time_t currentTime = 0;

	// get current time 
	time(&currentTime);

	// time to send a ping
	if (currentTime >= nextSendPingTime_)
	{
		/* send ping to server */
		if (chnl && SendPing(chnl) != RSSL_RET_SUCCESS)
		{
			ExitThread();
			return;
		}

		// schedule next
		nextSendPingTime_ = currentTime + (time_t)pingTimeoutClient_;
	}

	// time to check if we received anything
	if (currentTime >= nextReceivePingTime_)
	{
		//  check if client received message from server since last time 
		if (receivedServerMsg_)
		{
			// reset flag/
			receivedServerMsg_ = RSSL_FALSE;

			// schedule next time
			nextReceivePingTime_ = currentTime + (time_t)pingTimeoutServer_;
		}
		else // lost contact
		{
			t42log_error("Lost contact with server...\n");
			RecoverConnection();
			return;
		}
	}

}

RsslRet UPANIProvider::ProcessResponse( RsslChannel* chnl, RsslBuffer* buffer )
{

	RsslRet ret = 0;
	RsslMsg msg = RSSL_INIT_MSG;
	RsslDecodeIterator dIter;
	UPALogin::RsslLoginResponseInfo *loginRespInfo = NULL;


	// reset the decode iterator
	rsslClearDecodeIterator(&dIter);

	// set version info 
	rsslSetDecodeIteratorRWFVersion(&dIter, chnl->majorVersion, chnl->minorVersion);

	if ((ret = rsslSetDecodeIteratorBuffer(&dIter, buffer)) != RSSL_RET_SUCCESS)
	{
		t42log_error("rsslSetDecodeIteratorBuffer() failed with return code: %d\n", ret);
		return RSSL_RET_FAILURE;
	}

	// Decode the message - note this only decode the outer parts of the message
	ret = rsslDecodeMsg(&dIter, &msg);				
	if (ret != RSSL_RET_SUCCESS)
	{
		t42log_error("rsslDecodeMsg(): Error %d on SessionData fd=%d  Size %d \n", ret, chnl->socketId, buffer->length);
		return RSSL_RET_FAILURE;
	}

	switch ( msg.msgBase.domainType )
	{
	case RSSL_DMT_LOGIN:
		if (login_->processLoginResponse(chnl, &msg, &dIter) != RSSL_RET_SUCCESS)
		{
			if (login_->IsClosed())
			{
				return RSSL_RET_FAILURE;
			}
			else if (login_->IsClosedRecoverable())
			{
				RecoverConnection();
			}
			else if (login_->IsSuspect())
			{
				isInLoginSuspectState_ = RSSL_TRUE;
			}
		}
		else
		{
			if (isInLoginSuspectState_)
			{
				isInLoginSuspectState_ = RSSL_FALSE;
			}
		}
		break;

	case RSSL_DMT_DICTIONARY:
		// we dont send a dictionary
		t42log_info("received RSSL_DMT_DICTIONARY message");

		break;
	}
	return RSSL_RET_SUCCESS;
}

void UPANIProvider::ExitThread()
{
	// shutdown the channel
	if ((rsslNIProviderChannel_ != NULL) && (rsslNIProviderChannel_->socketId != -1))
	{
		RemoveChannel(rsslNIProviderChannel_);
	}

	// shutdown rssl
	rsslUninitialize();

	// drop out of the thread loop
	runThread_ = false;

	return;
}

bool UPANIProvider::RequestLogin( mamaQueue requestQueue )
{
	return login_->QueueLogin(requestQueue, UPANIProvider::LoginRequestCb);
}

void MAMACALLTYPE UPANIProvider::LoginRequestCb( mamaQueue queue,void *closure )
{
	UPALogin * pLogin = (UPALogin*)closure;

	// doesnt seem to be an emu8m for this but provider == 1
	pLogin->Role(1);
	pLogin->SendLoginRequest();


}

bool UPANIProvider::SendSourceDirectory( mamaQueue requestQueue )
{
	UPASourceDirectory srcDir;

	// we send the source directory unsolicited for the NI Provider

	RsslMsgKey key = RSSL_INIT_MSG_KEY;

	key.filter = RDM_DIRECTORY_SERVICE_INFO_FILTER | \
		RDM_DIRECTORY_SERVICE_STATE_FILTER| \
		/*RDM_DIRECTORY_SERVICE_GROUP_FILTER | \ not applicable for refresh message - here for reference*/
		RDM_DIRECTORY_SERVICE_LOAD_FILTER | \
		/*RDM_DIRECTORY_SERVICE_DATA_FILTER | \ not applicable for non-ANSI Page based provider - here for reference*/
		RDM_DIRECTORY_SERVICE_LINK_FILTER;

	srcDir.SendSourceDirectoryResponse(rsslNIProviderChannel_, -1, &key, owner_->GetSource(), false);

	return true;
}

void MAMACALLTYPE UPANIProvider::SendSourceDirectoryCb( mamaQueue queue,void *closure )
{

}