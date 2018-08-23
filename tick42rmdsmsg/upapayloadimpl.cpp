/*
* UpaPayloadImpl: The Reuters RMDS Bridge for OpenMama
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

#ifdef str
#undef str
#endif

#include "version.h"
#include "upapayloadimpl.h"
#include "upamsgimpl.h"
#include "upapayload.h"
#include "upavaluetype.h"

#include "MamaPriceWrapper.h"
#include "MamaDateTimeWrapper.h"

#include "rmdsBridgeTypes.h"

std::ostream & operator<<(std::ostream& out, const mama_datetime & val){return out << val.dt;}

std::ostream& operator<<(std::ostream& out, const MamaMsgVectorWrapper_ptr_t v){return out << "msgVector";}

#define upaPayload(msg) ((UpaPayload*)msg)

#define CHECK_PAYLOAD(msg) \
    do {  \
    if ((upaPayload(msg)) == 0) return MAMA_STATUS_NULL_ARG; \
    } while(0)

//CHECK_BUFFER should work for output variable (by reference as pointer) and for input variable the same way.
//For example: either bufferLength is mama_size_t or mama_size_t* the condition (bufferLength) == 0) is still valid for both
#define CHECK_BUFFER(buffer, bufferLength) \
    do { \
    if (!buffer) return MAMA_STATUS_NULL_ARG; \
    if (bufferLength <= 0) return MAMA_STATUS_INVALID_ARG; \
    } while(0)

#define CHECK_NAME(name,fid) \
    do {  \
    if ((fid == 0) && (name == 0)) return MAMA_STATUS_NULL_ARG; \
    if ((fid == 0) && (strlen(name)== 0)) return MAMA_STATUS_INVALID_ARG; \
    } while(0)


#define CHECK_ITER(iter) \
    do {  \
    if (((upaMsgIteratorImpl*)(iter)) == 0) return MAMA_STATUS_NULL_ARG; \
    } while(0)

#define upaField(field) ((UpaFieldPayload*)(field))

#define CHECK_FIELD(field) \
    do {  \
    if (upaField(field) == 0) return MAMA_STATUS_NULL_ARG; \
    } while(0)


#define CHECK_SIZE_PTR(size) \
    do {  \
    if (size == NULL) return MAMA_STATUS_NULL_ARG; \
    } while(0)

#define CHECK_MESSAGE(msg) \
    do {  \
    if (!msg) return MAMA_STATUS_NULL_ARG; \
    } while(0)

#define CHECK_ARRAY_VALUE(array_value) \
    do {  \
    if (array_value == NULL) return MAMA_STATUS_NULL_ARG; \
    } while(0)

#define CHECK_RESULT_ARRAY_PTR(result_array) \
    do {  \
    if (result_array == NULL) return MAMA_STATUS_NULL_ARG; \
    } while(0)

msgFieldPayload
    tick42rmdsmsgPayloadIter_get          (msgPayloadIter  iter,
    msgFieldPayload field,
    msgPayload      msg);
/******************************************************************************
* bridge functions
*******************************************************************************/

extern mama_status
    tick42rmdsmsgPayload_init(mamaPayloadBridge payloadBridge, char* identifier)
{
    mama_status resultStatus = MAMA_STATUS_OK;

    /* Will set the bridge's compile time MAMA version */
    MAMA_SET_BRIDGE_COMPILE_TIME_VERSION(PAYLOAD_BRIDGE_NAME_STRING);

    *identifier = (char)MAMA_PAYLOAD_TICK42RMDS;

    return resultStatus;
}

mamaPayloadType
    tick42rmdsmsgPayload_getType ()
{
    return MAMA_PAYLOAD_TICK42RMDS;
}

/******************************************************************************
* general functions
*******************************************************************************/
mama_status
    tick42rmdsmsgPayload_create (msgPayload* msg)

{
    if (!msg)  return MAMA_STATUS_NULL_ARG;

    UpaPayload* newPayload = new (std::nothrow) UpaPayload();

    *msg = (msgPayload)newPayload;

    // init the fid map here
    InitFidMap();

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_createForTemplate (msgPayload*         msg,
    mamaPayloadBridge       bridge,
    mama_u32_t          templateId)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgPayload_createFromByteBuffer(msgPayload* msg,
    mamaPayloadBridge bridge,
    const void* buffer, mama_size_t bufferLength)
{
    if (!msg)
        return MAMA_STATUS_NULL_ARG;
    if (!bufferLength)
        return MAMA_STATUS_INVALID_ARG;

    CHECK_BUFFER(buffer, bufferLength); //todo: CHECK_BUFFER should be rewrite

    UpaPayload* newPayload = new (std::nothrow) UpaPayload();

    *msg = (msgPayload)newPayload;

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_copy              (const msgPayload    msg,
    msgPayload*         copy)
{
    CHECK_PAYLOAD(msg);

    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    UpaPayload* newPayload = reinterpret_cast<UpaPayload*>(*copy);

    if (!newPayload)
    {
        newPayload = new (std::nothrow) UpaPayload(*payload);
        if (!newPayload)
        {
            return MAMA_STATUS_PLATFORM;
        }
    }
    else
    {
        *newPayload = *payload;
    }

    *copy = reinterpret_cast<msgPayload*>(newPayload);

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_clear             (msgPayload          msg)
{
    CHECK_PAYLOAD(msg);

    UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    if (!payload)
    {
        return MAMA_STATUS_PLATFORM;
    }

    payload->clear();

    return MAMA_STATUS_OK;
}

//mama_status
//    tick42rmdsmsgPayload_markAllDirty (msgPayload msg)
//{
//    CHECK_PAYLOAD(msg);
//
//    UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
//    if (!payload)
//    {
//        return MAMA_STATUS_PLATFORM;
//    }
//
//    payload->markAllDirty();
//
//    return MAMA_STATUS_OK;
//}

mama_status
    tick42rmdsmsgPayload_destroy(msgPayload msg)
{
    CHECK_PAYLOAD(msg);

    UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    if (!payload)
    {
        return MAMA_STATUS_PLATFORM;
    }

    payload->clear();
    delete payload;

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_setParent (msgPayload          msg,
    const mamaMsg       parent)
{

    CHECK_PAYLOAD(msg);
    UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    payload->setParent(parent);
    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_getByteSize       (const msgPayload    msg,
    mama_size_t*        size)
{
    //CHECK_PAYLOAD(msg);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgPayload_unSerialize (const msgPayload    msg,
    const void**        buffer,
    mama_size_t        bufferLength)
{
    //CHECK_PAYLOAD(msg);
    //CHECK_BUFFER(buffer, bufferLength);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
    tick42rmdsmsgPayload_serialize     (const msgPayload    msg,
    const void**        buffer,
    mama_size_t*        bufferLength)
{
    //CHECK_PAYLOAD(msg);
    //CHECK_BUFFER(buffer, bufferLength);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgPayload_getByteBuffer     (const msgPayload    msg,
    const void**        buffer,
    mama_size_t*        bufferLength)
{
    CHECK_PAYLOAD(msg);
    CHECK_BUFFER(buffer, bufferLength);
    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);

    *buffer = payload;
    *bufferLength = sizeof(UpaPayload);
    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_setByteBuffer     (const msgPayload    msg,
    mamaPayloadBridge       bridge,
    const void*         buffer,
    mama_size_t         bufferLength)
{
    //CHECK_PAYLOAD(msg);
    //if (!bridge)
    //    return MAMA_STATUS_NULL_ARG;
    //CHECK_BUFFER(buffer, bufferLength);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgPayload_getSendSubject     (const msgPayload    msg,
    const char ** subject)
{
    //CHECK_PAYLOAD(msg);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
    tick42rmdsmsgPayload_getNumFields      (const msgPayload    msg,
    mama_size_t*        numFields)
{
    CHECK_PAYLOAD(msg);
    if (!numFields)
        return MAMA_STATUS_NULL_ARG;

    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    *numFields = payload->numFields();

    return MAMA_STATUS_OK;
}

struct toStringClosure_t
{
    std::string str;
};

/*
* Add a string representation of a filed to its closure. See tick42rmdsmsgPayload_toString for the usage with tick42rmdsmsgPayload_iterateFields
* @param msg - not used, the parameter is part of the callback signature. Might be used with recursive implementation of that callback
* @param field - the current field to work with. it is given by the iteration function.
* @param closure - a closure of type toStringClosure_t.
* @return a dynamically created buffer which should be release by the caller using free or NULL if msg is NULL
*/
void MAMACALLTYPE FieldToStringCb (const mamaMsg msg, const mamaMsgField  field, void* closure)
{
    if (!field || !closure)
        return;
    toStringClosure_t &toStringClosure = *reinterpret_cast<toStringClosure_t*>(closure);

    mamaMsgFieldImpl* impl = (mamaMsgFieldImpl*)field;
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(impl->myPayload);

    std::string text;
    if (MAMA_STATUS_OK == fieldPayload->get(text))
    {
        // TODO better handling of strings
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", fieldPayload->fid_);
        std::string bufs = buf;
        toStringClosure.str += "{" + bufs + ":";

        snprintf(buf, sizeof(buf), "%d", fieldPayload->type_);
        bufs = buf;
        toStringClosure.str += bufs + ":";

        toStringClosure.str += text + "}";
    }
    else
        toStringClosure.str += "{Error}";
}

/*
* Returns a string representation of the message
* @param msg
* @return a dynamically created buffer which should be release by the caller using free or NULL if msg is NULL
*/
const char*
    tick42rmdsmsgPayload_toString          (const msgPayload    msg)
{
    toStringClosure_t closure;
    // Iterate through all fields and add their string representation inside  braces too {field}
    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    if (!payload)
    {
        return NULL;
    }

    // return "{}"; //avoid leak since there is no implementation in OpenMAMA for freeString...

    //todo: the code below is dead. a fix should be applied to the OpenMAMA freeString API to make the code below not leaking
    // Prepare message frame "{....}"
    closure.str += "{";

    //Prepare field for later use
    mamaMsgField  msgField;
    mamaMsgField_create (&msgField);

    tick42rmdsmsgPayload_iterateFields (msg,
        payload->getParent(),
        msgField,
        FieldToStringCb,
        (void*)&closure);

    // By now a message like this pattern should appear {{field}...{field}} field existance is optional (empty message)
    // Complete the message frame
    closure.str += "}";

    // Copy a dynamic buffer of the message to the caller as a result
    char *buf = (char*)malloc(closure.str.size()+1);
    memset(buf, 0, closure.str.size()+1);
    strcpy(buf,closure.str.c_str());

    // Cleanup
    mamaMsgField_destroy(msgField);
    return buf;
}

mama_status
    tick42rmdsmsgPayload_iterateFields (const msgPayload        msg,
    const mamaMsg           parent,
    mamaMsgField            field,
    mamaMsgIteratorCb       cb,
    void*                   closure)
{
    CHECK_PAYLOAD(msg);

    //Check for invalid parent
    if (!parent) return MAMA_STATUS_NULL_ARG;

    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    if (!payload)
    {
        return MAMA_STATUS_PLATFORM;
    }

    return payload->iterateFields(field, cb, closure);
}

/**
* Convert the value of the specified field to a string. Caller must
* provide a buffer for the value.
*
* @param msg The message.
* @param fieldName The field name.
* @param fid The field identifier.
* @param buf The buffer where the resulting string will be copied.
* @param length The length of the caller supplied buffer. That means that there should be room for the extra NULL char in the buffer!;
*/
mama_status
    tick42rmdsmsgPayload_getFieldAsString  (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    char*               buf,
    mama_size_t         len)
{
    CHECK_PAYLOAD(msg);
    //CHECK_NAME(name, fid);
    CHECK_BUFFER(buf, len);

    msgFieldPayload field;

    mama_status result;

    result = tick42rmdsmsgPayload_getField (msg, name, fid, &field);
    if (result != MAMA_STATUS_OK)
        return result;

    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);

    std::string resStr;
    result = fieldPayload->get(resStr);

    size_t strlenBuf = resStr.length(); //including the NULL char
    size_t expected_len = (len-1) <=  strlenBuf ? len-1 : strlenBuf;

    strncpy(buf,resStr.c_str(), expected_len);

    buf[expected_len] ='\0';

    if (result != MAMA_STATUS_OK)
        return result;

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_apply(msgPayload dest,
                               const msgPayload src)
{
    CHECK_PAYLOAD(dest);
    CHECK_PAYLOAD(src);

    UpaPayload* payloadDest = reinterpret_cast<UpaPayload*>(dest);
    const UpaPayload* payloadSrc = reinterpret_cast<const UpaPayload*>(src);

    payloadDest->apply(payloadSrc);

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayload_getNativeMsg     (const msgPayload    msg,
    void**              nativeMsg)
{
    CHECK_PAYLOAD(msg);
    if (!nativeMsg)
        return MAMA_STATUS_NULL_ARG;
    *nativeMsg = msg;
    return MAMA_STATUS_OK;
}

/******************************************************************************
* add functions
*******************************************************************************/

mama_status
    tick42rmdsmsgPayload_addBool           (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_bool_t         value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name, fid);
    return upaMsg_setBool(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addChar(
    msgPayload      msg,
    const char*     name,
    mama_fid_t      fid,
    char            value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setChar(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addI8             (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i8_t           value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI8(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addU8             (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u8_t           value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU8(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addI16            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i16_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI16(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addU16            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u16_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU16(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addI32            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addU32            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addI64            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addU64            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addF32            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setF32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addF64            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setF64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addString         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const char*         value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setString(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_addOpaque         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const void*         value,
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setOpaque(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addDateTime       (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    mamaDateTime dt;
    mamaDateTime_create(&dt);
    MamaDateTimeWrapper_ptr_t pdt(new MamaDateTimeWrapper(dt));
    pdt->SetValue(value);
    return upaMsg_setDateTime(upaPayload(msg), name, fid, pdt);
}

mama_status
    tick42rmdsmsgPayload_addPrice          (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice     value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    mamaPrice price;
    mamaPrice_create(&price);
    mamaPrice_copy(price, value);
    MamaPriceWrapper_ptr_t pPrice(new MamaPriceWrapper(price));
    return upaMsg_setPrice(upaPayload(msg), name, fid, pPrice);
}

mama_status
    tick42rmdsmsgPayload_addMsg            (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    msgPayload          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setMsg(upaPayload(msg), name, fid, value);
}

/******************************************************************************
* addVector... functions
*/

mama_status
    tick42rmdsmsgPayload_addVectorBool     (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_bool_t   value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorBool(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorChar     (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const char          value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorChar(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorI8       (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i8_t     value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorI8(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorU8       (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u8_t     value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorU8(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorI16      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i16_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorI16(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorU16      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u16_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorU16(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorI32      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorI32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorU32      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorU32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorI64      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const int64_t       value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorI64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorU64      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u64_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorU64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorF32      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorF32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorF64      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f64_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorF64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorString   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const char*         value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorString(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorMsg      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaMsg       value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorMsg(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorDateTime (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorDateTime(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_addVectorPrice    (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_ARRAY_VALUE(value);
    return upaMsg_setVectorPrice(upaPayload(msg), name, fid, value, size);
}

/******************************************************************************
* update functions
*******************************************************************************/
mama_status
    tick42rmdsmsgPayload_updateBool        (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_bool_t         value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setBool(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateChar        (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    char                value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setChar(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateU8          (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u8_t           value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU8(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateI8          (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i8_t           value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI8(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateI16         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i16_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI16(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateU16         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u16_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU16(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateI32         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateU32         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateI64         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setI64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateU64         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setU64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateF32         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f32_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setF32(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateF64         (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f64_t          value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setF64(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateString      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const char*         value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setString(upaPayload(msg), name, fid, value);
}

mama_status
    tick42rmdsmsgPayload_updateOpaque      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const void*         value,
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setOpaque(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateDateTime    (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    mamaDateTime dt;
    mamaDateTime_create(&dt);
    MamaDateTimeWrapper_ptr_t pdt(new MamaDateTimeWrapper(dt));
    pdt->SetValue(value);
    return upaMsg_setDateTime(upaPayload(msg), name, fid, pdt);
}

mama_status
    tick42rmdsmsgPayload_updatePrice       (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice     value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    mamaPrice price;
    mamaPrice_create(&price);
    mamaPrice_copy(price, value);
    MamaPriceWrapper_ptr_t pPrice(new MamaPriceWrapper(price));
    return upaMsg_setPrice(upaPayload(msg), name, fid, pPrice);
}

mama_status
    tick42rmdsmsgPayload_updateSubMsg      (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const msgPayload    value)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!value)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_setMsg(upaPayload(msg), name, fid, value);
}


/******************************************************************************
* updateVector... functions
*/

mama_status
    tick42rmdsmsgPayload_updateVectorMsg   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaMsg       value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorMsg(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorString (msgPayload         msg,
    const char*        name,
    mama_fid_t         fid,
    const char*        strList[],
    mama_size_t        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorString(upaPayload(msg), name, fid, strList, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorBool  (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_bool_t   boolList[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorBool(upaPayload(msg), name, fid, boolList, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorChar  (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const char          value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorChar(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorI8    (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i8_t     value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorI8(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorU8    (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u8_t     value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorU8(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorI16   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i16_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorI16(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorU16   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u16_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorU16(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorI32   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorI32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorU32   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorU32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorI64   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i64_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorI64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorU64   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u64_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorU64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorF32   (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f32_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorF32(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorF64   (msgPayload         msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f64_t    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorF64(upaPayload(msg), name, fid, value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorPrice (msgPayload         msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice*    value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorPrice(upaPayload(msg), name, fid, *value, size);
}

mama_status
    tick42rmdsmsgPayload_updateVectorTime  (msgPayload          msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime  value[],
    mama_size_t         size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_setVectorDateTime(upaPayload(msg), name, fid, value, size);
}

/******************************************************************************
* get... functions
*/

mama_status
    tick42rmdsmsgPayload_getBool           (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_bool_t*        mamaResult)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getBool(upaPayload(msg), name, fid, mamaResult);
}

mama_status
    tick42rmdsmsgPayload_getChar           (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    char*               result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getChar(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getI8             (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i8_t*          result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getI8(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getU8             (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u8_t*          result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getU8(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getI16            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i16_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getI16(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getU16            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u16_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getU16(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getI32            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i32_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getI32(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getU32            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u32_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getU32(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getI64            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_i64_t*         mamaResult)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getI64(upaPayload(msg), name, fid, mamaResult);
}

mama_status
    tick42rmdsmsgPayload_getU64            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_u64_t*         mamaResult)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getU64(upaPayload(msg), name, fid, mamaResult);
}

mama_status
    tick42rmdsmsgPayload_getF32            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f32_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getF32(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getF64            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mama_f64_t*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getF64(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getString         (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const char**        result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!result)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_getString(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getOpaque         (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const void**        result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!result)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_getOpaque(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getField          (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    msgFieldPayload*    result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name, fid);
    if (!result)
        return MAMA_STATUS_NULL_ARG;
    UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);

    return payload->getField(fid, name, result);
}

mama_status
    tick42rmdsmsgPayload_getDateTime       (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mamaDateTime        result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!result)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_getDateTime(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getPrice          (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    mamaPrice           result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    if (!result)
        return MAMA_STATUS_NULL_ARG;
    return upaMsg_getPrice(upaPayload(msg), name, fid, result);
}

mama_status
    tick42rmdsmsgPayload_getMsg            (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    msgPayload*         result)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    return upaMsg_getMsg(upaPayload(msg), name, fid, result);
}

/******************************************************************************
* getVector... functions
*/

mama_status
    tick42rmdsmsgPayload_getVectorBool     (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_bool_t** result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorBool(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorChar     (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const char**        result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorChar(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorI8       (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i8_t**   result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorI8(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorU8       (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u8_t**   result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorU8(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorI16      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i16_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorI16(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorU16      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u16_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorU16(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorI32      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i32_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorI32(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorU32      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u32_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorU32(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorI64      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_i64_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorI64(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorU64      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_u64_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorU64(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorF32      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f32_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorF32(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorF64      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mama_f64_t**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorF64(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorString   (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const char***       result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);

    return upaMsg_getVectorString(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorDateTime (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaDateTime* result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorDateTime(upaPayload(msg), name, fid, result, size);
}

mama_status
    tick42rmdsmsgPayload_getVectorPrice    (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const mamaPrice*    result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorPrice(upaPayload(msg), name, fid, result, size);
}


mama_status
    tick42rmdsmsgPayload_getVectorMsg      (const msgPayload    msg,
    const char*         name,
    mama_fid_t          fid,
    const msgPayload**  result,
    mama_size_t*        size)
{
    CHECK_PAYLOAD(msg);
    CHECK_NAME(name,fid);
    CHECK_RESULT_ARRAY_PTR(result);
    return upaMsg_getVectorMsg(upaPayload(msg), name, fid, result, size);
}

/******************************************************************************
* iterator functions
*******************************************************************************/
mama_status
    tick42rmdsmsgPayloadIter_create        (msgPayloadIter* iter,
    msgPayload      msg)
{
    if (!iter)
        return MAMA_STATUS_NULL_ARG;
    CHECK_PAYLOAD(msg);
    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    *iter = (msgPayloadIter) new (std::nothrow) UpaPayloadFieldIterator(*payload);
    return MAMA_STATUS_OK;
}

inline bool CheckIteratorContext(UpaPayloadFieldIterator *iterator, const UpaPayload* payload, msgFieldPayload field)
{
    return true;
}
msgFieldPayload
    tick42rmdsmsgPayloadIter_get          (msgPayloadIter  iter,
    msgFieldPayload field,
    msgPayload      msg)
{
    //CHECK_PAYLOAD(msg);
    //CHECK_ITER(iter);

    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    UpaPayloadFieldIterator *it = reinterpret_cast<UpaPayloadFieldIterator *>(iter);
    const UpaPayload* IterPayloadContext = it->payloadContext();

    if (payload && payload != IterPayloadContext)
        return NULL;

    if (it->current() != it->end())
        return (msgFieldPayload)&(it->current()->second);

    return NULL;
}

msgFieldPayload
    tick42rmdsmsgPayloadIter_next          (msgPayloadIter  iter,
    msgFieldPayload field,
    msgPayload      msg)
{
    //CHECK_PAYLOAD(msg);
    if (!iter || !msg)
        return NULL;
    UpaPayloadFieldIterator* it = reinterpret_cast<UpaPayloadFieldIterator*>(iter);

    UpaPayloadFieldIterator::const_iterator itResult;
    itResult=it->next();
    if (itResult != it->end())
        return (msgFieldPayload)&(itResult->second);

    return NULL;
}

mama_bool_t
    tick42rmdsmsgPayloadIter_hasNext       (msgPayloadIter iter,
    msgPayload     msg)
{
    if (!iter || !msg)
        return 0;
    const UpaPayloadFieldIterator* it = reinterpret_cast<UpaPayloadFieldIterator*>(iter);

    return it->has_next() ? 1 : 0;
}

msgFieldPayload
    tick42rmdsmsgPayloadIter_begin         (msgPayloadIter  iter,
    msgFieldPayload field,
    msgPayload      msg)
{
    if (!iter || !msg)
        return NULL;
    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    UpaPayloadFieldIterator *it = reinterpret_cast<UpaPayloadFieldIterator *>(iter);

    UpaFieldPayload temp = it->begin()->second;
    return (msgFieldPayload)&(it->begin()->second);
}

msgFieldPayload
    tick42rmdsmsgPayloadIter_end           (msgPayloadIter iter,
    msgPayload     msg)
{
    return NULL;
}

mama_status
    tick42rmdsmsgPayloadIter_associate      (msgPayloadIter iter,
    msgPayload     msg)
{
    if (!iter || !msg)
        return MAMA_STATUS_NULL_ARG;

    const UpaPayload* payload = reinterpret_cast<UpaPayload*>(msg);
    UpaPayloadFieldIterator *it = reinterpret_cast<UpaPayloadFieldIterator *>(iter);

    it->associate(*payload);
    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgPayloadIter_destroy       (msgPayloadIter iter)
{
    if (!iter)
        return MAMA_STATUS_NULL_ARG;
    UpaPayloadFieldIterator *it = reinterpret_cast<UpaPayloadFieldIterator *>(iter);

    delete it;
    return MAMA_STATUS_OK;
}

/******************************************************************************
* general field functions
*******************************************************************************/
mama_status
    tick42rmdsmsgFieldPayload_create      (msgFieldPayload*   field)
{
    *field = new (std::nothrow) UpaFieldPayload;
    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgFieldPayload_destroy      (msgFieldPayload   field)
{
    CHECK_FIELD(field);
    delete (UpaFieldPayload*)field;
    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgFieldPayload_getName      (const msgFieldPayload   field,
    mamaDictionary          dict,
    mamaFieldDescriptor     desc,
    const char**            result)
{
    mama_fid_t fid =0;
    CHECK_FIELD(field);

    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);

    *result = fieldPayload->name_;


    if (!*result)
        return MAMA_STATUS_INVALID_ARG;

    return MAMA_STATUS_OK;
}

mama_status
    tick42rmdsmsgFieldPayload_getFid       (const msgFieldPayload   field,
    mamaDictionary          dict,
    mamaFieldDescriptor     desc,
    uint16_t*               result)
{
    CHECK_FIELD(field);
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    *result = fieldPayload->fid_;

    return MAMA_STATUS_OK;
}


mama_status
    tick42rmdsmsgFieldPayload_getDescriptor(const msgFieldPayload   field,
    mamaDictionary          dict,
    mamaFieldDescriptor*    result)
{
    return MAMA_STATUS_OK;
}


mama_status
    tick42rmdsmsgFieldPayload_getType      (msgFieldPayload         field,
    mamaFieldType*          result)
{
    CHECK_FIELD(field);

    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    *result = fieldPayload->type_;

    return MAMA_STATUS_OK;
}

/******************************************************************************
* field update functions
*******************************************************************************/
mama_status
    tick42rmdsmsgFieldPayload_updateBool   (msgFieldPayload         field,
    msgPayload              msg,
    mama_bool_t             value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value ? true : false);
}

mama_status
    tick42rmdsmsgFieldPayload_updateChar   (msgFieldPayload         field,
    msgPayload              msg,
    char                    value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateU8     (msgFieldPayload         field,
    msgPayload              msg,
    mama_u8_t               value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateI8     (msgFieldPayload         field,
    msgPayload              msg,
    mama_i8_t               value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateI16    (msgFieldPayload         field,
    msgPayload              msg,
    mama_i16_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateU16    (msgFieldPayload         field,
    msgPayload              msg,
    mama_u16_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateI32    (msgFieldPayload         field,
    msgPayload              msg,
    mama_i32_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateU32    (msgFieldPayload         field,
    msgPayload              msg,
    mama_u32_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateI64    (msgFieldPayload         field,
    msgPayload              msg,
    mama_i64_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateU64    (msgFieldPayload         field,
    msgPayload              msg,
    mama_u64_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateF32    (msgFieldPayload         field,
    msgPayload              msg,
    mama_f32_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateF64    (msgFieldPayload         field,
    msgPayload              msg,
    mama_f64_t              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(value);
}

mama_status
    tick42rmdsmsgFieldPayload_updateString    (msgFieldPayload         field,
    msgPayload              msg,
    const char *              value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    return upaField(field)->set(std::string(value));
}

mama_status
    tick42rmdsmsgFieldPayload_updateDateTime
    (msgFieldPayload         field,
    msgPayload              msg,
    const mamaDateTime      value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    mamaDateTime dt;
    mamaDateTime_create(&dt);
    MamaDateTimeWrapper_ptr_t pDt(new MamaDateTimeWrapper(dt));
    pDt->SetValue(value);
    return upaField(field)->set(pDt);
}

mama_status
    tick42rmdsmsgFieldPayload_updatePrice  (msgFieldPayload         field,
    msgPayload              msg,
    const mamaPrice         value)
{
    CHECK_FIELD(field);
    CHECK_MESSAGE(msg);
    mamaPrice price;
    mamaPrice_create(&price);
    mamaPrice_copy(price, value);
    MamaPriceWrapper_ptr_t pPrice(new MamaPriceWrapper(price));
    return upaField(field)->set(pPrice);
}


/******************************************************************************
* field get functions
*******************************************************************************/
mama_status
    tick42rmdsmsgFieldPayload_getBool      (const msgFieldPayload   field,
    mama_bool_t*            result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getMamaBool(*result);

}

mama_status
    tick42rmdsmsgFieldPayload_getChar      (const msgFieldPayload   field,
    char*                   result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getChar(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getI8        (const msgFieldPayload   field,
    mama_i8_t*              result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getU8        (const msgFieldPayload   field,
    mama_u8_t*              result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getI16       (const msgFieldPayload   field,
    mama_i16_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getU16       (const msgFieldPayload   field,
    mama_u16_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getI32       (const msgFieldPayload   field,
    mama_i32_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getU32       (const msgFieldPayload   field,
    mama_u32_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getI64       (const msgFieldPayload   field,
    mama_i64_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getU64       (const msgFieldPayload   field,
    mama_u64_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getF32       (const msgFieldPayload   field,
    mama_f32_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getF64       (const msgFieldPayload   field,
    mama_f64_t*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getString    (const msgFieldPayload   field,
    const char**            result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    if (!result)
        return MAMA_STATUS_INVALID_ARG;
    return fieldPayload->get(*result);
}

mama_status
    tick42rmdsmsgFieldPayload_getOpaque    (const msgFieldPayload   field,
    const void**            result,
    mama_size_t*            size)
{
    //if (!result)
    //    return MAMA_STATUS_INVALID_ARG;
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getOpaque(result, size);
}


mama_status
    tick42rmdsmsgFieldPayload_getDateTime  (const msgFieldPayload   field,
    mamaDateTime            result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getDateTime(result);
}


mama_status
    tick42rmdsmsgFieldPayload_getPrice     (const msgFieldPayload   field,
    mamaPrice               result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getPrice(result);
}

mama_status
    tick42rmdsmsgFieldPayload_getMsg       (const msgFieldPayload   field,
    msgPayload*             result)
{
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    return fieldPayload->getMsg(mamaMsg(result));
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorBool(const msgFieldPayload   field,
    const mama_bool_t**     result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorChar(const msgFieldPayload   field,
    const char**            result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorI8  (const msgFieldPayload   field,
    const mama_i8_t**       result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorU8  (const msgFieldPayload   field,
    const mama_u8_t**       result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorI16 (const msgFieldPayload   field,
    const mama_i16_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorU16 (const msgFieldPayload   field,
    const mama_u16_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorI32 (const msgFieldPayload   field,
    const mama_i32_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorU32 (const msgFieldPayload   field,
    const mama_u32_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorI64 (const msgFieldPayload   field,
    const mama_i64_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorU64 (const msgFieldPayload   field,
    const mama_u64_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorF32 (const msgFieldPayload   field,
    const mama_f32_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorF64 (const msgFieldPayload   field,
    const mama_f64_t**      result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorString(const msgFieldPayload   field,
    const char ***result,
    mama_size_t *size)
{
    UpaFieldPayload* fieldPayload = reinterpret_cast<UpaFieldPayload*>(field);
    CHECK_FIELD(field);
    CHECK_SIZE_PTR(size);
    return fieldPayload->get(result, size);
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorDateTime
    (const msgFieldPayload   field,
    const mamaDateTime*     result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorPrice
    (const msgFieldPayload   field,
    const mamaPrice*        result,
    mama_size_t*            size)
{
    //CHECK_FIELD(field);
    //CHECK_SIZE_PTR(size);
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
    tick42rmdsmsgFieldPayload_getVectorMsg (const msgFieldPayload   field,
    const msgPayload**      result,
    mama_size_t*            size)
{
    CHECK_FIELD(field);
    CHECK_SIZE_PTR(size);
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    return fieldPayload->getVectorMsg(result, size);
}

mama_status
    tick42rmdsmsgFieldPayload_getAsString (
    const msgFieldPayload   field,
    const msgPayload   msg,
    char*         buf,
    mama_size_t   len)
{
    CHECK_FIELD(field);
    const UpaFieldPayload* fieldPayload = reinterpret_cast<const UpaFieldPayload*>(field);
    mama_fid_t fid = fieldPayload->fid_;
    const char* name = fieldPayload->name_;
    return tick42rmdsmsgPayload_getFieldAsString(msg, name, fid, buf, len);

}

