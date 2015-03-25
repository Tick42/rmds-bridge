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

#include "transport.h"
#include "tick42rmdsbridgefunctions.h"
#include "RMDSBridgeImpl.h"
#include "transportconfig.h"

#ifdef ENABLE_TICK42_ENHANCED
// Additional functionality provided by the enhanced bridge is available as part of a a support package
// please contact support@tick42.com
#include "enhanced/T42Enh_RMDSSubscriber.h"
#include "enhanced/T42Enh_RMDSPublisher.h"
#else
#include "RMDSSubscriber.h"
#include "RMDSPublisher.h"
#endif	//ENABLE_TICK42_ENAHNCED

#include "RMDSNIPublisher.h"
#include "UPATransportNotifier.h"

using namespace utils::thread;

//
namespace {
   static RMDSSubscriber * pSubscriber = NULL;
   static UPATransportNotifier *gNotify = NULL;
} /*namespace anonymous*/
/*=========================================================================
=                         Mandatory Functions                            =
=========================================================================*/

mama_status
   tick42rmdsBridgeMamaTransport_create (transportBridge*  result,
   const char*       name,
   mamaTransport     mamaTport )
{
   mama_status status;

   char buff[256];

#ifdef ENABLE_TICK42_ENHANCED
   // Additional functionality provided by the enhanced bridge is available as part of a a support package
   // please contact support@tick42.com
   sprintf(buff,"******* Initializing transport %s on Tick42-enhanced RMDS bridge %s : version %s *******\n", name
      , tick42rmdsBridge_getName(), tick42rmdsBridge_getVersion());
#else
   sprintf(buff,"******* Initializing transport %s on RMDS bridge %s : version %s *******\n", name
      , tick42rmdsBridge_getName(), tick42rmdsBridge_getVersion());
#endif // ENABLE_TICK42_ENHANCED
   
   // make sure it  prints this unless the logging is very restricted
   mama_log(MAMA_LOG_LEVEL_WARN, buff);


   // some logic here to validate the transport name
   //
   // this is not as easy as it might appear because all the properties have valid defaults so in principal one could define a transpoprt with no properties
   //
   // but we can rely on the fact that if the hosts property is missing or empty this only makes sense for an interactive publisher 
   // so the transport.<tport_name>.source property must be set
   TransportConfig_t config(name);

   string hosts = config.getString("hosts");
   string source = config.getString("source");

   if(hosts.empty() && source.empty())
   {
	   t42log_error("ERROR - Transport name %s is invalid\n", name);
	   return MAMA_STATUS_INVALID_ARG;
   }



   // get hold of the bridge implementation
   mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(mamaTport);
   if (!bridgeImpl) {
      mama_log (MAMA_LOG_LEVEL_ERROR, "tick42rmdsBridgeMamaTransport_create(): Could not get bridge");
      return MAMA_STATUS_PLATFORM;
   }

   RMDSBridgeImpl*  upaBridge = NULL;
   if (MAMA_STATUS_OK != (status = mamaBridgeImpl_getClosure((mamaBridge) bridgeImpl, (void**) &upaBridge))) 
   {
      mama_log (MAMA_LOG_LEVEL_ERROR, "tick4rmdsBridgeMamaTransport_create(): Could not get UPA bridge object");
      return status;
   }


   gNotify = new UPATransportNotifier(mamaTport); 


   // create out transport implemententation and hook it to the mama object
   boost::shared_ptr<RMDSTransportBridge> transport(new RMDSTransportBridge(name));

   transport->SetTransport(mamaTport);


   *result = (transportBridge)transport.get();

   upaBridge->setTransportBridge(transport);

   mama_status transport_result = transport->Start();
   if (transport_result != MAMA_STATUS_OK)
   {
      transport->Stop();
      return transport_result;
   }


   return MAMA_STATUS_OK;

}


mama_status
   tick42rmdsBridgeMamaTransport_destroy (transportBridge transport)
{

   RMDSTransportBridge * bridgeTransport = (RMDSTransportBridge *)transport;
   bridgeTransport->Stop();

   if (gNotify) 
   {
      delete gNotify;
      gNotify = NULL;
   }
   return MAMA_STATUS_OK;
}


int
   tick42rmdsBridgeMamaTransport_isValid (transportBridge transport)
{
   return MAMA_STATUS_PLATFORM;
}

/*=========================================================================
=                        Recommended Functions                           =
=========================================================================*/

mama_status
   tick42rmdsBridgeMamaTransport_getNumLoadBalanceAttributes (
   const char* name,
   int*        numLoadBalanceAttributes)
{
   *numLoadBalanceAttributes = 0;
   return MAMA_STATUS_OK;
}


mama_status
   tick42rmdsBridgeMamaTransport_getLoadBalanceSharedObjectName (
   const char*  name,
   const char** loadBalanceSharedObjectName)
{
   *loadBalanceSharedObjectName = NULL;
   return MAMA_STATUS_OK;
}


mama_status
   tick42rmdsBridgeMamaTransport_getLoadBalanceScheme (
   const char*    name,
   tportLbScheme* scheme)
{
   *scheme = TPORT_LB_SCHEME_STATIC;
   return MAMA_STATUS_OK;
}


mama_status
   tick42rmdsBridgeMamaTransport_findConnection (transportBridge*  transports,
   int               numTransports,
   mamaConnection*   result,
   const char*       ipAddress,
   uint16_t          port)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_getAllConnections (transportBridge*  transports,
   int               numTransports,
   mamaConnection**  result,
   uint32_t*         len)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_getAllConnectionsForTopic (
   transportBridge* transports,
   int              numTransports,
   const char*      topic,
   mamaConnection** result,
   uint32_t*        len)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_freeAllConnections (
   transportBridge* transports,
   int              numTransports,
   mamaConnection*  result,
   uint32_t         len)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_getAllServerConnections (
   transportBridge*       transports,
   int                    numTransports,
   mamaServerConnection** result,
   uint32_t*              len)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_freeAllServerConnections (
   transportBridge*      transports,
   int                   numTransports,
   mamaServerConnection* result,
   uint32_t              len)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_sendMsgToConnection (transportBridge  tport,
   mamaConnection   connection,
   mamaMsg          msg,
   const char*      topic)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_installConnectConflateMgr (
   transportBridge       handle,
   mamaConflationManager mgr,
   mamaConnection        connection,
   conflateProcessCb     processCb,
   conflateGetMsgCb      msgCb)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_uninstallConnectConflateMgr (
   transportBridge       handle,
   mamaConflationManager mgr,
   mamaConnection        connection)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_isConnectionIntercepted (mamaConnection connection,
   uint8_t*       result)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_startConnectionConflation (
   transportBridge       tport,
   mamaConflationManager mgr,
   mamaConnection        connection)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_getNativeTransport (transportBridge transport,
   void**          result)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_getNativeTransportNamingCtx (transportBridge transport,
   void**          result)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_requestConflation (transportBridge* transports,
   int              numTransports)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
   tick42rmdsBridgeMamaTransport_requestEndConflation (transportBridge* transports,
   int              numTransports)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}

extern mama_status 
   tick42rmdsBridgeMamaTransport_forceClientDisconnect (transportBridge* transports,
   int              numTransports,
   const char*      ipAddress,
   uint16_t         port)
{
   return MAMA_STATUS_NOT_IMPLEMENTED;
}



// RMDSTransportBridge implementation
//

RMDSTransportBridge* RMDSTransportBridge::GetTransport(mamaTransport transport)
{
   RMDSTransportBridge* pTransportBridge;
   mama_status status = mamaTransport_getBridgeTransport (transport, (transportBridge*)&pTransportBridge);
   if ((status != MAMA_STATUS_OK) || (pTransportBridge == NULL))
   {
      return 0;
   }

   return pTransportBridge;
}

RMDSExpDll RMDSTransportBridge::RMDSTransportBridge(const std::string &name)
   : transport_(0)
   , paused_(false)
   , started_(false)
   , stopped_(false)
   , name_(name)
   , DictionaryReply_()
   , subscriber_()
   , publisher_()
   , niPublisher_()
{

}

RMDSTransportBridge::~RMDSTransportBridge()
{
}

mama_status RMDSTransportBridge::Start()
{
   //shouldn't be stopped if haven't started yet!
   if (stopped_)
   {
      mama_log(MAMA_LOG_LEVEL_ERROR, "Trying to access transport that is already stopped!");
      return MAMA_STATUS_NULL_ARG;
   }

   if (started_)
   {
      // its probably OK if we are already started 
      return MAMA_STATUS_OK;
   }

   // get hold of the bridge implementation
   mamaBridgeImpl* bridgeImpl = mamaTransportImpl_getBridgeImpl(transport_);
   if (!bridgeImpl) {
      mama_log (MAMA_LOG_LEVEL_ERROR, "RMDSTransportBridge::Start(): Could not get bridge");
      return MAMA_STATUS_PLATFORM;
   }

   // put rssl init here as it will be for both publisher and subscriber
   // and use rssl lock global as we will have separate threads for the pub and the sub
   // don't need channel locking though as channels will be accessed from the same thread
   RsslRet ret;
   RsslError error;
   if (ret = rsslInitialize(RSSL_LOCK_GLOBAL_AND_CHANNEL, &error) != RSSL_RET_SUCCESS)
   {
      t42log_error("Failed to initialise rssl - error code = %d", ret);
      return MAMA_STATUS_PLATFORM;
   }

   // there are circumstances where this gets called more than once so guard in the initialisation
   T42Lock lock(&cs_);
   if (!subscriber_)
   {

#ifdef ENABLE_TICK42_ENHANCED
	   // Additional functionality provided by the enhanced bridge is available as part of a a support package
	   // please contact support@tick42.com
	   subscriber_ = boost::shared_ptr<RMDSSubscriber> (new T42Enh_RMDSSubscriber(*gNotify)); 
#else
	   subscriber_ = boost::shared_ptr<RMDSSubscriber> (new RMDSSubscriber(*gNotify)); 
#endif	//ENABLE_TICK42_ENHANCED

	   subscriber_->Initialize((mamaBridge)bridgeImpl, transport_, name_);

	   if (!subscriber_->Start(NULL, RSSL_CONN_TYPE_SOCKET ))
	   {
		  if (ret == RSSL_RET_SUCCESS)
			 rsslUninitialize();
		  started_ = false;
		  return MAMA_STATUS_PLATFORM;
	   }

   }


   // and if we have a publisher then create and start that too
   TransportConfig_t config(name_);
   if (config.getString("pubport") != "" && publisher_ == 0)
   {
      // create and start up a publisher too
#ifdef ENABLE_TICK42_ENHANCED
	   // Additional functionality provided by the enhanced bridge is available as part of a a support package
	   // please contact support@tick42.com
	   publisher_ = RMDSPublisherBase_ptr_t(new T42Enh_RMDSPublisher(*gNotify));

#else
      publisher_ = RMDSPublisherBase_ptr_t(new RMDSPublisher(*gNotify));
#endif

      publisher_->Initialize((mamaBridge)bridgeImpl, transport_, name_);

      publisher_->Start();

   }

   // if we have an NI publisher create and start that
   if (config.getString("pubhosts") != "" && niPublisher_ == 0)
   {
      niPublisher_ = RMDSPublisherBase_ptr_t(new RMDSNIPublisher(*gNotify));

      if (niPublisher_->Initialize((mamaBridge)bridgeImpl, transport_, name_))
      {
         niPublisher_->Start();
      }
   }

   started_ = true;
   return MAMA_STATUS_OK;
}

mama_status RMDSTransportBridge::Stop()
{
   mama_log (MAMA_LOG_LEVEL_NORMAL, "RMDSTransportBridge::Stop(%s)\n", name_.c_str());

   // stop all the services
   stopped_ = true;

   {
      T42Lock lock(&cs_);
      if (subscriber_)
      {
         subscriber_->Stop();
		 mama_log (MAMA_LOG_LEVEL_NORMAL, "RMDSTransportBridge::Stop(%s) - subscriber stopped\n", name_.c_str());
      }
   }

   // shutdown rssl
   rsslUninitialize();

   subscriber_.reset();

   mama_log (MAMA_LOG_LEVEL_NORMAL, "RMDSTransportBridge::Stop(%s) - rssl uninitialised\n", name_.c_str());


   return MAMA_STATUS_OK;   
}

void RMDSTransportBridge::setDictionaryReply( boost::shared_ptr<DictionaryReply_t> dictionaryReply )
{
   DictionaryReply_ = dictionaryReply; 
   
   T42Lock lock(&cs_);
   if (subscriber_)
   {
      subscriber_->SetDictionaryReply(DictionaryReply_);
   }
}

mama_status RMDSTransportBridge::Pause()
{
   // stop the statistics logger thread now, in case we are shutting down the whole bridge
   StatisticsLogger::PauseUpdates();

   T42Lock lock(&cs_);
   if (subscriber_)
   {
      subscriber_->PauseUpdates();
   }
   paused_ = true;
   return MAMA_STATUS_OK;
}

mama_status RMDSTransportBridge::Resume()
{
   if (stopped_)
   {
      t42log_error("cannot resume bridge - it has been stopped");
      return MAMA_STATUS_PLATFORM;
   }
   if (!paused_)
   {
      // Treat the first Resume() just like a Start()
      return Start();
   }
   paused_ = false;
   
   T42Lock lock(&cs_);
   if (subscriber_)
   {
      subscriber_->ResumeUpdates();
   }
   // ok to resume the statistics logger thread
   StatisticsLogger::ResumeUpdates();
   return MAMA_STATUS_OK;
}

void RMDSTransportBridge::SendSnapshotRequest( SnapshotReply_ptr_t snapReply )
{

	// should lock this to avoid shutdown issues
	T42Lock lock(&cs_);
	if (subscriber_)
	{
		subscriber_->SendSnapshotRequest(snapReply);
	}
}