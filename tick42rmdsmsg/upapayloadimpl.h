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

#ifndef UPA_MSGBRIDGE_H__
#define UPA_MSGBRIDGE_H__

#include <mama/types.h>
#include "payloadbridge.h"
#include "rmdsBridgeTypes.h"

extern "C" {

MAMAExpBridgeDLL
extern mama_status
tick42rmdsmsgPayload_destroyImpl       (mamaPayloadBridge mamaPayloadBridge);

MAMAExpBridgeDLL
extern mama_status
tick42rmdsmsgPayload_createImpl        (mamaPayloadBridge* result, char* identifier);

extern mamaPayloadType
tick42rmdsmsgPayload_getType           (void);

extern mama_status
tick42rmdsmsgPayload_create            (msgPayload*         msg);

extern mama_status
tick42rmdsmsgPayload_createForTemplate (msgPayload*         msg,
                                mamaPayloadBridge       bridge,
                                mama_u32_t          templateId);
extern mama_status
tick42rmdsmsgPayload_copy              (const msgPayload    msg,
                                msgPayload*         copy);
extern mama_status
tick42rmdsmsgPayload_clear             (msgPayload          msg);
extern mama_status
tick42rmdsmsgPayload_destroy           (msgPayload          msg);
extern mama_status
tick42rmdsmsgPayload_setParent         (msgPayload          msg,
                               const mamaMsg       parent);
extern mama_status
tick42rmdsmsgPayload_getByteSize       (const msgPayload    msg,
                                mama_size_t*        size);

extern mama_status
tick42rmdsmsgPayload_getNumFields      (const msgPayload    msg,
                                mama_size_t*        numFields);

extern mama_status
tick42rmdsmsgPayload_getSendSubject      (const msgPayload    msg,
                                const char **        subject);

extern const char*
tick42rmdsmsgPayload_toString          (const msgPayload    msg);
extern mama_status
tick42rmdsmsgPayload_iterateFields     (const msgPayload    msg,
                                const mamaMsg       parent,
                                mamaMsgField        field,
                                mamaMsgIteratorCb   cb,
                                void*               closure);
extern mama_status
tick42rmdsmsgPayload_getFieldAsString  (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                char*               buffer,
                                mama_size_t         len);

extern mama_status
tick42rmdsmsgPayload_serialize (const msgPayload    payload,
                           const void**        buffer,
                           mama_size_t*        bufferLength);

extern mama_status
tick42rmdsmsgPayload_unSerialize (const msgPayload    payload,
                           const void**        buffer,
                           mama_size_t        bufferLength);


extern mama_status
tick42rmdsmsgPayload_getByteBuffer     (const msgPayload    msg,
                                const void**        buffer,
                                mama_size_t*        bufferLength);
extern mama_status
tick42rmdsmsgPayload_setByteBuffer     (const msgPayload    msg,
                                mamaPayloadBridge       bridge,
                                const void*         buffer,
                                mama_size_t         bufferLength);

extern mama_status
tick42rmdsmsgPayload_createFromByteBuffer (msgPayload*          msg,
                                   mamaPayloadBridge        bridge,
                                   const void*          buffer,
                                   mama_size_t          bufferLength);
extern mama_status
tick42rmdsmsgPayload_apply             (msgPayload          dest,
                                const msgPayload    src);
extern mama_status
tick42rmdsmsgPayload_getNativeMsg     (const msgPayload    msg,
                               void**              nativeMsg);
extern mama_status
tick42rmdsmsgPayload_addBool           (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_bool_t         value);
extern mama_status
tick42rmdsmsgPayload_addChar           (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                char                value);
extern mama_status
tick42rmdsmsgPayload_addI8             (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i8_t           value);
extern mama_status
tick42rmdsmsgPayload_addU8             (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u8_t           value);
extern mama_status
tick42rmdsmsgPayload_addI16            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i16_t          value);
extern mama_status
tick42rmdsmsgPayload_addU16            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u16_t          value);
extern mama_status
tick42rmdsmsgPayload_addI32            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i32_t          value);
extern mama_status
tick42rmdsmsgPayload_addU32            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u32_t          value);
extern mama_status
tick42rmdsmsgPayload_addI64            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i64_t          value);
extern mama_status
tick42rmdsmsgPayload_addU64            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u64_t          value);
extern mama_status
tick42rmdsmsgPayload_addF32            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f32_t          value);
extern mama_status
tick42rmdsmsgPayload_addF64            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f64_t          value);
extern mama_status
tick42rmdsmsgPayload_addString         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char*         value);
extern mama_status
tick42rmdsmsgPayload_addOpaque         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const void*         value,
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addDateTime       (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaDateTime  value);
extern mama_status
tick42rmdsmsgPayload_addPrice          (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaPrice     value);
extern mama_status
tick42rmdsmsgPayload_addMsg            (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                msgPayload          value);
extern mama_status
tick42rmdsmsgPayload_addVectorBool     (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_bool_t   value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorChar     (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char          value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorI8       (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i8_t     value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorU8       (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u8_t     value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorI16      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i16_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorU16      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u16_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorI32      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorU32      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorI64      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorU64      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorF32      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorF64      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorString   (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char*         value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorMsg      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaMsg       value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorDateTime (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaDateTime  value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_addVectorPrice    (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaPrice    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateBool        (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_bool_t         value);
extern mama_status
tick42rmdsmsgPayload_updateChar        (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                char                value);
extern mama_status
tick42rmdsmsgPayload_updateU8          (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u8_t           value);
extern mama_status
tick42rmdsmsgPayload_updateI8          (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i8_t           value);
extern mama_status
tick42rmdsmsgPayload_updateI16         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i16_t          value);
extern mama_status
tick42rmdsmsgPayload_updateU16         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u16_t          value);
extern mama_status
tick42rmdsmsgPayload_updateI32         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i32_t          value);
extern mama_status
tick42rmdsmsgPayload_updateU32         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u32_t          value);
extern mama_status
tick42rmdsmsgPayload_updateI64         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i64_t          value);
extern mama_status
tick42rmdsmsgPayload_updateU64         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u64_t          value);
extern mama_status
tick42rmdsmsgPayload_updateF32         (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f32_t          value);
extern mama_status
tick42rmdsmsgPayload_updateF64
                               (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f64_t          value);
extern mama_status
tick42rmdsmsgPayload_updateString      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char*         value);
extern mama_status
tick42rmdsmsgPayload_updateOpaque      (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const void*         value,
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateDateTime    (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaDateTime  value);
extern mama_status
tick42rmdsmsgPayload_updatePrice       (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaPrice     value);
extern mama_status
tick42rmdsmsgPayload_getBool           (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_bool_t*        result);
extern mama_status
tick42rmdsmsgPayload_updateSubMsg      (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const msgPayload    subMsg);
extern mama_status
tick42rmdsmsgPayload_updateVectorMsg   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mamaMsg       value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorString (msgPayload         msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const char*         value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorBool  (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_bool_t   value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorChar  (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const char          value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorI8    (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_i8_t     value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorU8    (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_u8_t     value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorI16   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_i16_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorU16   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_u16_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorI32   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_i32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorU32   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_u32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorI64   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_i64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorU64   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_u64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorF32   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_f32_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorF64   (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mama_f64_t    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorPrice (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mamaPrice*    value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_updateVectorTime  (msgPayload          msg,
                                const char*         fname,
                                mama_fid_t          fid,
                                const mamaDateTime  value[],
                                mama_size_t         size);
extern mama_status
tick42rmdsmsgPayload_getChar           (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                char*               result);
extern mama_status
tick42rmdsmsgPayload_getI8             (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i8_t*          result);
extern mama_status
tick42rmdsmsgPayload_getU8             (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u8_t*          result);
extern mama_status
tick42rmdsmsgPayload_getI16            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i16_t*         result);
extern mama_status
tick42rmdsmsgPayload_getU16            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u16_t*         result);
extern mama_status
tick42rmdsmsgPayload_getI32            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i32_t*         result);
extern mama_status
tick42rmdsmsgPayload_getU32            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u32_t*         result);
extern mama_status
tick42rmdsmsgPayload_getI64            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_i64_t*         result);
extern mama_status
tick42rmdsmsgPayload_getU64            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_u64_t*         result);
extern mama_status
tick42rmdsmsgPayload_getF32            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f32_t*         result);
extern mama_status
tick42rmdsmsgPayload_getF64            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mama_f64_t*         result);
extern mama_status
tick42rmdsmsgPayload_getString         (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char**        result);
extern mama_status
tick42rmdsmsgPayload_getOpaque         (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const void**        result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getField          (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                msgFieldPayload*    result);
extern mama_status
tick42rmdsmsgPayload_getDateTime       (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mamaDateTime        result);
extern mama_status
tick42rmdsmsgPayload_getPrice          (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                mamaPrice           result);
extern mama_status
tick42rmdsmsgPayload_getMsg            (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                msgPayload*         result);
extern mama_status
tick42rmdsmsgPayload_getVectorBool     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_bool_t** result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorChar     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char**        result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorI8       (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i8_t**   result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorU8       (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u8_t**   result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorI16      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i16_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorU16      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u16_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorI32      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i32_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorU32      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u32_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorI64      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i64_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorU64      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u64_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorF32      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f32_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorF64      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f64_t**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorString   (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char***       result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorDateTime (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaDateTime* result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorPrice    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaPrice*    result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayload_getVectorMsg      (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const msgPayload**  result,
                                mama_size_t*        size);
extern mama_status
tick42rmdsmsgPayloadIter_create        (msgPayloadIter*     iter,
                                msgPayload          msg);
extern msgFieldPayload
tick42rmdsmsgPayloadIter_next          (msgPayloadIter      iter,
                                msgFieldPayload     field,
                                msgPayload          msg);
extern mama_bool_t
tick42rmdsmsgPayloadIter_hasNext       (msgPayloadIter      iter,
                                msgPayload          msg);
extern msgFieldPayload
tick42rmdsmsgPayloadIter_begin         (msgPayloadIter      iter,
                                msgFieldPayload     field,
                                msgPayload          msg);
extern msgFieldPayload
tick42rmdsmsgPayloadIter_end           (msgPayloadIter      iter,
                                msgPayload          msg);
extern mama_status
tick42rmdsmsgPayloadIter_associate     (msgPayloadIter      iter,
                                msgPayload          msg);
extern mama_status
tick42rmdsmsgPayloadIter_destroy       (msgPayloadIter      iter);

extern mama_status
tick42rmdsmsgFieldPayload_create       (msgFieldPayload*    field);

extern mama_status
tick42rmdsmsgFieldPayload_destroy      (msgFieldPayload     field);

extern mama_status
tick42rmdsmsgFieldPayload_getName      (const msgFieldPayload   field,
                                mamaDictionary          dict,
                                mamaFieldDescriptor     desc,
                                const char**            result);
extern mama_status
tick42rmdsmsgFieldPayload_getFid       (const msgFieldPayload   field,
                                mamaDictionary          dict,
                                mamaFieldDescriptor     desc,
                                uint16_t*               result);

extern mama_status
tick42rmdsmsgFieldPayload_getDescriptor(const msgFieldPayload   field,
                                mamaDictionary          dict,
                                mamaFieldDescriptor*    result);
extern mama_status
tick42rmdsmsgFieldPayload_getType      (const msgFieldPayload   field,
                                mamaFieldType*          result);
extern mama_status
tick42rmdsmsgFieldPayload_updateBool   (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_bool_t             value);
extern mama_status
tick42rmdsmsgFieldPayload_updateChar   (msgFieldPayload         field,
                                msgPayload              msg,
                                char                    value);
extern mama_status
tick42rmdsmsgFieldPayload_updateU8     (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_u8_t               value);
extern mama_status
tick42rmdsmsgFieldPayload_updateI8     (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_i8_t               value);
extern mama_status
tick42rmdsmsgFieldPayload_updateI16    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_i16_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateU16    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_u16_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateI32    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_i32_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateU32    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_u32_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateI64    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_i64_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateU64    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_u64_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateF32    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_f32_t              value);
extern mama_status
tick42rmdsmsgFieldPayload_updateF64    (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_f64_t              value);

extern mama_status
tick42rmdsmsgFieldPayload_updateString  (msgFieldPayload         field,
								msgPayload              msg,
								const char *              value);

extern mama_status
tick42rmdsmsgFieldPayload_updateDateTime
                               (msgFieldPayload         field,
                                msgPayload              msg,
                                const mamaDateTime      value);
extern mama_status
tick42rmdsmsgFieldPayload_updatePrice  (msgFieldPayload         field,
                                msgPayload              msg,
                                const mamaPrice         value);
extern mama_status
tick42rmdsmsgFieldPayload_getBool      (const msgFieldPayload   field,
                                mama_bool_t*            result);
extern mama_status
tick42rmdsmsgFieldPayload_getChar      (const msgFieldPayload   field,
                                char*                   result);
extern mama_status
tick42rmdsmsgFieldPayload_getI8        (const msgFieldPayload   field,
                                mama_i8_t*              result);
extern mama_status
tick42rmdsmsgFieldPayload_getU8        (const msgFieldPayload   field,
                                mama_u8_t*              result);
extern mama_status
tick42rmdsmsgFieldPayload_getI16       (const msgFieldPayload   field,
                                mama_i16_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getU16       (const msgFieldPayload   field,
                                mama_u16_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getI32       (const msgFieldPayload   field,
                                mama_i32_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getU32       (const msgFieldPayload   field,
                                mama_u32_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getI64       (const msgFieldPayload   field,
                                mama_i64_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getU64       (const msgFieldPayload   field,
                                mama_u64_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getF32       (const msgFieldPayload   field,
                                mama_f32_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getF64       (const msgFieldPayload   field,
                                mama_f64_t*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getString    (const msgFieldPayload   field,
                                const char**            result);
extern mama_status
tick42rmdsmsgFieldPayload_getOpaque    (const msgFieldPayload   field,
                                const void**            result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getField     (const msgFieldPayload   field,
                                mamaMsgField*           result);
extern mama_status
tick42rmdsmsgFieldPayload_getDateTime  (const msgFieldPayload   field,
                                mamaDateTime            result);
extern mama_status
tick42rmdsmsgFieldPayload_getPrice     (const msgFieldPayload   field,
                                mamaPrice               result);
extern mama_status
tick42rmdsmsgFieldPayload_getMsg       (const msgFieldPayload   field,
                                msgPayload*             result);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorBool
                               (const msgFieldPayload   field,
                                const mama_bool_t**     result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorChar
                               (const msgFieldPayload   field,
                                const char**            result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorI8
                               (const msgFieldPayload   field,
                                const mama_i8_t**       result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorU8  (const msgFieldPayload   field,
                                const mama_u8_t**       result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorI16 (const msgFieldPayload   field,
                                const mama_i16_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorU16 (const msgFieldPayload   field,
                                const mama_u16_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorI32 (const msgFieldPayload   field,
                                const mama_i32_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorU32 (const msgFieldPayload   field,
                                const mama_u32_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorI64 (const msgFieldPayload   field,
                                const mama_i64_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorU64 (const msgFieldPayload   field,
                                const mama_u64_t**      result,
                                mama_size_t*            size);
 extern mama_status
tick42rmdsmsgFieldPayload_getVectorF32 (const msgFieldPayload   field,
                                const mama_f32_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorF64 (const msgFieldPayload   field,
                                const mama_f64_t**      result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorString
                               (const msgFieldPayload   field,
                                const char***           result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorDateTime
                               (const msgFieldPayload   field,
                                const mamaDateTime*     result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorPrice
                               (const msgFieldPayload   field,
                                const mamaPrice*        result,
                                mama_size_t*            size);
extern mama_status
tick42rmdsmsgFieldPayload_getVectorMsg (const msgFieldPayload   field,
                                const msgPayload**      result,
                                mama_size_t*            size);

extern mama_status
tick42rmdsmsgFieldPayload_getAsString (const msgFieldPayload   field,
								const msgPayload   msg,
								char*         buf,
								mama_size_t   len);


//mama_status
//	tick42rmdsmsgPayload_markAllDirty (msgPayload msg);

}

#endif /* UPA_MSG_IMPL_H__*/
