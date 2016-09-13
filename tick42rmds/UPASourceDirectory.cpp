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
#include "UPASourceDirectory.h"
#include "UPAConsumer.h"
#include "RMDSPublisher.h"
#include "RMDSPublisherSource.h"
#include <utils/t42log.h>

const int srcDirStreamId = 2;

UPASourceDirectory::UPASourceDirectory()
{
}

UPASourceDirectory::~UPASourceDirectory(void)
{
}

void UPASourceDirectory::AddListener( SourceDirectoryResponseListener * pListener )
{
    listeners_.push_back(pListener);
}

void UPASourceDirectory::RemoveListener( SourceDirectoryResponseListener * pListener )
{
    for (listener_t::iterator itListener = listeners_.begin();
        itListener != listeners_.end(); ++itListener)
    {
        if (*itListener == pListener)
        {
            listeners_.erase(itListener);
            return;
        }
    }
}

// Sends a source directory request to a channel.  This consists
// of getting a message buffer, encoding the source directory request,
// and sending the source directory request to the server.
// chnl - The channel to send a source directory request to
//
bool UPASourceDirectory::SendRequest()
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

    if (UPAChannel_ == 0)
    {
        mama_log(MAMA_LOG_LEVEL_WARN,"UPASourceDirectory::sendRequest - No Channel");
        return false;
    }

     //get a buffer for the source directory request 
    msgBuf = rsslGetBuffer(UPAChannel_, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
         //encode source directory request 
        if (EncodeSourceDirectoryRequest(msgBuf, srcDirStreamId) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_error("encodeSourceDirectoryRequest() failed\n");
            return false;
        }

         //send source directory request 
        if (SendUPAMessage(UPAChannel_, msgBuf) != RSSL_RET_SUCCESS)
            return false;
    }
    else
    {
        t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
        return false;
    }

    return true;
}

RsslRet UPASourceDirectory::ProcessSourceDirectoryResponse(RsslMsg* msg, RsslDecodeIterator* dIter)
{
    RsslState *pState = 0;
    char tempData[1024];
    RsslBuffer tempBuffer;

    tempBuffer.data = tempData;
    tempBuffer.length = 1024;

    switch(msg->msgBase.msgClass)
    {
    case RSSL_MC_REFRESH:

        //decode source directory response
        if (DecodeSourceDirectoryResponse(dIter, true,
            MaxCapabilities,
            MaxQOS,
            MaxDictionaries,
            MaxLinks) != RSSL_RET_SUCCESS)
        {
            t42log_error("decodeSourceDirectoryResponse() failed\n");
            NotifyListenersComplete(false);
            return RSSL_RET_FAILURE;
        }

        t42log_info("Received Source Directory Refresh\n");

        NotifyListenersComplete(true);

        break;

    case RSSL_MC_UPDATE:
        //decode source directory response
        if (DecodeSourceDirectoryResponse(dIter, false,
            MaxCapabilities,
            MaxQOS,
            MaxDictionaries,
            MaxLinks) != RSSL_RET_SUCCESS)
        {
            t42log_error("decodeSourceDirectoryResponse() failed\n");
            return RSSL_RET_FAILURE;
        }

        t42log_debug("Received Source Directory Update\n");
        break;

    case RSSL_MC_CLOSE:
        t42log_info("Received Source Directory Close\n");
        break;

    case RSSL_MC_STATUS:
        t42log_info("Received Source Directory StatusMsg\n");
        if (msg->statusMsg.flags & RSSL_STMF_HAS_STATE)
        {
            pState = &msg->statusMsg.state;
            rsslStateToString(&tempBuffer, pState);
            t42log_info("    %s\n\n", tempBuffer.data);
        }
        break;

    default:
        t42log_info("Received unhandled Source Directory Msg Class: %d\n", msg->msgBase.msgClass);
        break;
    }

    return RSSL_RET_SUCCESS;
}

// Close the source directory stream.

RsslRet UPASourceDirectory::CloseSourceDirectoryStream()
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

    // get a buffer for the source directory close
    msgBuf = rsslGetBuffer(UPAChannel_, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
        // encode the source directory close message
        if (EncodeSourceDirectoryClose(msgBuf, srcDirStreamId) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_error("encodeSourceDirectoryClose() failed\n");
            return RSSL_RET_FAILURE;
        }

        // send the  close message
        if (SendUPAMessage(UPAChannel_, msgBuf) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
    }
    else
    {
        t42log_error("rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}

/// decoders

//Decodes the source directory response into the RsslSourceDirectoryResponseInfo structure.  Returns success if decoding succeeds or failure if decoding fails.

//dIter - The decode iterator
//maxCapabilities - The maximum number of capabilities that the structure holds
//maxQOS - The maximum number of QOS entries that the structure holds
//maxDictionaries - The maximum number of dictionaries that the structure holds
//maxLinks - The maximum number of link entries that the structure holds


// We fire a separate notification for each sourcedirectoryinfo which is handled by the bridge infrastructure to maintain a record of the set of sources and their states
// The client code can then extract relevant information from the structure

RsslRet UPASourceDirectory::DecodeSourceDirectoryResponse(RsslDecodeIterator *dIter, bool isRefresh,
                                                          RsslUInt32 maxCapabilities, RsslUInt32 maxQOS, RsslUInt32 maxDictionaries, RsslUInt32 maxLinks)
{
    RsslRet ret = 0;
    RsslMap map;

    //decode source directory response

    // decode the map
    if ((ret = rsslDecodeMap(dIter, &map)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeMap() failed with return code: %d\n", ret);
        return ret;
    }

    // source directory response key data type must be RSSL_DT_UINT
    if (map.keyPrimitiveType != RSSL_DT_UINT)
    {
        t42log_error("Map has incorrect keyPrimitiveType: %s", rsslDataTypeToString(map.keyPrimitiveType));
        return RSSL_RET_FAILURE;
    }

    // decode map entries
    RsslMapEntry mapEntry;
    RsslUInt64 serviceIdTemp;

    while ((ret = rsslDecodeMapEntry(dIter, &mapEntry, &serviceIdTemp)) != RSSL_RET_END_OF_CONTAINER)
    {
        // so create an instance of the ResponseInfo on the stack. This is what will be pass up in the notification
        RsslSourceDirectoryResponseInfo srcDirRespInfo;

        // clear down the struct
        ResetSourceDirRespInfo(&srcDirRespInfo);

        bool hasStateInfo = false;
        if (ret == RSSL_RET_SUCCESS)
        {
            if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeMapEntry() failed with return code: %d\n", ret);
                return ret;
            }

            srcDirRespInfo.ServiceId = serviceIdTemp;

            // decode filter list
            RsslFilterList rsslFilterList;
            if ((ret = rsslDecodeFilterList(dIter, &rsslFilterList)) < RSSL_RET_SUCCESS)
            {
                t42log_error("rsslDecodeFilterList() failed with return code: %d\n", ret);
                return ret;
            }

            // for each map entry we get a filterlist telling us which items are encoded for this source. We then decode each of them
            //
            // The names are self-explanatory

            RsslFilterEntry filterListItem;

            while ((ret = rsslDecodeFilterEntry(dIter, &filterListItem)) != RSSL_RET_END_OF_CONTAINER)
            {
                if (ret == RSSL_RET_SUCCESS)
                {
                    switch (filterListItem.id)
                    {
                    case RDM_DIRECTORY_SERVICE_INFO_ID:
                        if ((ret = DecodeServiceGeneralInfo(&srcDirRespInfo.ServiceGeneralInfo, dIter, maxCapabilities, maxQOS, maxDictionaries)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceGeneralInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        break;

                    case RDM_DIRECTORY_SERVICE_STATE_ID:
                        if ((ret = DecodeServiceStateInfo(&srcDirRespInfo.ServiceStateInfo, dIter)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceStateInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        hasStateInfo = true;
                        break;

                    case RDM_DIRECTORY_SERVICE_GROUP_ID:
                        if ((ret = DecodeServiceGroupInfo(&srcDirRespInfo.ServiceGroupInfo, dIter)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceGroupInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        break;

                    case RDM_DIRECTORY_SERVICE_LOAD_ID:
                        if ((ret = DecodeServiceLoadInfo(&srcDirRespInfo.ServiceLoadInfo, dIter)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceLoadInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        break;

                    case RDM_DIRECTORY_SERVICE_DATA_ID:
                        if ((ret = DecodeServiceDataInfo(&srcDirRespInfo.ServiceDataInfo, dIter)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceDataInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        break;

                    case RDM_DIRECTORY_SERVICE_LINK_ID:
                        if ((ret = DecodeServiceLinkInfo(&srcDirRespInfo.ServiceLinkInfo[0], dIter, maxLinks)) != RSSL_RET_SUCCESS)
                        {
                            t42log_error("decodeServiceLinkInfo() failed with return code: %d\n", ret);
                            return ret;
                        }
                        break;

                    default:
                        t42log_error("Unknown FilterListEntry filterID: %d\n", filterListItem.id);
                        return RSSL_RET_FAILURE;
                    }
                }
            }
        }

        if (hasStateInfo)
        {
            NotifyListenersUpdate(&srcDirRespInfo, isRefresh);
        }
    }

    return RSSL_RET_SUCCESS;
}

// Decodes the service's general information into an RsslServiceGeneralInfo structure.
// Returns success if decoding succeeds or failure if decoding fails.

// serviceGeneralInfo - The service's general information structure
// dIter - The decode iterator
// maxCapabilities - The maximum number of capabilities that the structure holds
// maxQOS - The maximum number of QOS entries that the structure holds
// maxDictionaries - The maximum number of dictionaries that the structure holds
//

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.1

RsslRet UPASourceDirectory::DecodeServiceGeneralInfo(RsslServiceGeneralInfo* serviceGeneralInfo, RsslDecodeIterator* dIter,    RsslUInt32 maxCapabilities, RsslUInt32 maxQOS, RsslUInt32 maxDictionaries)
{
    // decode element list from the message
    RsslRet ret = 0;
    RsslElementList    eltList;
    if ((ret = rsslDecodeElementList(dIter, &eltList, NULL)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
        return ret;
    }

    // then decode each element in turn
    RsslElementEntry elt;

    RsslArray arr = RSSL_INIT_ARRAY;
    RsslBuffer arrBuff;
    int arrayCount = 0;

    while ((ret = rsslDecodeElementEntry(dIter, &elt)) != RSSL_RET_END_OF_CONTAINER)
    {
        if (ret == RSSL_RET_SUCCESS)
        {
            // the service name
            if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_NAME))
            {
                // insert into the response structure, truncate if necessary
                if (elt.encData.length < MaxSourceDirInfoStrLength)
                {
                    strncpy(serviceGeneralInfo->ServiceName, elt.encData.data, elt.encData.length);
                    serviceGeneralInfo->ServiceName[elt.encData.length] = '\0';
                }
                else
                {
                    strncpy(serviceGeneralInfo->ServiceName, elt.encData.data, MaxSourceDirInfoStrLength - 1);
                    serviceGeneralInfo->ServiceName[MaxSourceDirInfoStrLength - 1] = '\0';
                }
            }

            // Vendor name
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_VENDOR))
            {
                // truncate if necessary
                if (elt.encData.length < MaxSourceDirInfoStrLength)
                {
                    strncpy(serviceGeneralInfo->Vendor, elt.encData.data, elt.encData.length);
                    serviceGeneralInfo->Vendor[elt.encData.length] = '\0';
                }
                else
                {
                    strncpy(serviceGeneralInfo->Vendor, elt.encData.data, MaxSourceDirInfoStrLength - 1);
                    serviceGeneralInfo->Vendor[MaxSourceDirInfoStrLength - 1] = '\0';
                }
            }

            // IsSource  flag
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_IS_SOURCE))
            {
                ret = rsslDecodeUInt(dIter, &serviceGeneralInfo->IsSource);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
                    return ret;
                }
            }

            // Capabilities
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_CAPABILITIES))
            {
                if ((ret = rsslDecodeArray(dIter, &arr)) < RSSL_RET_SUCCESS)
                {
                    t42log_error("rsslDecodeArray() failed with return code: %d\n", ret);
                    return ret;
                }
                while ((ret = rsslDecodeArrayEntry(dIter, &arrBuff)) != RSSL_RET_END_OF_CONTAINER)
                {
                    // break out of decoding array items when max capabilities reached
                    if (arrayCount == maxCapabilities)
                    {
                        rsslFinishDecodeEntries(dIter);
                        break;
                    }

                    if (ret == RSSL_RET_SUCCESS)
                    {
                        ret = rsslDecodeUInt(dIter, &serviceGeneralInfo->Capabilities[arrayCount]);
                        if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                        {
                            t42log_error("rsslDecodeUInt() for service capabolities failed with return code: %d\n", ret);
                            return ret;
                        }
                    }
                    else if (ret != RSSL_RET_BLANK_DATA)
                    {
                        t42log_error("rsslDecodeArrayEntry() failed with return code: %d\n", ret);
                        return ret;
                    }
                    arrayCount++;
                }
                arrayCount = 0;
            }

            // Dictionaries provided
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_DICTIONARYS_PROVIDED))
            {
                if ((ret = rsslDecodeArray(dIter, &arr)) < RSSL_RET_SUCCESS)
                {
                    t42log_error("rsslDecodeArray() failed with return code: %d\n", ret);
                    return ret;
                }
                while ((ret = rsslDecodeArrayEntry(dIter, &arrBuff)) != RSSL_RET_END_OF_CONTAINER)
                {
                    // break out of decoding array items when max dictionaries reached
                    if (arrayCount == maxDictionaries)
                    {
                        rsslFinishDecodeEntries(dIter);
                        break;
                    }

                    if (ret == RSSL_RET_SUCCESS)
                    {
                        if (arrBuff.length < MaxSourceDirInfoStrLength)
                        {
                            strncpy(serviceGeneralInfo->DictionariesProvided[arrayCount],
                                arrBuff.data,
                                arrBuff.length);
                            serviceGeneralInfo->DictionariesProvided[arrayCount][arrBuff.length] = '\0';
                        }
                        else
                        {
                            strncpy(serviceGeneralInfo->DictionariesProvided[arrayCount],
                                arrBuff.data,
                                MaxSourceDirInfoStrLength - 1);
                            serviceGeneralInfo->DictionariesProvided[arrayCount][MaxSourceDirInfoStrLength - 1] = '\0';
                        }
                    }
                    else if (ret != RSSL_RET_BLANK_DATA)
                    {
                        t42log_error("rsslDecodeArrayEntry() failed with return code: %d\n", ret);
                        return ret;
                    }
                    arrayCount++;
                }
                arrayCount = 0;
            }

            // DictionariesUsed
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_DICTIONARYS_USED))
            {
                if ((ret = rsslDecodeArray(dIter, &arr)) < RSSL_RET_SUCCESS)
                {
                    t42log_error("rsslDecodeArray() failed with return code: %d\n", ret);
                    return ret;
                }
                while ((ret = rsslDecodeArrayEntry(dIter, &arrBuff)) != RSSL_RET_END_OF_CONTAINER)
                {
                    // break out of decoding array items when max dictionaries reached
                    if (arrayCount == maxDictionaries)
                    {
                        rsslFinishDecodeEntries(dIter);
                        break;
                    }

                    if (ret == RSSL_RET_SUCCESS)
                    {
                        if (arrBuff.length < MaxSourceDirInfoStrLength)
                        {
                            strncpy(serviceGeneralInfo->DictionariesUsed[arrayCount],
                                arrBuff.data,
                                arrBuff.length);
                            serviceGeneralInfo->DictionariesUsed[arrayCount][arrBuff.length] = '\0';
                        }
                        else
                        {
                            strncpy(serviceGeneralInfo->DictionariesUsed[arrayCount],
                                arrBuff.data,
                                MaxSourceDirInfoStrLength - 1);
                            serviceGeneralInfo->DictionariesUsed[arrayCount][MaxSourceDirInfoStrLength - 1] = '\0';
                        }
                    }
                    else if (ret != RSSL_RET_BLANK_DATA)
                    {
                        t42log_error("rsslDecodeArrayEntry() failed with return code: %d\n", ret);
                        return ret;
                    }
                    arrayCount++;
                }
                arrayCount = 0;
            }
            // QoS
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_QOS))
            {
                if ((ret = rsslDecodeArray(dIter, &arr)) < RSSL_RET_SUCCESS)
                {
                    t42log_error("rsslDecodeArray() failed with return code: %d\n", ret);
                    return ret;
                }
                while ((ret = rsslDecodeArrayEntry(dIter, &arrBuff)) != RSSL_RET_END_OF_CONTAINER)
                {
                    // break out of decoding array items when max QOS reached
                    if (arrayCount == maxQOS)
                    {
                        rsslFinishDecodeEntries(dIter);
                        break;
                    }

                    if (ret == RSSL_RET_SUCCESS)
                    {
                        ret = rsslDecodeQos(dIter, &serviceGeneralInfo->QoS[arrayCount]);
                        if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                        {
                            t42log_error("rsslDecodeQos() failed with return code: %d\n", ret);
                            return ret;
                        }
                    }
                    else if (ret != RSSL_RET_BLANK_DATA)
                    {
                        t42log_error("rsslDecodeArrayEntry() failed with return code: %d\n", ret);
                        return ret;
                    }
                    arrayCount++;
                }
                arrayCount = 0;
            }
            // SupportsQosRange
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_SUPPS_QOS_RANGE))
            {
                ret = rsslDecodeUInt(dIter, &serviceGeneralInfo->SupportsQosRange);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
                    return ret;
                }
            }
            // ItemList
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_ITEM_LIST))
            {
                if (elt.encData.length < MaxSourceDirInfoStrLength)
                {
                    strncpy(serviceGeneralInfo->ItemList, elt.encData.data, elt.encData.length);
                    serviceGeneralInfo->ItemList[elt.encData.length] = '\0';
                }
                else
                {
                    strncpy(serviceGeneralInfo->ItemList, elt.encData.data, MaxSourceDirInfoStrLength - 1);
                    serviceGeneralInfo->ItemList[MaxSourceDirInfoStrLength - 1] = '\0';
                }
            }
            // SupportsOutOfBandSnapshots
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_SUPPS_OOB_SNAPSHOTS))
            {
                ret = rsslDecodeUInt(dIter, &serviceGeneralInfo->SupportsOutOfBandSnapshots);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
                    return ret;
                }
            }
            // AcceptingConsumerStatus
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_ACCEPTING_CONS_STATUS))
            {
                ret = rsslDecodeUInt(dIter, &serviceGeneralInfo->AcceptingConsumerStatus);
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

    return RSSL_RET_SUCCESS;
}


//Decodes the service's state information into the RsslServiceStateInfo structure.
//Returns success if decoding succeeds or failure if decoding fails.
//
//serviceStateInfo - The service's state information structure
//dIter - The decode iterator

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.2

RsslRet UPASourceDirectory::DecodeServiceStateInfo(RsslServiceStateInfo* serviceStateInfo,  RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;

    // decode element list
    RsslElementList    eltList;
    if ((ret = rsslDecodeElementList(dIter, &eltList, NULL)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
        return ret;
    }

    /* decode element list elements */
    RsslElementEntry elt;
    while ((ret = rsslDecodeElementEntry(dIter, &elt)) != RSSL_RET_END_OF_CONTAINER)
    {
        if (ret == RSSL_RET_SUCCESS)
        {

            // ServiceState
            if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_SVC_STATE))
            {
                ret = rsslDecodeUInt(dIter, &serviceStateInfo->ServiceState);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() for service state failed with return code: %d\n", ret);
                    return ret;
                }
            }
            // AcceptingRequests
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_ACCEPTING_REQS))
            {
                ret = rsslDecodeUInt(dIter, &serviceStateInfo->AcceptingRequests);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt()for accepting requests failed with return code: %d\n", ret);
                    return ret;
                }
            }
            // Status
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_STATUS))
            {
                ret = rsslDecodeState(dIter, &serviceStateInfo->Status);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt()  for status failed with return code: %d\n", ret);
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

    return RSSL_RET_SUCCESS;
}


//Decodes the service's group information into the RsslServiceGroupInfo structure.
//Returns success if decoding succeeds or failure if decoding fails.

//serviceGroupInfo - The service's group information structure
//dIter - The decode iterator
//

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.3

RsslRet UPASourceDirectory::DecodeServiceGroupInfo(RsslServiceGroupInfo* serviceGroupInfo,  RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;

    // decode element list
    RsslElementList    eltList;
    if ((ret = rsslDecodeElementList(dIter, &eltList, NULL)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
        return ret;
    }

    // extract each of the elements
    RsslElementEntry elt;

    while ((ret = rsslDecodeElementEntry(dIter, &elt)) != RSSL_RET_END_OF_CONTAINER)
    {
        if (ret == RSSL_RET_SUCCESS)
        {
            // Group - truncate of necessary
            if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_GROUP))
            {
                if (elt.encData.length < MaxGroupInfoLength)
                {
                    memcpy(serviceGroupInfo->Group, elt.encData.data, elt.encData.length);
                }
                else
                {
                    memcpy(serviceGroupInfo->Group, elt.encData.data, MaxGroupInfoLength);
                }
            }

            // MergedToGroup
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_MERG_TO_GRP))
            {
                if (elt.encData.length < MaxGroupInfoLength)
                {
                    memcpy(serviceGroupInfo->MergedToGroup, elt.encData.data, elt.encData.length);
                }
                else
                {
                    memcpy(serviceGroupInfo->MergedToGroup, elt.encData.data, MaxGroupInfoLength);
                }
            }

            // Status
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_STATUS))
            {
                ret = rsslDecodeState(dIter, &serviceGroupInfo->Status);
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

    return RSSL_RET_SUCCESS;
}

//
//Decodes the service's load information into the RsslServiceLoadInfo structure.
//Returns success if decoding succeeds or failure if decoding fails.

//serviceLoadInfo - The service's load information structure
//dIter - The decode iterator
//

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.4

RsslRet UPASourceDirectory::DecodeServiceLoadInfo(RsslServiceLoadInfo* serviceLoadInfo, RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;

    // decode element list
    RsslElementList    eltList;
    if ((ret = rsslDecodeElementList(dIter, &eltList, NULL)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
        return ret;
    }

    // extract each element
    RsslElementEntry elt;
    while ((ret = rsslDecodeElementEntry(dIter, &elt)) != RSSL_RET_END_OF_CONTAINER)
    {
        if (ret == RSSL_RET_SUCCESS)
        {
            //OpenLimit
            if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_OPEN_LIMIT))
            {
                ret = rsslDecodeUInt(dIter, &serviceLoadInfo->OpenLimit);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() for OpenLimit failed with return code: %d\n", ret);
                    return ret;
                }
            }

            // OpenWindow
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_OPEN_WINDOW))
            {
                ret = rsslDecodeUInt(dIter, &serviceLoadInfo->OpenWindow);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() for OpenWindow failed with return code: %d\n", ret);
                    return ret;
                }
            }

            //LoadFactor
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_LOAD_FACT))
            {
                ret = rsslDecodeUInt(dIter, &serviceLoadInfo->LoadFactor);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() for LoadFactor failed with return code: %d\n", ret);
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

    return RSSL_RET_SUCCESS;
}

//Decodes the service's data information into the RsslServiceDataInfo structure.
//Returns success if decoding succeeds or failure if decoding fails.
//serviceDataInfo - The service's data information structure
//dIter - The decode iterator
//

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.5


RsslRet UPASourceDirectory::DecodeServiceDataInfo(RsslServiceDataInfo* serviceDataInfo, RsslDecodeIterator* dIter)
{
    RsslRet ret = 0;

    /* decode element list */
    RsslElementList    eltList;
    if ((ret = rsslDecodeElementList(dIter, &eltList, NULL)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
        return ret;
    }

    /* decode element list elements */
    RsslElementEntry elt;
    while ((ret = rsslDecodeElementEntry(dIter, &elt)) != RSSL_RET_END_OF_CONTAINER)
    {
        if (ret == RSSL_RET_SUCCESS)
        {

            // Type
            if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_TYPE))
            {
                ret = rsslDecodeUInt(dIter, &serviceDataInfo->Type);
                if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                {
                    t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
                    return ret;
                }
            }

            //Data
            else if (rsslBufferIsEqual(&elt.name, &RSSL_ENAME_DATA))
            {
                if (elt.encData.length < MaxDataInfoLength)
                {
                    memcpy(serviceDataInfo->Data, elt.encData.data, elt.encData.length);
                }
                else
                {
                    memcpy(serviceDataInfo->Data, elt.encData.data, MaxDataInfoLength);
                }
            }
        }
        else
        {
            t42log_error("rsslDecodeElementEntry() failed with return code: %d\n", ret);
            return ret;
        }
    }

    return RSSL_RET_SUCCESS;
}


//Decodes the service's link information into the RsslServiceLinkInfo structure.
//Returns success if decoding succeeds or failure if decoding fails.

//serviceLinkInfo - The service's link information structure
//dIter - The decode iterator
//maxLinks - The maximum number of link entries that the structure holds

// For more details of the message schema and semantics please refer to the UPA-RDMUsageGuide section 4.3.1.5

RsslRet UPASourceDirectory::DecodeServiceLinkInfo(RsslServiceLinkInfo* serviceLinkInfo, RsslDecodeIterator* dIter, RsslUInt32 maxLinks)
{
    RsslRet ret = 0;
    int linkCount = 0;

    //decode map
    RsslMap map;
    if ((ret = rsslDecodeMap(dIter, &map)) < RSSL_RET_SUCCESS)
    {
        t42log_error("rsslDecodeMap() failed with return code: %d\n", ret);
        return ret;
    }

    // service link key data type must be RSSL_DT_ASCII_STRING
    if (map.keyPrimitiveType != RSSL_DT_ASCII_STRING)
    {
        t42log_error("Decode Service Link Info - Map has wrong keyPrimitiveType: %s", rsslDataTypeToString(map.keyPrimitiveType));
        return RSSL_RET_FAILURE;
    }

    // decode map entries
    // link name is contained in map entry encKey
    RsslMapEntry mapEntry;
    RsslBuffer linkNameBuff;
    while ((ret = rsslDecodeMapEntry(dIter, &mapEntry, &linkNameBuff)) != RSSL_RET_END_OF_CONTAINER)
    {
        // break out of decoding when max services reached
        if (linkCount == maxLinks)
        {
            rsslFinishDecodeEntries(dIter);
            break;
        }


        // store link name in service link information
        if (linkNameBuff.length < MaxSourceDirInfoStrLength)
        {
            strncpy(serviceLinkInfo[linkCount].LinkName, linkNameBuff.data, linkNameBuff.length);
            serviceLinkInfo[linkCount].LinkName[linkNameBuff.length] = '\0';
        }
        else
        {
            strncpy(serviceLinkInfo[linkCount].LinkName, linkNameBuff.data, MaxSourceDirInfoStrLength - 1);
            serviceLinkInfo[linkCount].LinkName[MaxSourceDirInfoStrLength - 1] = '\0';
        }

        if (ret == RSSL_RET_SUCCESS)
        {
            if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeUInt() failed with return code: %d\n", ret);
                return ret;
            }

            //decode element list
            RsslElementList    elementList;

            if ((ret = rsslDecodeElementList(dIter, &elementList, NULL)) < RSSL_RET_SUCCESS)
            {
                t42log_error("rsslDecodeElementList() failed with return code: %d\n", ret);
                return ret;
            }

            // decode element list elements
            RsslElementEntry element;
            while ((ret = rsslDecodeElementEntry(dIter, &element)) != RSSL_RET_END_OF_CONTAINER)
            {
                if (ret == RSSL_RET_SUCCESS)
                {

                    // Type
                    if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_TYPE))
                    {
                        ret = rsslDecodeUInt(dIter, &serviceLinkInfo[linkCount].Type);
                        if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                        {
                            t42log_error("rsslDecodeUInt() for service link info type failed with return code: %d\n", ret);
                            return ret;
                        }
                    }

                    // LinkState
                    else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_LINK_STATE))
                    {
                        ret = rsslDecodeUInt(dIter, &serviceLinkInfo[linkCount].LinkState);
                        if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                        {
                            t42log_error("rsslDecodeUInt() for service link info state failed with return code: %d\n", ret);
                            return ret;
                        }
                    }

                    // LinkCode
                    else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_LINK_CODE))
                    {
                        ret = rsslDecodeUInt(dIter, &serviceLinkInfo[linkCount].LinkCode);
                        if (ret != RSSL_RET_SUCCESS && ret != RSSL_RET_BLANK_DATA)
                        {
                            t42log_error("rsslDecodeUInt() for service link info linkCode failed with return code: %d\n", ret);
                            return ret;
                        }
                    }

                    // Text
                    else if (rsslBufferIsEqual(&element.name, &RSSL_ENAME_TEXT))
                    {
                        if (element.encData.length < MaxSourceDirInfoStrLength)
                        {
                            strncpy(serviceLinkInfo[linkCount].Text, element.encData.data, element.encData.length);
                            serviceLinkInfo[linkCount].Text[element.encData.length] = '\0';
                        }
                        else
                        {
                            strncpy(serviceLinkInfo[linkCount].Text, element.encData.data, MaxSourceDirInfoStrLength - 1);
                            serviceLinkInfo[linkCount].Text[MaxSourceDirInfoStrLength - 1] = '\0';
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
        linkCount++;
    }

    return RSSL_RET_SUCCESS;
}


// send the response to all the listeners
void UPASourceDirectory::NotifyListenersUpdate( RsslSourceDirectoryResponseInfo *pResponseInfo, bool isRefresh )
{
    listener_t::iterator it = listeners_.begin();

    while (it != listeners_.end())
    {
        (*it)->SourceDirectoryUpdate(pResponseInfo, isRefresh);
        it++;
    }
}

// notify all the listeners that the set of source responses is complete
void UPASourceDirectory::NotifyListenersComplete( bool succeeded )
{
    listener_t::iterator it = listeners_.begin();

    while(it != listeners_.end() )
    {
        (*it)->SourceDirectoryRefreshComplete(succeeded);
        it++;
    }
}

// queue a source directory request onto the consumer thread
bool UPASourceDirectory::QueueRequest(mamaQueue Queue)
{
    mama_status status;
    if ((status = mamaQueue_enqueueEvent(Queue,  UPAConsumer::SourceDirectoryRequestCb, (void*) this)) != MAMA_STATUS_OK)
    {
        t42log_error("Failed to enqueue source directory request, status code = %d", status);
        return false;
    }
    return true;
}

// queue a source directory request onto the consumer thread
bool UPASourceDirectory::QueueRequest(mamaQueue Queue, mamaQueueEventCB cb)
{
    mama_status status;
    if ((status = mamaQueue_enqueueEvent(Queue, cb, (void*) this)) != MAMA_STATUS_OK)
    {
        t42log_error("Failed to enqueue source directory request, status code = %d", status);
        return false;
    }
    return true;
}

RsslRet UPASourceDirectory::EncodeSourceDirectoryRequest(RsslBuffer* msgBuf, RsslInt32 streamId)
{
    RsslRet ret = 0;
    RsslRequestMsg msg = RSSL_INIT_REQUEST_MSG;
    RsslEncodeIterator encodeIter;

    /* clear encode iterator */
    rsslClearEncodeIterator(&encodeIter);

    /* set-up message */
    msg.msgBase.msgClass = RSSL_MC_REQUEST;
    msg.msgBase.streamId = streamId;
    msg.msgBase.domainType = RSSL_DMT_SOURCE;
    msg.msgBase.containerType = RSSL_DT_NO_DATA;
    msg.flags = RSSL_RQMF_STREAMING | RSSL_RQMF_HAS_PRIORITY;
    msg.priorityClass = 1;
    msg.priorityCount = 1;


    /* set members in msgKey */
    msg.msgBase.msgKey.flags = RSSL_MKF_HAS_FILTER;
    msg.msgBase.msgKey.filter = RDM_DIRECTORY_SERVICE_INFO_FILTER | \
        RDM_DIRECTORY_SERVICE_STATE_FILTER | \
        RDM_DIRECTORY_SERVICE_GROUP_FILTER | \
        RDM_DIRECTORY_SERVICE_LOAD_FILTER | \
        RDM_DIRECTORY_SERVICE_DATA_FILTER | \
        RDM_DIRECTORY_SERVICE_LINK_FILTER;

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

RsslRet  UPASourceDirectory::EncodeSourceDirectoryClose(RsslBuffer* msgBuf, RsslInt32 streamId)
{
    RsslRet ret = 0;
    RsslCloseMsg msg = RSSL_INIT_CLOSE_MSG;
    RsslEncodeIterator encodeIter;

    /* clear encode iterator */
    rsslClearEncodeIterator(&encodeIter);

    /* set-up message */
    msg.msgBase.msgClass = RSSL_MC_CLOSE;
    msg.msgBase.streamId = streamId;
    msg.msgBase.domainType = RSSL_DMT_SOURCE;
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


void UPASourceDirectory::ResetServiceGeneralInfo(RsslServiceGeneralInfo* serviceGeneralInfo)
{
    int i;

    serviceGeneralInfo->ServiceName[0] = '\0';
    serviceGeneralInfo->Vendor[0] = '\0';
    serviceGeneralInfo->IsSource = 0;
    for (i = 0; i < MaxCapabilities; i++)
    {
        serviceGeneralInfo->Capabilities[i] = 0;
    }
    for (i = 0; i < MaxDictionaries; i++)
    {
        serviceGeneralInfo->DictionariesProvided[i][0] = '\0';
        serviceGeneralInfo->DictionariesUsed[i][0] = '\0';
    }
    for (i = 0; i < MaxQOS; i++)
    {
        serviceGeneralInfo->QoS[i].dynamic = 0;
        serviceGeneralInfo->QoS[i].rate = RSSL_QOS_RATE_UNSPECIFIED;
        serviceGeneralInfo->QoS[i].rateInfo = 0;
        serviceGeneralInfo->QoS[i].timeInfo = 0;
        serviceGeneralInfo->QoS[i].timeliness = RSSL_QOS_TIME_UNSPECIFIED;
    }
    serviceGeneralInfo->SupportsQosRange = 0;
    serviceGeneralInfo->ItemList[0] = '\0';
    serviceGeneralInfo->SupportsOutOfBandSnapshots = 0;
    serviceGeneralInfo->AcceptingConsumerStatus = 0;
}


void UPASourceDirectory::ResetServiceStateInfo(RsslServiceStateInfo* serviceStateInfo)
{
    serviceStateInfo->ServiceState = 0;
    serviceStateInfo->AcceptingRequests = 0;
    rsslClearState(&serviceStateInfo->Status);
}

void UPASourceDirectory::ResetServiceGroupInfo(RsslServiceGroupInfo* serviceGroupInfo)
{
    memset(serviceGroupInfo->Group, 0, sizeof(serviceGroupInfo->Group));
    memset(serviceGroupInfo->MergedToGroup, 0, sizeof(serviceGroupInfo->MergedToGroup));
    rsslClearState(&serviceGroupInfo->Status);
}

void UPASourceDirectory::ResetServiceLoadInfo(RsslServiceLoadInfo* serviceLoadInfo)
{
    serviceLoadInfo->OpenLimit = 0;
    serviceLoadInfo->OpenWindow = 0;
    serviceLoadInfo->LoadFactor = 0;
}

void UPASourceDirectory::ResetServiceDataInfo(RsslServiceDataInfo* serviceDataInfo)
{
    serviceDataInfo->Type = 0;
    memset(serviceDataInfo->Data, 0, sizeof(serviceDataInfo->Data));
}

void UPASourceDirectory::ResetServiceLinkInfo(RsslServiceLinkInfo* serviceLinkInfo)
{
    int i;

    for (i = 0; i < MaxLinks; i++)
    {
        serviceLinkInfo[i].LinkName[0] = '\0';
        serviceLinkInfo[i].Type = 0;
        serviceLinkInfo[i].LinkState = 0;
        serviceLinkInfo[i].LinkCode = 0;
        serviceLinkInfo[i].Text[0] = '\0';
    }
}

// clear a single source directory response info
void UPASourceDirectory::ResetSourceDirRespInfo(RsslSourceDirectoryResponseInfo* srcDirRespInfo)
{

    srcDirRespInfo->StreamId = 0;
    srcDirRespInfo->ServiceId = 0;
    UPASourceDirectory::ResetServiceGeneralInfo(&srcDirRespInfo->ServiceGeneralInfo);
    UPASourceDirectory::ResetServiceStateInfo(&srcDirRespInfo->ServiceStateInfo);
    UPASourceDirectory::ResetServiceGroupInfo(&srcDirRespInfo->ServiceGroupInfo);
    UPASourceDirectory::ResetServiceLoadInfo(&srcDirRespInfo->ServiceLoadInfo);
    UPASourceDirectory::ResetServiceDataInfo(&srcDirRespInfo->ServiceDataInfo);
    UPASourceDirectory::ResetServiceLinkInfo(&srcDirRespInfo->ServiceLinkInfo[0]);
}

RsslRet UPASourceDirectory::SendSrcDirectoryRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslSrcDirRequestRejectReason reason)
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

    // get a buffer for the source directory request reject status
    msgBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf != NULL)
    {
        // encode source directory request reject status
        if (EncodeSrcDirectoryRequestReject(chnl, streamId, reason, msgBuf) != RSSL_RET_SUCCESS)
        {
            rsslReleaseBuffer(msgBuf, &error);
            t42log_warn("UPASourceDirectory::SendSrcDirectoryRequestReject - encodeSrcDirectoryRequestReject() failed\n");
            return RSSL_RET_FAILURE;
        }

        // send request reject status
        if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
            return RSSL_RET_FAILURE;
    }
    else
    {
        t42log_warn("UPASourceDirectory::SendSrcDirectoryRequestReject - rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;

}

RsslRet UPASourceDirectory::EncodeSrcDirectoryRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslSrcDirRequestRejectReason reason, RsslBuffer* msgBuf)
{

    RsslRet ret = 0;
    RsslStatusMsg msg = RSSL_INIT_STATUS_MSG;
    char stateText[MaxSourceDirInfoStrLength];
    RsslEncodeIterator encodeIter;

    // clear encode iterator
    rsslClearEncodeIterator(&encodeIter);

    // init message
    msg.msgBase.msgClass = RSSL_MC_STATUS;
    msg.msgBase.streamId = streamId;
    msg.msgBase.domainType = RSSL_DMT_SOURCE;
    msg.msgBase.containerType = RSSL_DT_NO_DATA;
    msg.flags = RSSL_STMF_HAS_STATE;
    msg.state.streamState = RSSL_STREAM_CLOSED_RECOVER;
    msg.state.dataState = RSSL_DATA_SUSPECT;
    switch(reason)
    {
    case MaxSrcdirRequestsReached:
        msg.state.code = RSSL_SC_TOO_MANY_ITEMS;
        sprintf(stateText, "Source directory request rejected for stream id %d - max request count reached", streamId);
        msg.state.text.data = stateText;
        msg.state.text.length = (RsslUInt32)strlen(stateText) + 1;
        break;
    case IncorrectFilterFlags:
        msg.state.code = RSSL_SC_USAGE_ERROR;
        sprintf(stateText, "Source directory request rejected for stream id %d - request must minimally have RDM_DIRECTORY_SERVICE_INFO_FILTER, RDM_DIRECTORY_SERVICE_STATE_FILTER, and RDM_DIRECTORY_SERVICE_GROUP_FILTER filter flags", streamId);
        msg.state.text.data = stateText;
        msg.state.text.length = (RsslUInt32)strlen(stateText) + 1;
        break;
    default:
        break;
    }

    /* encode message */
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSrcDirectoryRequestReject - rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
        return ret;
    }
    rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
    if ((ret = rsslEncodeMsg(&encodeIter, (RsslMsg*)&msg)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSrcDirectoryRequestReject - rsslEncodeMsg() failed with return code: %d\n", ret);
        return ret;
    }

    msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);
    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::SendSourceDirectoryResponse(RsslChannel *chnl, RsslInt32 streamId,
                                                        RsslMsgKey* msgKey, RMDSPublisherSource *source, bool solicited)
{
    RsslError error;
    RsslBuffer* msgBuf = 0;

    // get a buffer for the source directory response
    msgBuf = rsslGetBuffer(chnl, MAX_MSG_SIZE, RSSL_FALSE, &error);

    if (msgBuf == 0)
    {
        t42log_warn("UPASourceDirectory::SendSourceDirectoryResponse - rsslGetBuffer(): Failed <%s>\n", error.text);
        return RSSL_RET_FAILURE;
    }

    // set refresh flags
    RsslUInt16 refreshFlags = RSSL_RFMF_HAS_MSG_KEY |  RSSL_RFMF_REFRESH_COMPLETE | RSSL_RFMF_CLEAR_CACHE;
    if (solicited)
    {
        refreshFlags |= RSSL_RFMF_SOLICITED;
    }

    t42log_debug("Building SourceDirectory response\n");

    if (EncodeSourceDirectoryResponse(chnl, source, streamId, msgKey, msgBuf, refreshFlags) != RSSL_RET_SUCCESS)
    {
        rsslReleaseBuffer(msgBuf, &error);
        t42log_warn("UPASourceDirectory::SendSourceDirectoryResponse - EncodeSourceDirectoryResponse() failed\n");
        return RSSL_RET_FAILURE;
    }

    t42log_debug("Sending SourceDirectory response message \n");
    // send  response  message
    if (SendUPAMessage(chnl, msgBuf) != RSSL_RET_SUCCESS)
    {
        return RSSL_RET_FAILURE;
    }

    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::EncodeSourceDirectoryResponse(RsslChannel* chnl, RMDSPublisherSource *source,
    RsslInt32 streamId, RsslMsgKey* requestKey, RsslBuffer* msgBuf, RsslUInt16 refreshFlags)
{
    RsslRet ret = 0;

    // set up the message
    RsslRefreshMsg msg = RSSL_INIT_REFRESH_MSG;

    msg.msgBase.msgClass = RSSL_MC_REFRESH;
    msg.msgBase.domainType = RSSL_DMT_SOURCE;
    msg.msgBase.containerType = RSSL_DT_MAP;
    msg.flags = refreshFlags;
    msg.state.streamState = RSSL_STREAM_OPEN;
    msg.state.dataState = RSSL_DATA_OK;
    msg.state.code = RSSL_SC_NONE;
    char stateText[MaxSourceDirInfoStrLength];
    sprintf(stateText, "Source Directory Refresh Completed");
    msg.state.text.data = stateText;
    msg.state.text.length = (RsslUInt32)strlen(stateText);

    // set filters and stream id from request
    msg.msgBase.msgKey.flags = RSSL_MKF_HAS_FILTER;
    msg.msgBase.msgKey.filter = requestKey->filter;
    msg.msgBase.streamId = streamId;

    // set up the encode iterator
    RsslEncodeIterator encodeIter;
    rsslClearEncodeIterator(&encodeIter);

    // attach it to the message
    if ((ret = rsslSetEncodeIteratorBuffer(&encodeIter, msgBuf)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - rsslSetEncodeIteratorBuffer() failed with return code: %d\n", ret);
        return ret;
    }
    // and set the version
    rsslSetEncodeIteratorRWFVersion(&encodeIter, chnl->majorVersion, chnl->minorVersion);
    if ((ret = rsslEncodeMsgInit(&encodeIter, (RsslMsg*)&msg, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - rsslEncodeMsgInit() failed with return code: %d\n", ret);
        return ret;
    }

    // the set of source directory infos are encoded as a map
    // encode map
    RsslMap map = RSSL_INIT_MAP;
    map.keyPrimitiveType = RSSL_DT_UINT;
    map.containerType = RSSL_DT_FILTER_LIST;
    if ((ret = rsslEncodeMapInit(&encodeIter, &map, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - rsslEncodeMapInit() failed with return code: %d\n", ret);
        return ret;
    }

    // check if service id and/or service name is present in request key
    // if present, they must match those for provider  And, if not just ignore this one

    RsslBuffer tempBuffer;
    tempBuffer.data = const_cast<char *>(source->Name().c_str());  // data member of rssl buffer is not const
    tempBuffer.length = (RsslUInt32)source->Name().size();
    RsslUInt64 serviceId = source->ServiceId();

    t42log_info("Adding source '%s' - service id %d to source directory response message\n", source->Name().c_str(), source->ServiceId());

    if ((!(requestKey->flags & RSSL_MKF_HAS_SERVICE_ID) && !(requestKey->flags & RSSL_MKF_HAS_NAME)) ||
        ((requestKey->flags & RSSL_MKF_HAS_SERVICE_ID) && (requestKey->serviceId == serviceId) && !(requestKey->flags & RSSL_MKF_HAS_NAME)) ||
        ((requestKey->flags & RSSL_MKF_HAS_NAME) && rsslBufferIsEqual(&requestKey->name, &tempBuffer) && !(requestKey->flags & RSSL_MKF_HAS_SERVICE_ID)) ||
        (((requestKey->flags & RSSL_MKF_HAS_SERVICE_ID) && (requestKey->serviceId == serviceId) &&
        (requestKey->flags & RSSL_MKF_HAS_NAME) && rsslBufferIsEqual(&requestKey->name, &tempBuffer))))
    {
        if ( ret = EncodeSourceDirectoryInfo(chnl, source, requestKey, msgBuf, &encodeIter) < RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - EncodeSourceDirectoryInfo() failed with return code: %d\n", ret);
            return ret;
        }
    }
    else
    {
        t42log_info("UPASourceDirectory::EncodeSourceDirectoryResponse - Service id %llu ('%s') not in request - skipping response\n", serviceId, source->Name().c_str());
    }

    // complete encode map
    if ((ret = rsslEncodeMapComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - rsslEncodeMapComplete() failed with return code: %d\n", ret);
        return ret;
    }

    // complete encode message
    if ((ret = rsslEncodeMsgComplete(&encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryResponse - rsslEncodeMsgComplete() failed with return code: %d\n", ret);
        return ret;
    }
    msgBuf->length = rsslGetEncodedBufferLength(&encodeIter);
    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::EncodeSourceDirectoryInfo(RsslChannel *chnl, RMDSPublisherSource *srcInfo,
                                                      RsslMsgKey *requestKey, RsslBuffer *msgBuf, RsslEncodeIterator *encodeIter)
{
    RsslRet ret = 0;
    // encode a map entry
    RsslMapEntry mEntry = RSSL_INIT_MAP_ENTRY;
    mEntry.action = RSSL_MPEA_ADD_ENTRY;
    RsslUInt64 serviceId = srcInfo->ServiceId();
    if ((ret = rsslEncodeMapEntryInit(encodeIter, &mEntry, &serviceId, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - rsslEncodeMapEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // encode filter list
    // this is the list of elements in ther map item - each of the sub-structures in the service info

    RsslFilterList rsslFilterList = RSSL_INIT_FILTER_LIST;
    rsslFilterList.containerType = RSSL_DT_ELEMENT_LIST;
    if ((ret = rsslEncodeFilterListInit(encodeIter, &rsslFilterList)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - rsslEncodeFilterListInit() failed with return code: %d\n", ret);
        return ret;
    }


    // now we can encode each of the filter list items
    //

    /* encode filter list items */
    if (requestKey->filter & RDM_DIRECTORY_SERVICE_INFO_FILTER)
    {
        if ((ret = EncodeServiceGeneralInfo(srcInfo, encodeIter)) != RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - EncodeServiceGeneralInfo() failed with return code: %d\n", ret);
            return ret;
        }
    }
    if (requestKey->filter & RDM_DIRECTORY_SERVICE_STATE_FILTER)
    {
        if ((ret = EncodeServiceStateInfo(srcInfo, encodeIter)) != RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - EncodeServiceStateInfo() failed with return code: %d\n", ret);
            return ret;
        }
    }

    if (requestKey->filter & RDM_DIRECTORY_SERVICE_LOAD_FILTER)
    {
        if ((ret = EncodeServiceLoadInfo(srcInfo, encodeIter)) != RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - EncodeServiceLoadInfo() failed with return code: %d\n", ret);
            return ret;
        }
    }

    if (requestKey->filter & RDM_DIRECTORY_SERVICE_LINK_FILTER)
    {
        if ((ret = EncodeServiceLinkInfo(srcInfo, encodeIter)) != RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - EncodeServiceLinkInfo() failed with return code: %d\n", ret);
            return ret;
        }
    }

    /* complete encode filter list */
    if ((ret = rsslEncodeFilterListComplete(encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - rsslEncodeFilterListComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode map entry */
    if ((ret = rsslEncodeMapEntryComplete(encodeIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeSourceDirectoryInfo - rsslEncodeMapEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    return RSSL_RET_SUCCESS;
}


RsslRet UPASourceDirectory::EncodeServiceGeneralInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter)
{
    RsslRet ret = 0;

    // encode filter list item
    RsslFilterEntry filterListItem = RSSL_INIT_FILTER_ENTRY;
    filterListItem.id = RDM_DIRECTORY_SERVICE_INFO_ID;
    filterListItem.action = RSSL_FTEA_SET_ENTRY;
    if ((ret = rsslEncodeFilterEntryInit(eIter, &filterListItem, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeFilterEntryInit() failed with return code: %d\n", ret);
        return ret;
    }

    // encode the element list
    RsslElementList    elementList = RSSL_INIT_ELEMENT_LIST;
    elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
    if ((ret = rsslEncodeElementListInit(eIter, &elementList, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementListInit() failed with return code: %d\n", ret);
        return ret;
    }

    // ServiceName
    RsslBuffer tempBuffer;
    RsslElementEntry element = RSSL_INIT_ELEMENT_ENTRY;
    char temp[256];
    ::strcpy(temp, srcInfo->Name().c_str());
    //    tempBuffer.data = const_cast<char*>(srcInfo->Name().c_str());
    tempBuffer.data = temp;
    tempBuffer.length = (RsslUInt32)srcInfo->Name().size();
    element.dataType = RSSL_DT_ASCII_STRING;
    element.name = RSSL_ENAME_NAME;
    if ((ret = rsslEncodeElementEntry(eIter, &element, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

     //Capabilities 
    element.dataType = RSSL_DT_ARRAY;
    element.name = RSSL_ENAME_CAPABILITIES;
    if ((ret = rsslEncodeElementEntryInit(eIter, &element, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryInit() failed with return code: %d\n", ret);
        return ret;
    }

    RsslArray rsslArray = RSSL_INIT_ARRAY;
    rsslClearArray(&rsslArray);
    rsslArray.itemLength = 1;
    rsslArray.primitiveType = RSSL_DT_UINT;
    if ((ret = rsslEncodeArrayInit(eIter, &rsslArray)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayInit() failed with return code: %d\n", ret);
        return ret;
    }

    for (size_t i = 0; i < srcInfo->GetMaxCapabilities() && srcInfo->GetCapability((int) i) != 0; i++)
    {
        RsslUInt64 cap = srcInfo->GetCapability((int)i);
        if ((ret = rsslEncodeArrayEntry(eIter, 0, &cap)) < RSSL_RET_SUCCESS)
        {
            t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
            return ret;
        }
    }


    if ((ret = rsslEncodeArrayComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayComplete() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeElementEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }


    // DictionariesProvided
    // this is just a 2 element array with the field file and the enum file
    // although the schema allows for an array of arbitrary size it seems unlikely that there will ever be anything other than fields file and enu7m types. RMDS will not cope well
    // with anything else
    element.dataType = RSSL_DT_ARRAY;
    element.name = RSSL_ENAME_DICTIONARYS_PROVIDED;
    if ((ret = rsslEncodeElementEntryInit(eIter, &element, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryInit() failed with return code: %d\n", ret);
        return ret;
    }
    rsslClearArray(&rsslArray);
    rsslArray.itemLength = 0;
    rsslArray.primitiveType = RSSL_DT_ASCII_STRING;
    if ((ret = rsslEncodeArrayInit(eIter, &rsslArray)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayInit() failed with return code: %d\n", ret);
        return ret;
    }
    ::strcpy(temp, srcInfo->FieldDictionaryName().c_str());
    tempBuffer.data = temp;
    tempBuffer.length = (RsslUInt32)strlen(temp);
    if ((ret = rsslEncodeArrayEntry(eIter, 0, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
        return ret;
    }
    ::strcpy(temp, srcInfo->EnumTypeDictionaryName().c_str());
    tempBuffer.data = (char*)temp;
    tempBuffer.length = (RsslUInt32)strlen(temp);
    if ((ret = rsslEncodeArrayEntry(eIter, 0, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeArrayComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayComplete() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeElementEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    // DictionariesUsed
    element.dataType = RSSL_DT_ARRAY;
    element.name = RSSL_ENAME_DICTIONARYS_USED;
    if ((ret = rsslEncodeElementEntryInit(eIter, &element, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryInit() failed with return code: %d\n", ret);
        return ret;
    }
    rsslClearArray(&rsslArray);
    rsslArray.itemLength = 0;
    rsslArray.primitiveType = RSSL_DT_ASCII_STRING;
    if ((ret = rsslEncodeArrayInit(eIter, &rsslArray)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayInit() failed with return code: %d\n", ret);
        return ret;
    }
    ::strcpy(temp, srcInfo->FieldDictionaryName().c_str());
    tempBuffer.data = (char*)temp;
    tempBuffer.length = (RsslUInt32)strlen(temp);
    if ((ret = rsslEncodeArrayEntry(eIter, 0, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
        return ret;
    }
    ::strcpy(temp, srcInfo->EnumTypeDictionaryName().c_str());
    tempBuffer.data = (char*)temp;
    tempBuffer.length = (RsslUInt32)strlen(temp);
    if ((ret = rsslEncodeArrayEntry(eIter, 0, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeArrayComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayComplete() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeElementEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }
    // QoS
    //

    // although the schema allows for an array we only support one
    element.dataType = RSSL_DT_ARRAY;
    element.name = RSSL_ENAME_QOS;
    if ((ret = rsslEncodeElementEntryInit(eIter, &element, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryInit() failed with return code: %d\n", ret);
        return ret;
    }
    rsslClearArray(&rsslArray);
    rsslArray.itemLength = 0;
    rsslArray.primitiveType = RSSL_DT_QOS;
    if ((ret = rsslEncodeArrayInit(eIter, &rsslArray)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayInit() failed with return code: %d\n", ret);
        return ret;
    }
    RsslQos q = srcInfo->Qos();
    if ((ret = rsslEncodeArrayEntry(eIter, 0, &q)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayEntry() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeArrayComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeArrayComplete() failed with return code: %d\n", ret);
        return ret;
    }
    if ((ret = rsslEncodeElementEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }


    // SupportsOutOfBandSnapshots
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_SUPPS_OOB_SNAPSHOTS;
    RsslUInt64 oobss = srcInfo->SupportOutOfBandSnapshots();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &oobss)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // AcceptingConsumerStatus
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_ACCEPTING_CONS_STATUS;
    RsslUInt64 acs = srcInfo->AcceptConsumerStatus();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &acs)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode element list */
    if ((ret = rsslEncodeElementListComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeElementListComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode filter list item */
    if ((ret = rsslEncodeFilterEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceGeneralInfo - rsslEncodeFilterEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::EncodeServiceStateInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter )
{
    RsslRet ret = 0;

    // encode  the filter list item
    RsslFilterEntry filterListItem = RSSL_INIT_FILTER_ENTRY;
    filterListItem.id = RDM_DIRECTORY_SERVICE_STATE_ID;
    filterListItem.action = RSSL_FTEA_SET_ENTRY;
    if ((ret = rsslEncodeFilterEntryInit(eIter, &filterListItem, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo - rsslEncodeFilterEntryInit() failed with return code: %d\n", ret);
        return ret;
    }

    // encode the element list
    RsslElementList    elementList = RSSL_INIT_ELEMENT_LIST;
    elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
    if ((ret = rsslEncodeElementListInit(eIter, &elementList, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeElementListInit() failed with return code: %d\n", ret);
        return ret;
    }

    // ServiceState
    RsslElementEntry element = RSSL_INIT_ELEMENT_ENTRY;
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_SVC_STATE;
    RsslUInt64 serviceState =  srcInfo->ServiceState();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &serviceState)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // AcceptingRequests
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_ACCEPTING_REQS;
    RsslUInt64 acceptingRequests = srcInfo->AcceptingRequests();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &acceptingRequests)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }
    // Status
    element.dataType = RSSL_DT_STATE;
    element.name = RSSL_ENAME_STATUS;
    RsslState status = srcInfo->Status();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &status)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // complete encode element list
    if ((ret = rsslEncodeElementListComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeElementListComplete() failed with return code: %d\n", ret);
        return ret;
    }

    // complete encode filter list item
    if ((ret = rsslEncodeFilterEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceStateInfo -rsslEncodeFilterEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::EncodeServiceLoadInfo( RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter )
{
    RsslRet ret = 0;

    // encode filter list item
    RsslFilterEntry filterListItem = RSSL_INIT_FILTER_ENTRY;
    filterListItem.id = RDM_DIRECTORY_SERVICE_LOAD_ID;
    filterListItem.action = RSSL_FTEA_SET_ENTRY;
    if ((ret = rsslEncodeFilterEntryInit(eIter, &filterListItem, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo - rsslEncodeFilterEntryInit() failed with return code: %d\n", ret);
        return ret;
    }

    // encode the element list
    RsslElementList    elementList = RSSL_INIT_ELEMENT_LIST;
    elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
    if ((ret = rsslEncodeElementListInit(eIter, &elementList, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo - rsslEncodeElementListInit() failed with return code: %d\n", ret);
        return ret;
    }

    /// OpenLimit
    RsslElementEntry element = RSSL_INIT_ELEMENT_ENTRY;
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_OPEN_LIMIT;
    RsslUInt64 openLimit = srcInfo->OpenLimit();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &openLimit)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    //OpenWindow
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_OPEN_WINDOW;
    RsslUInt64 openWindow = srcInfo->OpenWindow();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &openWindow)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // LoadFactor
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_LOAD_FACT;
    RsslUInt64 loadFactor = srcInfo->LoadFactor();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &loadFactor)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo -rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode element list */
    if ((ret = rsslEncodeElementListComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo -rsslEncodeElementListComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode filter list item */
    if ((ret = rsslEncodeFilterEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLoadInfo -rsslEncodeFilterEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    return RSSL_RET_SUCCESS;
}

RsslRet UPASourceDirectory::EncodeServiceLinkInfo(RMDSPublisherSource * srcInfo, RsslEncodeIterator* eIter)
{
    RsslRet ret = 0;

    RsslElementEntry element = RSSL_INIT_ELEMENT_ENTRY;
    RsslElementList    elementList = RSSL_INIT_ELEMENT_LIST;
    RsslBuffer tempBuffer;

    // encode filter list item
    RsslFilterEntry filterListItem = RSSL_INIT_FILTER_ENTRY;
    filterListItem.id = RDM_DIRECTORY_SERVICE_LINK_ID;
    filterListItem.action = RSSL_FTEA_SET_ENTRY;
    filterListItem.flags = RSSL_FTEF_HAS_CONTAINER_TYPE;
    filterListItem.containerType = RSSL_DT_MAP;
    if ((ret = rsslEncodeFilterEntryInit(eIter, &filterListItem, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeFilterEntryInit() failed with return code: %d\n", ret);
        return ret;
    }

    // encode map
    RsslMap map = RSSL_INIT_MAP;
    map.keyPrimitiveType = RSSL_DT_ASCII_STRING;
    map.containerType = RSSL_DT_ELEMENT_LIST;
    if ((ret = rsslEncodeMapInit(eIter, &map, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeMapInit() failed with return code: %d\n", ret);
        return ret;
    }

    // encode map entry
    RsslMapEntry mEntry = RSSL_INIT_MAP_ENTRY;
    mEntry.action = RSSL_MPEA_ADD_ENTRY;
    char tmp[256];
    strcpy(tmp, srcInfo->LinkName().c_str());
    tempBuffer.data = tmp;
    tempBuffer.length = (RsslUInt32)strlen(tmp);
    if ((ret = rsslEncodeMapEntryInit(eIter, &mEntry, &tempBuffer, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeMapEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // encode the element list for this map entry
    elementList.flags = RSSL_ELF_HAS_STANDARD_DATA;
    if ((ret = rsslEncodeElementListInit(eIter, &elementList, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementListInit() failed with return code: %d\n", ret);
        return ret;
    }

    // Type
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_TYPE;
    RsslUInt64 linkType = srcInfo->LinkType();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &linkType)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    // LinkState
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_LINK_STATE;
    RsslUInt64 linkState = srcInfo->LinkState();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &linkState)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }
    /* LinkCode */
    element.dataType = RSSL_DT_UINT;
    element.name = RSSL_ENAME_LINK_CODE;
    RsslUInt64 linkCode = srcInfo->LinkCode();
    if ((ret = rsslEncodeElementEntry(eIter, &element, &linkCode)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }
    /* Text */
    element.dataType = RSSL_DT_ASCII_STRING;
    element.name = RSSL_ENAME_TEXT;
    strcpy(tmp, srcInfo->LinkText().c_str());
    tempBuffer.data = tmp;
    tempBuffer.length = (RsslUInt32)strlen(tmp);
    if ((ret = rsslEncodeElementEntry(eIter, &element, &tempBuffer)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementEntry() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode element list */
    if ((ret = rsslEncodeElementListComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeElementListComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode map entry */
    if ((ret = rsslEncodeMapEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeMapEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode map */
    if ((ret = rsslEncodeMapComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeMapComplete() failed with return code: %d\n", ret);
        return ret;
    }

    /* complete encode filter list item */
    if ((ret = rsslEncodeFilterEntryComplete(eIter, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("UPASourceDirectory::EncodeServiceLinkInfo - rsslEncodeFilterEntryComplete() failed with return code: %d\n", ret);
        return ret;
    }

    return RSSL_RET_SUCCESS;
}
