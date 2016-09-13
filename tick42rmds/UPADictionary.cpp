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
#include "transportconfig.h"

#include "UPADictionary.h"
#include "UPAConsumer.h"
#include <utils/filesystem.h>
#include <utils/t42log.h>
#include "RMDSFileSystem.h"

using namespace std;

const int FieldDictionaryStreamId = 3;
const int EnumDictionaryStreamId = 4;

UPADictionary::UPADictionary( const std::string &transport_name ) 
    : rsslDictionary_(new UPADictionaryWrapper) 
    , transport_name_(transport_name)
{
    fieldDictionaryStreamId_ = 0; 
    enumDictionaryStreamId_ = 0;

    LoadDictionaryFromFile();
}



UPADictionary::~UPADictionary(void)
{
}

void UPADictionary::AddListener( DictionaryResponseListener * pListener )
{
    listeners_.push_back(pListener);
}

bool UPADictionary::QueueRequest( mamaQueue queue )
{
    mama_status status;
    if ((status = mamaQueue_enqueueEvent(queue,  UPAConsumer::DictionaryRequestCb, (void*) this)) != MAMA_STATUS_OK)
    {
        t42log_error("Failed to enqueue dictionary request, status code = %d", status);
        return false;
    }
    return true;
}


bool UPADictionary::QueueMamaClientRequest(mamaQueue queue)
{
    mama_status status;
    if ((status = mamaQueue_enqueueEvent(queue,  UPAConsumer::ClientDictionaryRequestCb, (void*) this)) != MAMA_STATUS_OK)
    {
        t42log_error("Failed to enqueue mama client dictionary request, status code = %d", status);
        return false;
    }
    return true;
}


// if the file paths are configured in mama.properties then load the rmds dictionaries from the specified files (otherwise they will be subscribed)
void UPADictionary::LoadDictionaryFromFile()
{
    if (transport_name_.empty())
    {
        mama_log(MAMA_LOG_LEVEL_WARN, "UPADictionary::LoadDictionaryFromFile(): Can't load dictionary if transport name is missing!");
        return;
    }

    using namespace utils;

    TransportConfig_t config(transport_name_);

    std::string dictionaryFileNamePath = GetActualPath(config.getString("fieldfile"));
    if (!dictionaryFileNamePath.empty())
    {
        rsslDictionary_->LoadFieldDictionary(dictionaryFileNamePath);
        if (rsslDictionary_->GetFieldsStatus().loaded)
        {
            mama_log(MAMA_LOG_LEVEL_FINER, "UPADictionary::LoadDictionaryFromFile(): Load Fields dictionary from [%s]", dictionaryFileNamePath.c_str());
        }
    }
    else
        mama_log(MAMA_LOG_LEVEL_WARN, "UPADictionary::LoadDictionaryFromFile(): Cannot find fields dictionary in path '%s'", dictionaryFileNamePath.c_str());

    std::string enumTableFileNamePath = GetActualPath(config.getString("enumfile"));
    if (!enumTableFileNamePath.empty())
    {
        rsslDictionary_->LoadEnumTypeDictionary(enumTableFileNamePath);
        if (rsslDictionary_->GetEnumTypesStatus().loaded)
        {
            mama_log(MAMA_LOG_LEVEL_FINER, "UPADictionary::LoadDictionaryFromFile(): Loaded Enum Types dictionary from [%s]", enumTableFileNamePath.c_str());
        }
    }
    else
        mama_log(MAMA_LOG_LEVEL_WARN, "UPADictionary::LoadDictionaryFromFile(): Cannot find enumtype.def in path '%s'", enumTableFileNamePath.c_str());

    // Notify listeners later on when there are active listeners (see SendRequest)
}


bool UPADictionary::SendRequest()
{
    RsslRet ret;

    if (!rsslDictionary_->GetFieldsStatus().loaded)
    {
        if (ret = SendDictionaryRequest(DictionaryDownloadName, FieldDictionaryStreamId) != RSSL_RET_SUCCESS)
        {
            mama_log(MAMA_LOG_LEVEL_ERROR, "failed to send request for dictionary '%s' error code = %d", DictionaryDownloadName, ret);
            return false;
        }
    }

    if (!rsslDictionary_->GetEnumTypesStatus().loaded)
    {
        if (ret = SendDictionaryRequest(EnumTableDownloadName, EnumDictionaryStreamId) != RSSL_RET_SUCCESS)

        {
            mama_log(MAMA_LOG_LEVEL_ERROR, "failed to send request for dictionary '%s' error code = %d", EnumTableDownloadName, ret);
            return false;
        }
    }

    return true;
}



 // Sends a dictionary request to the rmds 

 // dictionaryName - The name of the dictionary to request
 // streamId - The stream id of the dictionary request 
 
RsslRet UPADictionary::SendDictionaryRequest(const char *dictionaryName, RsslInt32 streamId)
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

     // get a buffer for the dictionary request 
    msgBuf = rsslGetBuffer(UPAChannel_, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
         // encode the dictionary request 
        if (EncodeDictionaryRequest(msgBuf, dictionaryName, streamId) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_error("encodeDictionaryRequest() failed\n");
            return RSSL_RET_FAILURE;
        }

         //send request
        if (SendUPAMessage(UPAChannel_, msgBuf) != RSSL_RET_SUCCESS)
        {
            return RSSL_RET_FAILURE;
        }
    }
    else
    {
        t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}



 // Encodes the dictionary request.  Returns success
 // if encoding succeeds or failure if encoding fails.


 // msgBuf - The message buffer to encode the dictionary request into
 // dictionaryName - The name of the dictionary to request
  // streamId - The stream id of the dictionary request 
 
RsslRet UPADictionary::EncodeDictionaryRequest( RsslBuffer* msgBuf, const char *dictionaryName, RsslInt32 streamId)
{
    RsslRet ret = 0;
    RsslRequestMsg msg = RSSL_INIT_REQUEST_MSG;
    RsslEncodeIterator encodeIter;
         
    //clear encode iterator 
    rsslClearEncodeIterator(&encodeIter);

     
    // init the message 
    msg.msgBase.msgClass = RSSL_MC_REQUEST;
    msg.msgBase.streamId = streamId;
    msg.msgBase.domainType = RSSL_DMT_DICTIONARY;
    msg.msgBase.containerType = RSSL_DT_NO_DATA;
    msg.flags = RSSL_RQMF_NONE;

    msg.msgBase.msgKey.flags = RSSL_MKF_HAS_FILTER | RSSL_MKF_HAS_NAME_TYPE | RSSL_MKF_HAS_NAME | RSSL_MKF_HAS_SERVICE_ID;
    msg.msgBase.msgKey.nameType = RDM_INSTRUMENT_NAME_TYPE_RIC;
    msg.msgBase.msgKey.name.data = (char *)dictionaryName;
    msg.msgBase.msgKey.name.length = (RsslUInt32)strlen(dictionaryName);

    msg.msgBase.msgKey.filter = RDM_DICTIONARY_VERBOSE;

     
    // encode message 
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
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



 // Processes a dictionary response.  

 // chnl - The channel of the response
 // msg - The partially decoded message
 // dIter - The decode iterator
 
RsslRet UPADictionary::ProcessDictionaryResponse( RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslState *pState = 0;

    // create an rssl buffer to extract error messages
    char    errTxt[256];
    RsslBuffer errorText = {255, (char*)errTxt};

    // and a buffer to handle state to string conversiob
    char stateData[1024];
    RsslBuffer stateBuff;
    stateBuff.data = stateData;
    stateBuff.length = 1024;

    RDMDictionaryTypes dictionaryType = (RDMDictionaryTypes)0;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REFRESH:
        /* decode dictionary response */

        pState = &msg->refreshMsg.state;
        rsslStateToString(&stateBuff, pState);
        t42log_debug("    %s\n\n", stateBuff.data);

        if ((msg->msgBase.streamId != fieldDictionaryStreamId_) && (msg->msgBase.streamId != enumDictionaryStreamId_))
        {
            if (rsslExtractDictionaryType(dIter, &dictionaryType, &errorText) != RSSL_RET_SUCCESS)
            {
                t42log_error("rsslGetDictionaryType() failed: %.*s\n", errorText.length, errorText.data);
                NotifyListeners(false);
                return RSSL_RET_SUCCESS;
            }

             //The first part of a dictionary refresh should contain information about its type.
             // we use the stream id to identify which part of the dictionary have arrived
            switch (dictionaryType)
            {
            case RDM_DICTIONARY_FIELD_DEFINITIONS:
                fieldDictionaryStreamId_ = msg->msgBase.streamId; 
                break;
            case RDM_DICTIONARY_ENUM_TABLES:
                enumDictionaryStreamId_ = msg->msgBase.streamId;
                break;
            default:
                t42log_error("Unknown dictionary type %llu from message on stream %d\n", (RsslUInt)dictionaryType, msg->msgBase.streamId);
                NotifyListeners(false);
                return RSSL_RET_SUCCESS;
            }
        }

        // in either  case we need to insert the rnds dictionary into the merged dictionary
        if (msg->msgBase.streamId == fieldDictionaryStreamId_)
        {
            if (rsslDecodeFieldDictionary(dIter, &rsslDictionary_->GetRawDictionary(), RDM_DICTIONARY_VERBOSE, &errorText) != RSSL_RET_SUCCESS)
            {
                t42log_error("Decoding Dictionary failed: %.*s\n", errorText.length, errorText.data);
                NotifyListeners(false);
                return RSSL_RET_SUCCESS;
            }

            if (msg->refreshMsg.flags & RSSL_RFMF_REFRESH_COMPLETE)
            {
                rsslDictionary_->SetFieldsLoaded(true);
                fieldDictionaryStreamId_ = 0;
                if (!rsslDictionary_->GetEnumTypesStatus().loaded)
                    t42log_info("Field Dictionary complete, waiting for Enum Table...\n");
                else
                    t42log_info("Field Dictionary complete.\n");
            }
        } 
        else if (msg->msgBase.streamId == enumDictionaryStreamId_)
        {
            if (rsslDecodeEnumTypeDictionary(dIter, &rsslDictionary_->GetRawDictionary(), RDM_DICTIONARY_VERBOSE, &errorText) != RSSL_RET_SUCCESS)
            {
                t42log_error("Decoding Dictionary failed: %.*s\n", errorText.length, errorText.data);
                NotifyListeners(false);
                return RSSL_RET_SUCCESS;
            }

            if (msg->refreshMsg.flags & RSSL_RFMF_REFRESH_COMPLETE)
            {
                rsslDictionary_->SetEnumsLoaded(true);
                enumDictionaryStreamId_ = 0;
                if (!rsslDictionary_->GetEnumTypesStatus().loaded)
                    t42log_info("Enumerated Types Dictionary complete, waiting for Field Dictionary...\n");
                else
                    t42log_info("Enumerated Types Dictionary complete.\n");
            }
        }
        else
        {
            t42log_error("Received unexpected dictionary message on stream %d\n", msg->msgBase.streamId);
            NotifyListeners(false);
            return RSSL_RET_SUCCESS;
        }

        if (rsslDictionary_->isComplete())
        {
            t42log_info("Dictionary ready, requesting item...\n\n");
            NotifyListeners(true);

        }
        break;

    case RSSL_MC_STATUS:
        t42log_info("Received StatusMsg for dictionary\n");
        if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
        {
            RsslState *pState = &msg->statusMsg.state;
            rsslStateToString(&stateBuff, pState);
            t42log_info("    %s\n\n", stateBuff.data);
        }
        break;

    default:
        t42log_warn("Received Unhandled Dictionary MsgClass: %d\n", msg->msgBase.msgClass);
        break;
    }

    return RSSL_RET_SUCCESS;
}


 // Close the dictionary stream if there is one.

 // chnl - The channel to send a dictionary close to
 // streamId - The stream id of the dictionary stream to close 
 
RsslRet UPADictionary::CloseDictionaryStream(RsslInt32 streamId)
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

     
    //get a buffer for the dictionary close 
    msgBuf = rsslGetBuffer(UPAChannel_, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
        //encode the dictionary close  message
        if (EncodeDictionaryClose(msgBuf, streamId) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_error("encodeDictionaryClose() failed\n");
            return RSSL_RET_FAILURE;
        }

        // send it
        if (SendUPAMessage(UPAChannel_, msgBuf) != RSSL_RET_SUCCESS)
        {
            NotifyListeners(false);
            return RSSL_RET_FAILURE;
        }
    }
    else
    {
        t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}


 // Encodes the dictionary close.  Returns success if
 // encoding succeeds or failure if encoding fails.


 //msgBuf - The message buffer to encode the dictionary close into
 // streamId - The stream id of the dictionary stream to close 
 //
RsslRet UPADictionary::EncodeDictionaryClose(RsslBuffer* msgBuf, RsslInt32 streamId)
{
    RsslRet ret = 0;
    RsslCloseMsg msg = RSSL_INIT_CLOSE_MSG;
    RsslEncodeIterator encodeIter;

    /* clear encode iterator */
    rsslClearEncodeIterator(&encodeIter);

    /* set-up message */
    msg.msgBase.msgClass = RSSL_MC_CLOSE;
    msg.msgBase.streamId = streamId;
    msg.msgBase.domainType = RSSL_DMT_DICTIONARY;
    msg.msgBase.containerType = RSSL_DT_NO_DATA;
    
    /* encode message */
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
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


 // Calls the function to delete the dictionary, freeing all memory associated with it.
void UPADictionary::FreeDictionary()
{
    rsslDeleteDataDictionary(&rsslDictionary_->GetRawDictionary());
}

void UPADictionary::ResetDictionaryStreamId()
{
    fieldDictionaryStreamId_ = 0;
    enumDictionaryStreamId_ = 0;
}


// todo add the logic in the recover to deal with this

/* if the dictionary stream IDs are non-zero, it indicates we downloaded the dictionaries.  Because of this, we want to free the memory before recovery 
since we will download them again upon recovery.  If the stream IDs are zero, it implies no dictionary or dictionary was loaded from file, so we only 
want to release when we are cleaning up the entire app. */
RsslBool UPADictionary::NeedToDeleteDictionary()
{
    if ((fieldDictionaryStreamId_ != 0) || (enumDictionaryStreamId_ != 0))
        return RSSL_TRUE;
    else
        return RSSL_FALSE;
}


void UPADictionary::NotifyListeners( bool dictionaryComplete )
{
    vector<DictionaryResponseListener*>::iterator it = listeners_.begin();

    while(it != listeners_.end() )
    {
        (*it)->DictionaryUpdate(dictionaryComplete);
        it++;
    }


}

bool UPADictionary::IsComplete()
{
    return rsslDictionary_->isComplete();
}

void UPADictionary::NotifyComplete()
{
    NotifyListeners(true);
}
