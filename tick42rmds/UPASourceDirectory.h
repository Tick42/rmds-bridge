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
//#ifndef __UPASOURCEDIRECTORY_H__
//#define __UPASOURCEDIRECTORY_H__

#include "UPAMessage.h"

#include "SourceDirectoryTypes.h"

#include "SourceDirectoryResponseListener.h"
class SourceDirectoryResponseListener;
class RMDSPublisherSource;
class RMDSPublisher;

#include "RMDSPublisherSource.h"

class SourceDirectoryResponseListener;

// encapsulates the responses to a source directory request, handles updates and notifies state changes

class UPASourceDirectory
{
public:
    UPASourceDirectory(unsigned int maxMessageSize);
    ~UPASourceDirectory(void);


    void AddListener( SourceDirectoryResponseListener * pListener );
   void RemoveListener( SourceDirectoryResponseListener * pListener );

    bool QueueRequest(mamaQueue Queue);
    bool QueueRequest(mamaQueue Queue, mamaQueueEventCB cb);

    bool SendRequest();

    RsslChannel* UPAChannel() const { return UPAChannel_; }
    void UPAChannel(RsslChannel* val) { UPAChannel_ = val; }

    RsslRet ProcessSourceDirectoryResponse(RsslMsg* msg, RsslDecodeIterator* dIter);



    // currently just the reasons in the rssl provider sample, but might want to add more
    typedef enum
    {
    MaxSrcdirRequestsReached = 0,
    IncorrectFilterFlags = 1
    } RsslSrcDirRequestRejectReason;


    RsslRet SendSrcDirectoryRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslSrcDirRequestRejectReason reason);

    // send the response with all the source services
    RsslRet SendSourceDirectoryResponse(RsslChannel* chnl, RsslInt32 streamId, RsslMsgKey* msgKey, RMDSPublisherSource *source, bool solicited = true);

private:
   typedef std::vector<SourceDirectoryResponseListener *> listener_t;
    listener_t listeners_;

    RsslChannel* UPAChannel_;
    unsigned int maxMessageSize_;

    // encode the request;
    RsslRet EncodeSourceDirectoryRequest(RsslBuffer* msgBuf, RsslInt32 streamId);

    // and the close
    RsslRet  EncodeSourceDirectoryClose(RsslBuffer* msgBuf, RsslInt32 streamId);

    // decoding functions
    RsslRet DecodeSourceDirectoryResponse(RsslDecodeIterator* dIter, bool isRefresh, RsslUInt32 maxCapabilities, RsslUInt32 maxQOS, RsslUInt32 maxDictionaries, RsslUInt32 maxLinks);
    RsslRet DecodeServiceGeneralInfo(RsslServiceGeneralInfo* serviceGeneralInfo, RsslDecodeIterator* dIter,    RsslUInt32 maxCapabilities, RsslUInt32 maxQOS, RsslUInt32 maxDictionaries);
    RsslRet DecodeServiceLinkInfo(RsslServiceLinkInfo* serviceLinkInfo, RsslDecodeIterator* dIter, RsslUInt32 maxLinks);
    RsslRet DecodeServiceDataInfo(RsslServiceDataInfo* serviceDataInfo, RsslDecodeIterator* dIter);
    RsslRet DecodeServiceLoadInfo(RsslServiceLoadInfo* serviceLoadInfo, RsslDecodeIterator* dIter);
    RsslRet DecodeServiceGroupInfo(RsslServiceGroupInfo* serviceGroupInfo,  RsslDecodeIterator* dIter);
    RsslRet DecodeServiceStateInfo(RsslServiceStateInfo* serviceStateInfo,  RsslDecodeIterator* dIter);

    RsslRet CloseSourceDirectoryStream();


    RsslRet EncodeSrcDirectoryRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslSrcDirRequestRejectReason reason, RsslBuffer* msgBuf);

    //RsslRet EncodeSourceDirectoryResponse(RsslChannel* chnl, RMDSPublisher * pub, RsslInt32 streamId, RsslMsgKey* requestKey, RsslBuffer* msgBuf, RsslUInt16 refreshFlags);
    RsslRet EncodeSourceDirectoryResponse(RsslChannel* chnl, RMDSPublisherSource *, RsslInt32 streamId, RsslMsgKey* requestKey, RsslBuffer* msgBuf, RsslUInt16 refreshFlags);

    RsslRet EncodeSourceDirectoryInfo(RsslChannel* chnl, RMDSPublisherSource * srcInfo,  RsslMsgKey* requestKey, RsslBuffer* msgBuf, RsslEncodeIterator * eIter);

    RsslRet EncodeServiceGeneralInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter);
    RsslRet EncodeServiceStateInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter);
    RsslRet EncodeServiceLoadInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter);
    RsslRet EncodeServiceLinkInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter);

    void NotifyListenersUpdate(RsslSourceDirectoryResponseInfo *pResponseInfo, bool isRefresh );
    void NotifyListenersComplete(bool succeeded);

    // clear down the response structures

    // clear a single source directory response info
    void ResetSourceDirRespInfo(RsslSourceDirectoryResponseInfo* srcDirRespInfo);
    void ResetServiceGeneralInfo(RsslServiceGeneralInfo * serviceGeneralInfo);
    void ResetServiceStateInfo(RsslServiceStateInfo * serviceStateInfo);
    void ResetServiceGroupInfo(RsslServiceGroupInfo * serviceGroupInfo);
    void ResetServiceLoadInfo(RsslServiceLoadInfo * serviceLoadInfo);
    void ResetServiceDataInfo(RsslServiceDataInfo * serviceDataInfo);
    void ResetServiceLinkInfo(RsslServiceLinkInfo * serviceLinkInfo);
};



//#endif //__UPASOURCEDIRECTORY_H__

