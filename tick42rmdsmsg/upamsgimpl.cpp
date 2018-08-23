/*
* UPAMsgImpl: The Reuters RMDS Bridge for OpenMama
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
#include "../tick42rmds/version.h"

#include "upamsgimpl.h"
#include "upapayload.h"

#define upaPayload(msg) ((UpaPayload*)msg)

//
// There are a whole bunch of interesting issues associate with data dictionaries in OM !
// One that we need to deal with here is that the set of fields delivered by bloomberg is disjoint to the set of fields defined in the wombat dictionary.
// Mostly this doesnt matter and we can just deliver bloomberg fields by name but there is set of fields whose values are requested by fid.
// So in the short term we work around this with a small hardcoded dictionary to lookup these fields

typedef utils::collection::unordered_map<int, std::string> fid2nameMap_t;

static fid2nameMap_t fid2names;

static bool fidMapIsInit = false;

void InitFidMap()
{
    if (fidMapIsInit)
    { 
        // no need to do it more than once
        return;
    }

    // announce the version that we are running
    mama_log(MAMA_LOG_LEVEL_NORMAL
#ifdef ENABLE_TICK42_ENHANCED
        , "******* Initializing payload on Tick42-enhanced RMDS bridge %s : version %s *******\n"
#else
        , "******* Initializing payload on Tick42 RMDS bridge %s : version %s *******\n"
#endif // ENABLE_TICK42_ENHANCED
        , BRIDGE_NAME_STRING, BRIDGE_VERSION_STRING);


    fid2names.emplace(1 , "MdMsgType");
    fid2names.emplace(2 , "MdMsgStatus");
    fid2names.emplace(7 , "MdMsgNum");
    fid2names.emplace(8 , "MdMsgTotal");
    fid2names.emplace(10 , "MdSeqNum");
    fid2names.emplace(11 , "MdFeedName");
    fid2names.emplace(12 , "MdFeedHost");
    fid2names.emplace(13 , "MdFeedGroup");
    fid2names.emplace(15 , "MdItemSeq");
    fid2names.emplace(16 , "MamaSendTime");
    fid2names.emplace(17 , "MamaAppDataType");
    fid2names.emplace(18 , "MamaAppMsgType");
    fid2names.emplace(20 , "MamaSenderId");
    fid2names.emplace(21 , "wMsgQual");
    fid2names.emplace(22 , "wConflateCount");
    fid2names.emplace(23 , "wConflateQuoteCount");
    fid2names.emplace(24 , "wConflateTradeCount");
    fid2names.emplace(305 , "wIssueSymbol");

    fidMapIsInit = true;
}

namespace {
    static const char empty_buf[] = "";
}

mama_status
    upaMsg_setBool(msgPayload      msg,
    const char*     name,
    mama_fid_t      fid,
    mama_bool_t     value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value ? true : false);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setChar(msgPayload      msg,
    const char*     name,
    mama_fid_t      fid,
    char            value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setI8(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    int8_t          value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setU8(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    uint8_t         value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setI16(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    int16_t         value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;

}

mama_status
    upaMsg_setU16(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    uint16_t        value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setI32(msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    int32_t             value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setU32(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    uint32_t        value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setI64(msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    int64_t             value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setU64(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    uint64_t        value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setF32(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    mama_f32_t      value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setF64(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    mama_f64_t   value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setString(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    const char*  value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setOpaque(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    const void*  value,
    size_t       size)
{
    if (!name) 
        name = empty_buf;

    MamaOpaqueWrapper_ptr_t wrapper = MamaOpaqueWrapper_ptr_t(new MamaOpaqueWrapper(value, size));
    upaPayload(msg)->setOpaque(fid, name, wrapper);

    return MAMA_STATUS_OK;

}

mama_status
    upaMsg_setVectorBool (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const mama_bool_t  value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorChar (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const char         value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorI8 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const int8_t       value[],
    size_t             numElements)
{
    if (!name)
    {
        name = empty_buf;
    }
    upaPayload(msg)->setVector<int8_t>(fid, name, value, numElements);
    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setVectorU8 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const uint8_t      value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorI16 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const int16_t      value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorU16 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const uint16_t     value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorI32 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const int32_t      value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorU32 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const uint32_t     value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorI64 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const int64_t      value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorU64 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const uint64_t     value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorF32 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const mama_f32_t   value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setVectorF64 (
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const mama_f64_t   value[],
    size_t             numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////
//
mama_status
    upaMsg_setVectorString (
    msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    const char*  value[],
    size_t       numElements)
{
    if (!name)
    {
        name = empty_buf;
    }
    upaPayload(msg)->setVectorString(fid, name, value, numElements);
    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setVectorDateTime (
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value[],
    size_t              numElements)
{
    /*if (!name) name = empty_buf; */ //uncomment that line, once the function is implemented
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setMsg(
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const msgPayload   value)
{

    if (!name) 
        name = empty_buf;

    MamaMsgPayloadWrapper_ptr_t wrapper(new MamaMsgPayloadWrapper(value));
    upaPayload(msg)->setMsg(fid, name, wrapper);

    return MAMA_STATUS_OK;
    //return MAMA_STATUS_NOT_IMPLEMENTED;
}


extern "C"
{
    mama_status
        mamaMsgImpl_getPayload (const mamaMsg msg, msgPayload* payload);
};


mama_status
    upaMsg_setVectorMsg(
    msgPayload     msg,
    const char*        name,
    mama_fid_t         fid,
    const mamaMsg      value[],
    size_t             numElements)
{

    if (!name) 
        name = empty_buf;


    // now what we actually need to put into the stored message vector is msgPayloads, not the messages themselves
    // Open mama is a bit inconsistent here

    MamaMsgVectorWrapper_ptr_t wrapper(new MamaMsgVectorWrapper());
    for(size_t index = 0; index < numElements; index ++)
    {
        msgPayload payload;
        mamaMsgImpl_getPayload(value[index],  &payload);

        //        MamaMsgPayloadWrapper_ptr_t p = boost::shared_ptr<MamaMsgPayloadWrapper>(new MamaMsgPayloadWrapper(payload));
        wrapper->addMessage(payload);
    }

    upaPayload(msg)->setVectorMsg(fid, name, wrapper);
    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_getBool(msgPayload     msg,
    const char*   name,
    mama_fid_t    fid,
    mama_bool_t*  result)
{
    return upaPayload(msg)->get(fid, name, (bool&)*result);
}

mama_status
    upaMsg_getChar(msgPayload     msg,
    const char*   name,
    mama_fid_t    fid,
    char*         result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getI8(msgPayload     msg,
    const char*   name,
    mama_fid_t    fid,
    int8_t*       result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getU8(msgPayload     msg,
    const char*   name,
    mama_fid_t    fid,
    uint8_t*      result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getI16(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    int16_t*        result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getU16(msgPayload     msg,
    const char*     name,
    mama_fid_t      fid,
    uint16_t*       result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getI32(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    int32_t*     result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getU32(msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    uint32_t*      result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getI64(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    int64_t*     result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getU64(msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    uint64_t*      result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getF32(msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    mama_f32_t*    result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getF64(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    mama_f64_t*  result)
{
    return upaPayload(msg)->get(fid, name, *result);
}

mama_status
    upaMsg_getString(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    const char** result)
{
    return upaPayload(msg)->get(fid, name, result);
}


mama_status
    upaMsg_getOpaque(
    msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    const void**   result,
    size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getDateTime(
    msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    mamaDateTime   result)
{
    return upaPayload(msg)->getDateTime(fid, name, result);
}

mama_status
    upaMsg_setDateTime(
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    const MamaDateTimeWrapper_ptr_t&  value)
{

    if (!name) 
        name = empty_buf;

    upaPayload(msg)->set(fid, name, value);
    return MAMA_STATUS_OK;
}


mama_status
    upaMsg_getVectorDateTime (
    msgPayload     msg,
    const char*           name,
    mama_fid_t            fid,
    const mamaDateTime*   result,
    size_t*               resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorPrice (
    msgPayload     msg,
    const char*           name,
    mama_fid_t            fid,
    const mamaPrice*      result,
    size_t*               resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_setPrice(
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    const MamaPriceWrapper_ptr_t&     value)
{
    if (!name) 
        name = empty_buf;

    upaPayload(msg)->setPrice(fid, name, value);

    return MAMA_STATUS_OK;
}

mama_status
    upaMsg_setVectorPrice (
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice    value[],
    size_t              numElements)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getPrice(
    msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    mamaPrice      result)
{
    return upaPayload(msg)->getPrice(fid, name, result);
}

mama_status
    upaMsg_getMsg (
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    msgPayload*         result)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorBool (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const mama_bool_t**  result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorChar (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const char**         result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorI8 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const int8_t**       result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorU8 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const uint8_t**      result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorI16 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const int16_t**      result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorU16 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const uint16_t**     result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorI32 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const int32_t**      result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorU32 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const uint32_t**     result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorI64 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const int64_t**      result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorU64 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const uint64_t**     result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorF32 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const mama_f32_t**   result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorF64 (
    msgPayload     msg,
    const char*          name,
    mama_fid_t           fid,
    const mama_f64_t**   result,
    size_t*              resultLen)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    upaMsg_getVectorString (
    msgPayload     msg,
    const char*    name,
    mama_fid_t     fid,
    const char***  result,
    size_t*        resultLen)
{
    return upaPayload(msg)->get(fid, name, result, resultLen);
}

mama_status
    upaMsg_getVectorMsg (
    msgPayload     msg,
    const char*         name,
    mama_fid_t          fid,
    const msgPayload**  result,
    size_t*             resultLen)
{
    return upaPayload(msg)->get(fid, name, result, resultLen);
}

mama_status
    upaMsg_getFieldAsString(msgPayload     msg,
    const char*  name,
    mama_fid_t   fid,
    char*        buf,
    size_t       len)
{
    std::string value;
    mama_status ret = upaPayload(msg)->getAsString(fid, name, value);

    if (ret == MAMA_STATUS_OK)
    {
        strncpy(buf, value.c_str(), value.size());
        buf[value.length()] = 0;
    }

    return ret;
}

