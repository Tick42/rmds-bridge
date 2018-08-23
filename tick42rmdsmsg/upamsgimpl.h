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

#ifndef UPAMSGIMPL_H
#define UPAMSGIMPL_H

#include "bridge.h"
#include "payloadbridge.h"

#include "rmdsBridgeTypes.h"

#ifdef str
#undef str
#endif

#if defined(__cplusplus)
extern "C" {
#endif


// fid lookup
    void InitFidMap();

typedef struct upaPayload
{
    struct upaFieldPayload*    upaField_;
    mamaMsg                     parent_;
} upaPayloadImpl_t;


typedef struct upaIterator_t
{
    upaFieldPayload*   upaField_;
} upaIterator_t;


mama_status
upaMsg_setBool(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        mama_bool_t     value);


mama_status
upaMsg_setChar(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        char            value);

mama_status
upaMsg_setI8(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        int8_t          value);

mama_status
upaMsg_setU8(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        uint8_t         value);

mama_status
upaMsg_setI16(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        int16_t         value);

mama_status
upaMsg_setU16(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        uint16_t        value);

mama_status
upaMsg_setI32(
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    int32_t             value);

mama_status
upaMsg_setU32(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        uint32_t        value);

mama_status
upaMsg_setI64(
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    int64_t             value);

mama_status
upaMsg_setU64(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        uint64_t        value);

mama_status
upaMsg_setF32(
   const msgPayload    msg,
   const char*     name,
   mama_fid_t      fid,
   mama_f32_t      value);

mama_status
upaMsg_setF64(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    mama_f64_t   value);

mama_status
upaMsg_setString(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    const char*  value);

mama_status
upaMsg_setOpaque(
        const msgPayload    msg,
        const char*  name,
        mama_fid_t   fid,
        const void*  value,
        size_t       size);

mama_status
upaMsg_setVectorBool(
    const msgPayload    msg,
    const char*        name,
    mama_fid_t         fid,
    const mama_bool_t  value[],
    size_t             numElements);

mama_status
upaMsg_setVectorChar (
    const msgPayload    msg,
    const char*        name,
    mama_fid_t         fid,
    const char         value[],
    size_t             numElements);

mama_status
upaMsg_setVectorI8 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const int8_t       value[],
        size_t             numElements);

mama_status
upaMsg_setVectorU8 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const uint8_t      value[],
        size_t             numElements);

mama_status
upaMsg_setVectorI16 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const int16_t      value[],
        size_t             numElements);

mama_status
upaMsg_setVectorU16 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const uint16_t     value[],
        size_t             numElements);

mama_status
upaMsg_setVectorI32 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const int32_t      value[],
        size_t             numElements);


mama_status
upaMsg_setVectorU32 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const uint32_t     value[],
        size_t             numElements);


mama_status
upaMsg_setVectorI64 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const int64_t      value[],
        size_t             numElements);


mama_status
upaMsg_setVectorU64 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const uint64_t     value[],
        size_t             numElements);


mama_status
upaMsg_setVectorF32 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const mama_f32_t   value[],
        size_t             numElements);


mama_status
upaMsg_setVectorF64 (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const mama_f64_t   value[],
        size_t             numElements);


mama_status
upaMsg_setVectorString (
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const char*        value[],
        size_t             numElements);


mama_status
upaMsg_setVectorDateTime (
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value[],
    size_t              numElements);

mama_status
upaMsg_setMsg(
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const msgPayload   value);


mama_status
upaMsg_setVectorMsg(
        const msgPayload    msg,
        const char*        name,
        mama_fid_t         fid,
        const mamaMsg      value[],
        size_t             numElements);


mama_status
upaMsg_getBool(
        const msgPayload    msg,
        const char*   name,
        mama_fid_t    fid,
        mama_bool_t*  result);

mama_status
upaMsg_getChar(
        const msgPayload    msg,
        const char*   name,
        mama_fid_t    fid,
        char*         result);

mama_status
upaMsg_getI8(
        const msgPayload    msg,
        const char*   name,
        mama_fid_t    fid,
        int8_t*       result);

mama_status
upaMsg_getU8(
        const msgPayload    msg,
        const char*   name,
        mama_fid_t    fid,
        uint8_t*      result);

mama_status
upaMsg_getI16(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        int16_t*        result);


mama_status
upaMsg_getU16(
        const msgPayload    msg,
        const char*     name,
        mama_fid_t      fid,
        uint16_t*       result);

mama_status
upaMsg_getI32(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    int32_t*     result);

mama_status
upaMsg_getU32(
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        uint32_t*      result);


mama_status
upaMsg_getI64(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    int64_t*     result);


mama_status
upaMsg_getU64(
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        uint64_t*      result);

mama_status
upaMsg_getF32(
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        mama_f32_t*    result);


mama_status
upaMsg_getF64(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    mama_f64_t*  result);


mama_status
upaMsg_getString(
    const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    const char** result);


mama_status
upaMsg_getOpaque(
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        const void**   result,
        size_t*        size);


mama_status
upaMsg_getDateTime(
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        mamaDateTime   result);


mama_status
    upaMsg_setDateTime(
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const MamaDateTimeWrapper_ptr_t&  value);

mama_status
upaMsg_getVectorDateTime (
        const msgPayload    msg,
        const char*           name,
        mama_fid_t            fid,
        const mamaDateTime*   result,
        size_t*               resultLen);

mama_status
upaMsg_getVectorPrice (
        const msgPayload    msg,
        const char*           name,
        mama_fid_t            fid,
        const mamaPrice*      result,
        size_t*               resultLen);

mama_status
upaMsg_setPrice(
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const MamaPriceWrapper_ptr_t&     value);

mama_status
upaMsg_setVectorPrice (
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice     value[],
    size_t              numElements);

mama_status
upaMsg_getPrice(
    const msgPayload    msg,
    const char*    name,
    mama_fid_t     fid,
    mamaPrice      result);

mama_status
upaMsg_getMsg (
    const msgPayload    msg,
    const char*      name,
    mama_fid_t       fid,
    msgPayload*      result);

mama_status
upaMsg_getVectorBool (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const mama_bool_t**  result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorChar (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const char**         result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorI8 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const int8_t**       result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorU8 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const uint8_t**      result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorI16 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const int16_t**      result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorU16 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const uint16_t**     result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorI32 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const int32_t**      result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorU32 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const uint32_t**     result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorI64 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const int64_t**      result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorU64 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const uint64_t**     result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorF32 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const mama_f32_t**   result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorF64 (
        const msgPayload    msg,
        const char*          name,
        mama_fid_t           fid,
        const mama_f64_t**   result,
        size_t*              resultLen);


mama_status
upaMsg_getVectorString (
        const msgPayload    msg,
        const char*    name,
        mama_fid_t     fid,
        const char***  result,
        size_t*        resultLen);


mama_status
upaMsg_getVectorMsg (
    const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const msgPayload**  result,
    size_t*             resultLen);

mama_status
upaMsg_getFieldAsString(const msgPayload    msg,
    const char*  name,
    mama_fid_t   fid,
    char*        buf,
    size_t       len);



#if defined(__cplusplus)
}
#endif

#endif
