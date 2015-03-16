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
#ifndef __RMDS_BRIDGETYPES_H__
#define __RMDS_BRIDGETYPES_H__

// Collect together boost shared_ptr typedefs. This solves a number of rpoblems with header file dependencies

class UPASubscription;
typedef boost::shared_ptr<UPASubscription> UPASubscription_ptr_t;

class RMDSSubscriber;
typedef boost::shared_ptr<RMDSSubscriber> RMDSSubscriber_ptr_t;

class RMDSPublisherBase;
typedef boost::shared_ptr<RMDSPublisherBase> RMDSPublisherBase_ptr_t;

class RMDSPublisher;
typedef boost::shared_ptr<RMDSPublisher> RMDSPublisher_ptr_t;

class RMDSNIPublisher;
typedef boost::shared_ptr<RMDSNIPublisher> RMDSNIPublisher_ptr_t;

class RMDSSource;
typedef boost::shared_ptr<RMDSSource> RMDSSource_ptr_t;

class UPAConsumer;
typedef boost::shared_ptr<UPAConsumer> UPAConsumer_ptr_t;

class UPAProvider;
typedef boost::shared_ptr<UPAProvider> UPAProvider_ptr_t;

class UPANIProvider;
typedef boost::shared_ptr<UPANIProvider> UPANIProvider_ptr_t;

class UPADictionaryWrapper;
typedef boost::shared_ptr<UPADictionaryWrapper> UPADictionaryWrapper_ptr_t;

class UPAItem;
typedef boost::shared_ptr<UPAItem> UPAItem_ptr_t;

class MamaPriceWrapper;
typedef boost::shared_ptr<MamaPriceWrapper> MamaPriceWrapper_ptr_t;

class MamaDateTimeWrapper;
typedef boost::shared_ptr<MamaDateTimeWrapper> MamaDateTimeWrapper_ptr_t;

class MamaMsgWrapper;
typedef boost::shared_ptr<MamaMsgWrapper> MamaMsgWrapper_ptr_t;

class MamaMsgVectorWrapper;
typedef boost::shared_ptr<MamaMsgVectorWrapper> MamaMsgVectorWrapper_ptr_t;

class MamaStringVectorWrapper;
typedef boost::shared_ptr<MamaStringVectorWrapper> MamaStringVectorWrapper_ptr_t;

class MamaMsgPayloadWrapper;
typedef boost::shared_ptr<MamaMsgPayloadWrapper>MamaMsgPayloadWrapper_ptr_t;

class RMDSBridgeSubscription;
typedef boost::shared_ptr<RMDSBridgeSubscription>RMDSBridgeSubscription_ptr_t;

class UPABridgePublisher;
typedef boost::shared_ptr<UPABridgePublisher>UPABridgePublisher_ptr_t;

class UPABridgePoster;
typedef boost::shared_ptr<UPABridgePoster> UPABridgePoster_ptr_t;


class UPABridgePublisherItem;
typedef boost::shared_ptr<UPABridgePublisherItem> UPABridgePublisherItem_ptr_t;

class RMDSTransportBridge;
typedef boost::shared_ptr<RMDSTransportBridge> RMDSTransportBridge_ptr_t;

class UPAPublisherItem;
typedef boost::shared_ptr<UPAPublisherItem> UPAPublisherItem_ptr_t;

class SnapshotReply;
typedef boost::shared_ptr<SnapshotReply> SnapshotReply_ptr_t;

class RMDSBridgeSnapshot;
typedef boost::shared_ptr<RMDSBridgeSnapshot> RMDSBridgeSnapshot_ptr_t;

class UpaMamaFieldMapHandler_t;
typedef boost::shared_ptr<UpaMamaFieldMapHandler_t> UPAMamaFieldMapHandler_ptr_t;

#endif //__RMDS_BRIDGETYPES_H__
