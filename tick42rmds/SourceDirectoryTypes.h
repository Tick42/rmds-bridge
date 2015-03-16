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

// there is a huge amount of source information returned by the source directory response
// Rather than try incrementally add to the decoding in future versions of the bridge we have chosen
// to grab it all here, informed by the UPA documentation and sample applications
//
// refer to UPA-RDMUsageGuide.pdf section 4.3 for more detail


// some or all of these could be configurable. For the moment, the values usedUPA sample code looks reasonable
const size_t MaxCapabilities = 10;
const size_t MaxQOS = 5;
const size_t MaxDictionaries = 5;
const size_t MaxGroupInfoLength = 256;
const size_t MaxDataInfoLength = 1024;
const size_t MaxLinks = 5;
const size_t MaxSourceDirInfoStrLength = 256;


// Service General Information
typedef struct
{
	char		ServiceName[MaxSourceDirInfoStrLength];
	char		Vendor[MaxSourceDirInfoStrLength];
	RsslUInt64	IsSource;
	RsslUInt64	Capabilities[MaxCapabilities];
	char		DictionariesProvided[MaxDictionaries][MaxSourceDirInfoStrLength];
	char		DictionariesUsed[MaxDictionaries][MaxSourceDirInfoStrLength];
	RsslQos		QoS[MaxQOS];
	RsslUInt64	SupportsQosRange;
	char		ItemList[MaxSourceDirInfoStrLength];
	RsslUInt64	SupportsOutOfBandSnapshots;
	RsslUInt64	AcceptingConsumerStatus;
} RsslServiceGeneralInfo;

// service state information 
typedef struct
{
	RsslUInt64	ServiceState;
	RsslUInt64	AcceptingRequests;
	RsslState	Status;
} RsslServiceStateInfo;

// service group information 
typedef struct
{
	RsslUInt8	Group[MaxGroupInfoLength];
	RsslUInt8	MergedToGroup[MaxGroupInfoLength];
	RsslState	Status;
} RsslServiceGroupInfo;

// service load information 
typedef struct
{
	RsslUInt64	OpenLimit;
	RsslUInt64	OpenWindow;
	RsslUInt64	LoadFactor;
} RsslServiceLoadInfo;

// service data information 
typedef struct
{
	RsslUInt64	Type;
	RsslUInt8	Data[MaxDataInfoLength];
} RsslServiceDataInfo;


// service link information 
typedef struct
{
	char		LinkName[MaxSourceDirInfoStrLength];
	RsslUInt64	Type;
	RsslUInt64	LinkState;
	RsslUInt64	LinkCode;
	char		Text[MaxSourceDirInfoStrLength];
} RsslServiceLinkInfo;


// entire source directory response information 
typedef struct
{
	RsslInt32 StreamId;
	RsslUInt64 ServiceId;
	RsslServiceGeneralInfo ServiceGeneralInfo;
	RsslServiceStateInfo ServiceStateInfo;
	RsslServiceGroupInfo ServiceGroupInfo;
	RsslServiceLoadInfo ServiceLoadInfo;
	RsslServiceDataInfo ServiceDataInfo;
	RsslServiceLinkInfo ServiceLinkInfo[MaxLinks];
} RsslSourceDirectoryResponseInfo;




