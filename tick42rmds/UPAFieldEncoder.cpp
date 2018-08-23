/*
* RMDSBridge: The Reuters RMDS Bridge for OpenMama
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
#include <utils/t42log.h>
#include "UPAFieldEncoder.h"
#include "utils/namespacedefines.h"

using namespace std;

utils::collection::unordered_set<int> UPAFieldEncoder::suppressBadEnumWarnings_;

//////////////////////////////////////////////////////////////////////////////
//
UPAFieldEncoder::UPAFieldEncoder(mamaDictionary dictionary, const UpaMamaFieldMap_ptr_t& upaFieldMap,
        RsslDataDictionary *rmdsDictionary, const string &sourceName, const string &symbol)
    : dictionary_(dictionary)
    , upaFieldMap_(upaFieldMap)
    , rmdsDictionary_(rmdsDictionary)
    , sourceName_(sourceName)
    , symbol_(symbol)
{
}

//////////////////////////////////////////////////////////////////////////////
//
bool UPAFieldEncoder::encode(mamaMsg msg, RsslChannel *chnl, RsslMsg *rsslMsg, RsslBuffer *buffer)
{
    encodeFail_ = false;
    rsslMessageBuffer_ = buffer;

    rsslClearEncodeIterator(&itEncode_);
    RsslRet ret;
    if ((ret = rsslSetEncodeIteratorBuffer(&itEncode_, rsslMessageBuffer_)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeIteratorBuffer()  for %s : %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), ret);
        return false;
    }
    rsslSetEncodeIteratorRWFVersion(&itEncode_, chnl->majorVersion, chnl->minorVersion);

    // encode the message header
    if ((ret = rsslEncodeMsgInit(&itEncode_, rsslMsg, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeMsgInit() for %s : %s  failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), ret);
        return false;
    }

    rsslClearFieldList(&fList_);
    fList_.flags = RSSL_FLF_HAS_STANDARD_DATA;
    if ((ret = rsslEncodeFieldListInit(&itEncode_, &fList_, 0, 0)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeFieldListInit() failed for %s : %s with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), ret);
        return false;
    }

    mamaMsg_iterateFields(msg, mamaMsgIteratorCb, dictionary_, this);

    // complete encode field list
    if ((ret = rsslEncodeFieldListComplete(&itEncode_, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeFieldListComplete()  %s : %s  failed with return code: %d\n",  sourceName_.c_str(), symbol_.c_str(), ret);
        return false;
    }

    /* complete encode message */
    if ((ret = rsslEncodeMsgComplete(&itEncode_, RSSL_TRUE)) < RSSL_RET_SUCCESS)
    {
        t42log_warn("rsslEncodeMsgComplete()  %s : %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(),  ret);
        return false;
    }
    rsslMessageBuffer_->length = rsslGetEncodedBufferLength(&itEncode_);

    return !encodeFail_;
}

//////////////////////////////////////////////////////////////////////////////
//
void MAMACALLTYPE UPAFieldEncoder::mamaMsgIteratorCb(const mamaMsg msg,
    const mamaMsgField field, void *closure)
{
    ((UPAFieldEncoder*)closure)->encodeField(field);
}

//////////////////////////////////////////////////////////////////////////////
//
void UPAFieldEncoder::encodeField(const mamaMsgField &field)
{
    uint16_t fid;
    const char* fname;
    mamaFieldType fldType;
    mama_status status;

    // get the fid and its type and value (cf mamalistenc displayField line 1470 or thereabouts)
    mamaMsgField_getFid(field, &fid);
    mamaMsgField_getName(field, &fname);
    mamaMsgField_getType(field, &fldType);

    // now need to map the fid then insert the typed data into the rssl message
    RsslInt32 rmdsFid = upaFieldMap_->GetRMDSFidFromMAMAFid(fid);

    if (rmdsFid != 0)
    {
        t42log_info("RMDS fid %d from mama fid %d", rmdsFid, fid);
        //we can do something with it
        RsslDictionaryEntry * dictEntry = rmdsDictionary_->entriesArray[rmdsFid];

        if (dictEntry == 0)
        {
            // todo add something that puts the fids in a bag and uses that to check we only log the message once
            t42log_warn("No RMDS dictionary entry for published fid %d (mama fid = %d)\n", rmdsFid, fid);
            // just bail out
            return;
        }
        RsslFieldEntry fieldEntry = RSSL_INIT_FIELD_ENTRY;
        fieldEntry.fieldId = rmdsFid;
        fieldEntry.dataType = dictEntry->rwfType;

        RsslRet ret;

        switch(fieldEntry.dataType)
        {

        case RSSL_DT_INT:
            //            RsslInt64 intVal;
            mama_i64_t intVal;  // gcc seems happier with this
            status = mamaMsgField_getI64(field, &intVal);
            if (status != MAMA_STATUS_OK)
            {
                t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getI64\n",
                    mamaStatus_stringForStatus(status), status,
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;
            }
            else
            {
                // and encode into the message
                if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&intVal)) < RSSL_RET_SUCCESS)
                {
                    t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                        rsslRetCodeToString(ret), ret,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }
            }
            break;

        case RSSL_DT_UINT:

            // RsslUInt64 uintVal;
            mama_u64_t uintVal; // gcc seems happier with this
            status = mamaMsgField_getU64(field, &uintVal);
            if (status != MAMA_STATUS_OK)
            {
                t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getU64\n",
                    mamaStatus_stringForStatus(status), status,
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;
            }
            else
            {
                // and encode into the message
                if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&uintVal)) < RSSL_RET_SUCCESS)
                {
                    t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                        rsslRetCodeToString(ret), ret,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }
            }
            break;

        case RSSL_DT_FLOAT:
            RsslFloat fltVal;
            status = mamaMsgField_getF32(field, &fltVal);
            if (status != MAMA_STATUS_OK)
            {
                t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getF32\n",
                    mamaStatus_stringForStatus(status), status,
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;
            }
            else
            {
                // and encode into the message
                if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&fltVal)) < RSSL_RET_SUCCESS)
                {
                    t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                        rsslRetCodeToString(ret), ret,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }
            }
            break;

        case RSSL_DT_DOUBLE:
            RsslDouble dblVal;
            status = mamaMsgField_getF64(field, &dblVal);
            if (status != MAMA_STATUS_OK)
            {
                t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getF64\n",
                    mamaStatus_stringForStatus(status), status,
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;
            }
            else
            {
                // and encode into the message
                if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&dblVal)) < RSSL_RET_SUCCESS)
                {
                    t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                        rsslRetCodeToString(ret), ret,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }
            }
            break;

        case RSSL_DT_REAL:

            // if the mama field type is a price then encode as real with appropriate exponent
            // extract the mama price from the field

            switch(fldType)
            {
            case MAMA_FIELD_TYPE_PRICE:
                {
                    mamaPrice p;
                    mamaPrice_create(&p);
                    status = mamaMsgField_getPrice(field, p);
                    if (status != MAMA_STATUS_OK)
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getPrice\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                    else
                    {
                        double d;
                        mamaPrice_getValue(p, &d);

                        mamaPricePrecision precision;
                        mamaPrice_getPrecision(p, &precision);
                        RsslRealHints rsslHint = MamaPrecisionToRsslHint(precision, fid);

                        mama_bool_t b = false;
                        mamaPrice_getIsValidPrice(p, &b);

                        // set up an rssl real for the price value
                        RsslReal r;
                        if (b == false)
                        {
                            rsslBlankReal(&r);
                        }
                        else
                        {
                            rsslClearReal(&r);
                            rsslDoubleToReal(&r, &d, rsslHint);
                        }

                        RsslRet ret;
                        // and encode into the message
                        if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&r)) < RSSL_RET_SUCCESS)
                        {
                            t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                rsslRetCodeToString(ret), ret,
                                sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                            encodeFail_ = true;
                        }
                    }

                    mamaPrice_destroy(p);
                }
                break;

            case MAMA_FIELD_TYPE_I8:
            case MAMA_FIELD_TYPE_U8:
            case MAMA_FIELD_TYPE_I16:
            case MAMA_FIELD_TYPE_U16:
            case MAMA_FIELD_TYPE_I32:
            case MAMA_FIELD_TYPE_U32:
            case MAMA_FIELD_TYPE_I64:
            case MAMA_FIELD_TYPE_U64:
                // otherwise, if its an int type then encode with 0 exponent
                {

                    //    RsslInt64 intVal;
                    mama_i64_t intVal;  // gcc seems happier with this
                    status = mamaMsgField_getI64(field, &intVal);
                    if (status != MAMA_STATUS_OK)
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getF64\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                    else
                    {
                        RsslReal r;
                        r.isBlank = false;
                        r.value = intVal;
                        r.hint = RSSL_RH_EXPONENT0;

                        RsslRet ret;
                        // and encode into the message
                        if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&r)) < RSSL_RET_SUCCESS)
                        {
                            t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                rsslRetCodeToString(ret), ret,
                                sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                            encodeFail_ = true;
                        }
                    }
                }
                break;

            case MAMA_FIELD_TYPE_F64:
                {
                    // It's a double
                    mama_f64_t dblVal;
                    status = mamaMsgField_getF64(field, &dblVal);
                    if (status != MAMA_STATUS_OK)
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getF64\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                    else
                    {
                        RsslBuffer buffer;
                        buffer.length = 64;
                        buffer.data = (char *) alloca(buffer.length + 1);
                        snprintf(buffer.data, buffer.length, "%f", dblVal);

                        RsslReal r;
                        rsslClearReal(&r);
                        rsslNumericStringToReal(&r, &buffer);

                        RsslRet ret;
                        // and encode into the message
                        if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&r)) < RSSL_RET_SUCCESS)
                        {
                            t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                rsslRetCodeToString(ret), ret,
                                sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                            encodeFail_ = true;
                        }
                    }
                }
                break;

            case MAMA_FIELD_TYPE_F32:
                {
                    mama_f32_t floatVal;
                    status = mamaMsgField_getF32(field, &floatVal);
                    if (status != MAMA_STATUS_OK)
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getF32\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                    else
                    {
                        RsslBuffer buffer;
                        buffer.length = 64;
                        buffer.data = (char *) alloca(buffer.length + 1);
                        snprintf(buffer.data, buffer.length, "%f", floatVal);

                        RsslReal r;
                        rsslClearReal(&r);
                        rsslNumericStringToReal(&r, &buffer);

                        RsslRet ret;
                        // and encode into the message
                        if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&r)) < RSSL_RET_SUCCESS)
                        {
                            t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                rsslRetCodeToString(ret), ret,
                                sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                            encodeFail_ = true;
                        }
                    }
                }
                break;

            default:
                t42log_warn("Conversion error: %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed converting to RSSL_REAL\n",
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;
                break;

            }

            break;
        case RSSL_DT_DATE:
            {
                mama_status status;
                mamaDateTime d;
                mamaDateTime_create(&d);
                status = mamaMsgField_getDateTime(field, d);
                if (status == MAMA_STATUS_OK )
                {
                    // build an rssl date from the mama datetime
                    RsslDate dt;
                    rsslClearDate(&dt);    // do we need to do this?

                    mama_bool_t hasDate = false;
                    mamaDateTime_hasDate(d, &hasDate);
                    if (hasDate)
                    {
                        struct tm tmStruct;
                        mama_status status = mamaDateTime_getStructTm(d, &tmStruct);
                        if (MAMA_STATUS_OK == status)
                        {
                            dt.year = tmStruct.tm_year + 1900;
                            dt.month = tmStruct.tm_mon + 1;
                            dt.day = tmStruct.tm_mday;
                        }
                    }

                    RsslRet ret;
                    // and encode into the message
                    if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&dt)) < RSSL_RET_SUCCESS)
                    {
                        t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                            rsslRetCodeToString(ret), ret,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
                else
                {
                    t42log_warn("Conversion error: %s.%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getDateTime\n",
                        mamaStatus_stringForStatus(status), status,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }

                mamaDateTime_destroy(d);
            }

            break;
        case RSSL_DT_TIME:
            {
                mamaDateTime d;
                mamaDateTime_create(&d);
                status = mamaMsgField_getDateTime(field, d);
                if (status == MAMA_STATUS_OK )
                {
                    // build an rssl date from the mama datetime
                    RsslTime t;
                    rsslClearTime(&t);    // do we need to do this?

                    mama_bool_t hasTime = false;
                    mamaDateTime_hasTime(d, &hasTime);
                    if (hasTime)
                    {
                        mama_i64_t seconds = 0;
                        mama_u32_t nanoseconds = 0;
                        mamaDateTime_getEpochTimeExt(d, &seconds, &nanoseconds);

                        struct tm tmStruct;
                        mama_status status = mamaDateTime_getStructTm(d, &tmStruct);
                        if (MAMA_STATUS_OK == status)
                        {
                            t.hour = tmStruct.tm_hour;
                            t.minute = tmStruct.tm_min;
                            t.second = tmStruct.tm_sec;

                            t.millisecond = nanoseconds / 1e+06;
                            t.microsecond = (nanoseconds - t.millisecond * 1e+06) / 1e+03;
                            t.nanosecond = nanoseconds - t.millisecond * 1e+06 - t.microsecond * 1e+03;
                        }
                    }
                    else
                    {
                        rsslBlankTime(&t);
                    }

                    RsslRet ret;
                    // and encode into the message
                    if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&t)) < RSSL_RET_SUCCESS)
                    {
                        t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                            rsslRetCodeToString(ret), ret,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
                else
                {
                    t42log_warn("Conversion error: %s.%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getDateTime\n",
                        mamaStatus_stringForStatus(status), status,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }

                mamaDateTime_destroy(d);
            }

            break;
        case RSSL_DT_DATETIME:
            {
                mamaDateTime d;
                mamaDateTime_create(&d);
                status = mamaMsgField_getDateTime(field, d);
                if (status == MAMA_STATUS_OK)
                {
                    // build an rssl date from the mama datetime
                    RsslDateTime dt;
                    rsslClearDateTime(&dt);    // do we need to do this?

                    struct tm tmStruct;
                    mama_status status = mamaDateTime_getStructTm(d, &tmStruct);
                    if (MAMA_STATUS_OK == status)
                    {
                        dt.date.year = tmStruct.tm_yday + 1900;
                        dt.date.month = tmStruct.tm_mon + 1;
                        dt.date.day = tmStruct.tm_mday;

                        mama_bool_t hasTime = false;
                        mamaDateTime_hasTime(d, &hasTime);
                        if (hasTime)
                        {
                            mama_i64_t seconds = 0;
                            mama_u32_t nanoseconds = 0;
                            mamaDateTime_getEpochTimeExt(d, &seconds, &nanoseconds);

                            dt.time.hour = tmStruct.tm_hour;
                            dt.time.minute = tmStruct.tm_min;
                            dt.time.second = tmStruct.tm_sec;

                            dt.time.millisecond = nanoseconds / 1e+06;
                            dt.time.microsecond = (nanoseconds - dt.time.millisecond * 1e+06) / 1e+03;
                            dt.time.nanosecond = nanoseconds - dt.time.millisecond * 1e+06 - dt.time.microsecond * 1e+03;
                        }
                        else
                        {
                            rsslBlankTime(&dt.time);
                        }
                    }

                    RsslRet ret;
                    // and encode into the message
                    if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&dt)) < RSSL_RET_SUCCESS)
                    {
                        t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                            rsslRetCodeToString(ret), ret,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
                else
                {
                    t42log_warn("Conversion error: %s.%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getDateTime\n",
                        mamaStatus_stringForStatus(status), status,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }

                mamaDateTime_destroy(d);
            }
            break;

        case RSSL_DT_QOS:
        case RSSL_DT_STATE:
        case RSSL_DT_ENUM:

            // if the mama field is an int then check its a valid enum value and use it direct or of its a string
            // then convert to the enum
            switch(fldType)
            {
            case MAMA_FIELD_TYPE_STRING:
                {
                    // convert from string to enum value
                    //mama_status status;

                    // first get the string value
                    const char * stringVal;
                    status = mamaMsgField_getString(field, &stringVal);
                    if (status == MAMA_STATUS_OK)
                    {

                        size_t stringLen = strlen(stringVal);

                        // now we need to walk the enum table to find a match
                        RsslEnumTypeTable * enumTable = rmdsDictionary_->entriesArray[rmdsFid]->pEnumTypeTable;
                        RsslEnum numEntries = enumTable->maxValue + 1;

                        // now we need to walk the set of strings in the enum table to try find a match
                        // (it goes without saying that mama clients will be more efficient if they
                        // use the enumerators rather than string values)
                        RsslEnum enumVal;
                        bool found = false;
                        for (int index = 0; index < numEntries; index++)
                        {
                            RsslEnumType * enumDef = enumTable->enumTypes[index];

                            // handle enumdef NULL - this is because the set of values is not contiguous
                            if (enumDef == 0)
                            {
                                // just try the next value
                                continue;
                            }

                            // note that the enum strings are case sensitive (for example in the currency enum we have GBP and GBp
                            // also we need to ensure that members of the enum set are not substrings of other members, so check the length too
                            if (stringLen == enumDef->display.length && ::strncmp(stringVal, enumDef->display.data, enumDef->display.length) == 0)
                            {
                                enumVal = enumDef->value;
                                found = true;
                                break;
                            }
                        }


                        if (!found)
                        {
                            // Output message only once for each fid
                            if (suppressBadEnumWarnings_.insert(fid).second)
                            {
                                t42log_warn("PUB: RSSL_DT_ENUM: %s.%s unknown enum string='%s' for fid %d",
                                    sourceName_.c_str(), symbol_.c_str(), stringVal, fid);
                            }

                            enumVal = atoi(stringVal);
                        }

                        if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&enumVal)) < RSSL_RET_SUCCESS)
                        {
                            t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                rsslRetCodeToString(ret), ret,
                                sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                            encodeFail_ = true;
                        }

                    }
                    else
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getString\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
                break;

            case MAMA_FIELD_TYPE_I8:
            case MAMA_FIELD_TYPE_U8:
            case MAMA_FIELD_TYPE_I16:
            case MAMA_FIELD_TYPE_U16:
            case MAMA_FIELD_TYPE_I32:
            case MAMA_FIELD_TYPE_U32:
            case MAMA_FIELD_TYPE_I64:
            case MAMA_FIELD_TYPE_U64:
                // otherwise, if its an int type then encode with 0 exponent
                {
                    RsslEnum enumVal;
                    mama_status status = mamaMsgField_getU16(field, &enumVal);
                    if (status == MAMA_STATUS_OK)
                    {
                        // check the enum value is valid
                        RsslEnumType *pEnumType = getFieldEntryEnumType( rmdsDictionary_->entriesArray[rmdsFid], enumVal);
                        if (pEnumType != 0)
                        {
                            // and encode into the message
                            if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&enumVal)) < RSSL_RET_SUCCESS)
                            {
                                t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                                    rsslRetCodeToString(ret), ret,
                                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                                encodeFail_ = true;
                            }
                        }
                        else if (suppressBadEnumWarnings_.insert(fid).second) // Output message only once for each fid
                        {
                            t42log_warn("Invalid enum value %d specified for fid %d", enumVal, rmdsFid);
                        }
                    }
                    else
                    {
                        t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getU16\n",
                            mamaStatus_stringForStatus(status), status,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
                break;

            default:
                t42log_warn("Conversion error: %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed converting to enum\n",
                    sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                    rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                encodeFail_ = true;

            }

            break;

        case RSSL_DT_BUFFER:
        case RSSL_DT_ASCII_STRING:
        case RSSL_DT_UTF8_STRING:
        case RSSL_DT_RMTES_STRING:
            {
                const char * stringVal;
                status = mamaMsgField_getString(field, &stringVal);
                if (status != MAMA_STATUS_OK)
                {
                    t42log_warn("Conversion error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling mamaMsgField_getString\n",
                        mamaStatus_stringForStatus(status), status,
                        sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                        rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                    encodeFail_ = true;
                }
                else
                {
                    // stick it in an RsslBuffer for encoding
                    RsslBuffer buf;
                    buf.data = (char*) stringVal;
                    buf.length = (RsslUInt32) strlen(stringVal);

                    RsslRet ret;
                    // and encode into the message
                    if ((ret = rsslEncodeFieldEntry(&itEncode_, &fieldEntry, (void*)&buf)) < RSSL_RET_SUCCESS)
                    {
                        t42log_warn("Encoding error: %s/%d %s.%s field=%s mamaFid=%d mamaType=%s/%d rsslType=%s/%d failed calling rsslEncodeFieldEntry\n",
                            rsslRetCodeToString(ret), ret,
                            sourceName_.c_str(), symbol_.c_str(), fname, fid, mamaFieldTypeToString(fldType), fldType,
                            rsslDataTypeToString(fieldEntry.dataType), fieldEntry.dataType);
                        encodeFail_ = true;
                    }
                }
            }
            break;

        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
RsslRealHints UPAFieldEncoder::MamaPrecisionToRsslHint(mamaPricePrecision p, uint16_t fid)
{
    switch(p)
    {
    case MAMA_PRICE_PREC_UNKNOWN:
        return RSSL_RH_EXPONENT_2;

    case MAMA_PRICE_PREC_10:
        return RSSL_RH_EXPONENT_1;

    case MAMA_PRICE_PREC_100:
        return RSSL_RH_EXPONENT_2;

    case MAMA_PRICE_PREC_1000:
        return RSSL_RH_EXPONENT_3;

    case MAMA_PRICE_PREC_10000:
        return RSSL_RH_EXPONENT_4;

    case MAMA_PRICE_PREC_100000:
        return RSSL_RH_EXPONENT_5;

    case MAMA_PRICE_PREC_1000000:
        return RSSL_RH_EXPONENT_6;

    case MAMA_PRICE_PREC_10000000:
        return RSSL_RH_EXPONENT_7;

    case MAMA_PRICE_PREC_100000000:
        return RSSL_RH_EXPONENT_8;

    case MAMA_PRICE_PREC_1000000000:
        return RSSL_RH_EXPONENT_9;

    case MAMA_PRICE_PREC_10000000000:
        return RSSL_RH_EXPONENT_10;

    case MAMA_PRICE_PREC_INT:
        return RSSL_RH_EXPONENT0;

    // These are the fractional prices
    case MAMA_PRICE_PREC_DIV_2:
        return RSSL_RH_FRACTION_2;

    case MAMA_PRICE_PREC_DIV_4:
        return RSSL_RH_FRACTION_4;

    case MAMA_PRICE_PREC_DIV_8:
        return RSSL_RH_FRACTION_8;

    case MAMA_PRICE_PREC_DIV_16:
        return RSSL_RH_FRACTION_16;

    case MAMA_PRICE_PREC_DIV_32:
        return RSSL_RH_FRACTION_32;

    case MAMA_PRICE_PREC_DIV_64:
        return RSSL_RH_FRACTION_64;

    case MAMA_PRICE_PREC_DIV_128:
        return RSSL_RH_FRACTION_128;

    case MAMA_PRICE_PREC_DIV_256:
        return RSSL_RH_FRACTION_256;

    default:
        t42log_info("Unhandled mamaPricePrecisionValue %d converting fid %d\n", p, fid );
        return RSSL_RH_EXPONENT0;
    }
}
