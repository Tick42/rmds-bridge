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
#ifndef __UPADICTIONARY_H__
#define __UPADICTIONARY_H__

#include "UPAMessage.h"
#include "rmdsBridgeTypes.h"
#include "UPADictionaryWrapper.h"
#include "transportconfig.h"

class DictionaryResponseListener;

// Wraps the RSSL dictionary request and response messages
//
// Also manages the ability to load the dictionary from a file rather than subscribe from the RMDS

class UPADictionary
{
public:
    UPADictionary(const std::string &transport_name);
    virtual ~UPADictionary(void);

    void AddListener( DictionaryResponseListener * pListener );

    // queue a request to rmds for the dictionary
    bool QueueRequest(mamaQueue Queue);

    // queue a request from the mamaclient
    // this decouples handling mama client dictionary subscriptions from the rmds dictionary request process
    bool QueueMamaClientRequest(mamaQueue queue);
    bool HandleClientRequest();

    // request the dictionary from RMDS
    bool SendRequest();

    bool IsComplete();

    void NotifyComplete();

    void LoadDictionaryFromFile(const TransportConfig_t& config);
    RsslChannel* UPAChannel() const { return UPAChannel_; }
    void UPAChannel(RsslChannel* val) { UPAChannel_ = val; }


    // called from consumer message loop
    RsslRet ProcessDictionaryResponse(RsslMsg* msg, RsslDecodeIterator* dIter);


    UPADictionaryWrapper_ptr_t RsslDictionary() const { return rsslDictionary_; }
    UPADictionaryWrapper_ptr_t GetUnderlyingDictionary() {return rsslDictionary_;}
private:

    void NotifyListeners( bool dictionaryComplete );
    std::vector<DictionaryResponseListener *> listeners_;

    RsslChannel* UPAChannel_;

    /* dictionary loaded flag */
    RsslBool fieldDictionaryLoaded_ ;
    /* dictionary loaded from file flag */
    RsslBool fieldDictionaryLoadedFromFile_;

    /* enum table loaded flag */
    RsslBool enumTypeDictionaryLoaded_ ;
    /* enum table loaded from file flag */
    RsslBool enumTypeDictionaryLoadedFromFile_;
    /* enum table file name */

    // send requests to channel
    RsslRet SendDictionaryRequest(const char *dictionaryName, RsslInt32 streamId);
    RsslRet EncodeDictionaryRequest( RsslBuffer* msgBuf, const char *dictionaryName, RsslInt32 streamId);


    RsslInt32 fieldDictionaryStreamId_;
    RsslInt32 enumDictionaryStreamId_;

    RsslRet CloseDictionaryStream( RsslInt32 streamId);
    RsslRet EncodeDictionaryClose( RsslBuffer* msgBuf, RsslInt32 streamId);
    void FreeDictionary();
    void ResetDictionaryStreamId();
    RsslBool NeedToDeleteDictionary();


    // wrapper on the underlying dictionary
    UPADictionaryWrapper_ptr_t rsslDictionary_;

    std::string transport_name_;
    unsigned int maxMessageSize_;
};

class DictionaryResponseListener
{
public:
    virtual void DictionaryUpdate(bool dictionaryComplete) = 0;
};

#endif //__UPADICTIONARY_H__

