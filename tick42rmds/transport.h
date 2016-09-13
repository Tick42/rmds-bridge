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
#ifndef RMDS_TRANPSORT_BRIDGE_H
#define RMDS_TRANPSORT_BRIDGE_H

#include "rmdsdefs.h"

#include "DictionaryReply.h"
#include <utils/thread/lock.h>
#include "rmdsBridgeTypes.h"

class RMDSSubscriber;

//
// Bridge implemenation of the transport, contains subscriber and publisher as configured and maps from the mama C interafce
//

class RMDSTransportBridge
{
public:

   RMDSExpDll RMDSTransportBridge(const std::string &name);

   ~RMDSTransportBridge();

   // set the mama transport handle
   void SetTransport(mamaTransport transport)
   {
      if (!stopped_)
         transport_ = transport;
      else
         transport_=0;
   }


   //get  MAMA transport handle
   mamaTransport GetTransport() const
   {
      if (!stopped_)
         return transport_;
      else
         return 0;
   }


   // Starts RMDS Session and services
   RMDSExpDll mama_status Start();

   // Stops RMDS Session.
   RMDSExpDll mama_status Stop();

   RMDSExpDll mama_status Pause();
   RMDSExpDll mama_status Resume();

   RMDSSubscriber_ptr_t Subscriber() const { return subscriber_; }
   RMDSPublisherBase_ptr_t Publisher() const { return publisher_; }
   RMDSPublisherBase_ptr_t NIPublisher() const {return niPublisher_;}

   std::string Name() const
   {
      return name_;
   }

public:
   static RMDSTransportBridge* GetTransport(mamaTransport transport);

private:
   void ProcessSubscriptions();

private:
   mamaTransport transport_;

   RMDSSubscriber_ptr_t subscriber_;
   RMDSPublisherBase_ptr_t publisher_;
   RMDSPublisherBase_ptr_t niPublisher_;

   mutable utils::thread::lock_t cs_;
   bool started_;
   bool stopped_;
   bool paused_;


   std::string name_;
   boost::shared_ptr<DictionaryReply_t> DictionaryReply_;
public:
    void setDictionaryReply(boost::shared_ptr<DictionaryReply_t> dictionaryReply);
    void SendSnapshotRequest(SnapshotReply_ptr_t snapReply);

};

#endif
