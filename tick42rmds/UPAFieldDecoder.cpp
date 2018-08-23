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

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "stdafx.h"
#include "UPAFieldDecoder.h"
#include "UPADecodeUtils.h"
#include "utils/time.h"

using namespace std;

RsslRet UPAFieldDecoder::DecodeFieldEntry(RsslFieldEntry* fEntry, RsslDecodeIterator *dIter, mamaMsg msg)
{
    RsslRet ret = 0;
    RsslDictionaryEntry* dictionaryEntry = NULL;

    /* get dictionary entry */
    if (!dictionary_->entriesArray)
    {
        t42log_warn("No dictionary");
        return RSSL_RET_SUCCESS;
    }
    else
    {
        dictionaryEntry = dictionary_->entriesArray[fEntry->fieldId];
    }

    /* return if no entry found */
    if (!dictionaryEntry)
    {
        return RSSL_RET_SUCCESS;
    }

    RsslDataType dataType = dictionaryEntry->rwfType;

    FindFieldResult findFieldResult = fieldmap_->GetTranslatedField(fEntry->fieldId);
    if (!findFieldResult.first)
    {
        return RSSL_RET_SUCCESS;
    }

    const MamaField_t& mamaField = findFieldResult.second;

    // insert the field into the message according to the type
    switch (dataType)
    {
    case RSSL_DT_UINT:
        {
            RsslUInt64 UIntVal = 0;
            if ((ret = rsslDecodeUInt(dIter, &UIntVal)) == RSSL_RET_SUCCESS)
            {
                AddRsslUintToMsg(msg, mamaField, UIntVal, fEntry->fieldId);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeUInt() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // set to 0 for missing data
                AddRsslUintToMsg(msg, mamaField, 0, fEntry->fieldId);
            }
            break;
        }

    case RSSL_DT_INT:
        {
            RsslInt64 intVal = 0;
            if ((ret = rsslDecodeInt(dIter, &intVal)) == RSSL_RET_SUCCESS)
            {
                AddRsslIntToMsg(msg, mamaField, intVal, fEntry->fieldId);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeInt() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // set to 0 for missing data
                AddRsslIntToMsg(msg, mamaField, 0, fEntry->fieldId);
            }
            break;
        }

    case RSSL_DT_FLOAT:
        {
            RsslFloat floatVal = 0;
            if ((ret = rsslDecodeFloat(dIter, &floatVal)) == RSSL_RET_SUCCESS)
            {
                AddRsslFloatToMsg(msg, mamaField, floatVal, fEntry->fieldId);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeFloat() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // set to 0 for missing data
                AddRsslFloatToMsg(msg, mamaField, 0.0, fEntry->fieldId);
            }
            break;
        }

    case RSSL_DT_DOUBLE:
        {
            RsslDouble doubleVal = 0;
            if ((ret = rsslDecodeDouble(dIter, &doubleVal)) == RSSL_RET_SUCCESS)
            {
                AddRsslDoubleToMsg(msg, mamaField, doubleVal, RSSL_RH_EXPONENT0, fEntry->fieldId);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeDouble() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // set to 0 for missing data
                AddRsslDoubleToMsg(msg, mamaField, 0.0, RSSL_RH_EXPONENT0, fEntry->fieldId);
            }
            break;
        }

    case RSSL_DT_REAL:
        {
            RsslReal realVal = RSSL_INIT_REAL;
            if ((ret = rsslDecodeReal(dIter, &realVal)) == RSSL_RET_SUCCESS)
            {
                // need to look at marketfeed type and decide whether this is a price or something else
                // this seems to be more reliable than using the hint field in the rsslReal
                if (dictionaryEntry->fieldType == RSSL_MFEED_INTEGER)
                {
                    // then its an integer, (presumably a size)
                    RsslDouble dblVal;
                    rsslRealToDouble(&dblVal, &realVal);
                    RsslInt64 intVal = RsslInt64(dblVal);

                    AddRsslIntToMsg(msg, mamaField, intVal, fEntry->fieldId);
                }
                else if (dictionaryEntry->fieldType == RSSL_MFEED_PRICE)
                {
                    // otherwise its a price
                    RsslDouble dblVal;
                    rsslRealToDouble(&dblVal, &realVal);

                    // this will add as a price of thats the fielt type its mapped to
                    AddRsslDoubleToMsg(msg, mamaField, dblVal, realVal.hint, fEntry->fieldId);
                }
                else
                {
                    RsslBuffer realBuff;
                    // all else fails, render as a string
                    realBuff.data = (char*)alloca(35);
                    realBuff.length = 35;
                    rsslRealToString(&realBuff, &realVal);
                    AddRsslStringToMsg(msg, mamaField, realBuff.data, fEntry->fieldId);
                }

            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeReal() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                if (dictionaryEntry->fieldType == RSSL_MFEED_INTEGER)
                {
                    // missing value set to 0
                    AddRsslIntToMsg(msg, mamaField, 0, fEntry->fieldId);
                }
                else if (dictionaryEntry->fieldType == RSSL_MFEED_PRICE)
                {
                    // missing value set to 0 price
                    // this will add as a price of thats the field type its mapped to
                    AddRsslDoubleToMsg(msg, mamaField, 0.0, RSSL_RH_EXPONENT0, fEntry->fieldId);
                }
                else
                {
                    // missing value set as empty string
                    AddRsslStringToMsg(msg, mamaField, "", fEntry->fieldId);
                }
            }
            break;
        }

    case RSSL_DT_ENUM:
        {
            RsslEnum enumVal;
            if ((ret = rsslDecodeEnum(dIter, &enumVal)) == RSSL_RET_SUCCESS)
            {
                // look up the enum type for this fid and field value
                RsslEnumType *pEnumType = getFieldEntryEnumType(dictionary_->entriesArray[fEntry->fieldId], enumVal);
                if (pEnumType != 0)
                {
                    // use the string value from the enum type
                    std::string strEnumVal(pEnumType->display.data, pEnumType->display.length);
                    AddRsslStringToMsg(msg, mamaField, strEnumVal.c_str(), fEntry->fieldId);
                }
                else
                {
                    // enum lookup failed, just put the number value into the message
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d", enumVal);
                    AddRsslStringToMsg(msg, mamaField, buf, fEntry->fieldId);
                }
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeEnum() failed with return code: %d\n", ret);
                return ret;
            }
            else
            {
                // missing data just set to 0
                AddRsslStringToMsg(msg, mamaField, "0", fEntry->fieldId);
            }

            break;
        }

    case RSSL_DT_DATE:
        {
            RsslDateTime dateTimeVal = RSSL_INIT_DATETIME;
            if ((ret = rsslDecodeDate(dIter, &dateTimeVal.date)) == RSSL_RET_SUCCESS)
            {
                // Build a MAMA DateTime from the field value
                AddRsslDateToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeDate() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // missing date, set blank
                rsslClearDateTime(&dateTimeVal);
                AddRsslDateToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId, true);
            }
            break;
        }

    case RSSL_DT_TIME:
        {
            RsslDateTime dateTimeVal = RSSL_INIT_DATETIME;
            if ((ret = rsslDecodeTime(dIter, &dateTimeVal.time)) == RSSL_RET_SUCCESS)
            {
                AddRsslTimeToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId);

            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeTime() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // missing time, set to midnight

                // just clear the time to set to midnight
                rsslClearDateTime(&dateTimeVal);
                AddRsslTimeToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId, true);
            }
            break;
        }

    case RSSL_DT_DATETIME:
        {
            RsslDateTime dateTimeVal;
            if ((ret = rsslDecodeDateTime(dIter, &dateTimeVal)) == RSSL_RET_SUCCESS)
            {
                AddRsslDateTimeToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId);

            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeDateTime() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // missing datetime , set to Jan 1 1970, midnight
                // just clear the time to set to midnight, then set to jan 1 1970
                rsslClearDateTime(&dateTimeVal);
                dateTimeVal.date.day = 1;
                dateTimeVal.date.month = 1;
                dateTimeVal.date.year = 1970;
                AddRsslDateTimeToMsg(msg, mamaField, dateTimeVal, fEntry->fieldId, true);
            }
            break;
        }

    case RSSL_DT_QOS:
        {
            RsslQos qosVal= RSSL_INIT_QOS;
            RsslBuffer qosBuff;
            if ((ret = rsslDecodeQos(dIter, &qosVal)) == RSSL_RET_SUCCESS)
            {
                qosBuff.data = (char*)alloca(100);
                qosBuff.length = 100;
                rsslQosToString(&qosBuff, &qosVal);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeQos() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            break;
        }

    case RSSL_DT_STATE:
        {
            RsslState stateVal;
            RsslBuffer stateBuff;
            if ((ret = rsslDecodeState(dIter, &stateVal)) == RSSL_RET_SUCCESS)
            {
                int stateBufLen = 80;
                if (stateVal.text.data)
                    stateBufLen += stateVal.text.length;
                stateBuff.data = (char*)alloca(stateBufLen);
                stateBuff.length = stateBufLen;
                rsslStateToString(&stateBuff, &stateVal);
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeState() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            break;
        }

        // Currently don't support array
    case RSSL_DT_ARRAY:
        break;

        // all other types treat as string
    case RSSL_DT_BUFFER:
    case RSSL_DT_ASCII_STRING:
    case RSSL_DT_UTF8_STRING:
    case RSSL_DT_RMTES_STRING:
        {
            RsslBuffer bufferVal;
            if ((ret = rsslDecodeBuffer(dIter, &bufferVal)) == RSSL_RET_SUCCESS)
            {
                // need to null terminate so extact onto a string
                string strVal(bufferVal.data, bufferVal.length);
                mamaMsg_addString(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid,strVal.c_str());
            }
            else if (ret != RSSL_RET_BLANK_DATA)
            {
                t42log_error("rsslDecodeBuffer() %s.%s %s failed with return code: %d\n", sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str(), ret);
                return ret;
            }
            else
            {
                // missing value set as  empty string
                mamaMsg_addString(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid, "");
            }
            break;
        }

    default:
        t42log_warn("Unsupported data type=%d %s.%s %s\n", dataType, sourceName_.c_str(), symbol_.c_str(), mamaField.mama_field_name.c_str());
        break;
    }

    return RSSL_RET_SUCCESS;
}

// decode a book field
//
// for this we are concerned with specific fields rather than a generic type based conversion

RsslRet UPAFieldDecoder::DecodeBookFieldEntry(RsslFieldEntry* fEntry, RsslDecodeIterator *dIter, const UPABookEntry_ptr_t& entry)
{
    RsslRet ret = 0;
    RsslDataType dataType = RSSL_DT_UNKNOWN;
    RsslUInt64 fidUIntValue = 0;
    RsslInt64 fidIntValue = 0;
    RsslFloat tempFloat = 0;
    RsslDouble tempDouble = 0;
    RsslReal fidRealValue = RSSL_INIT_REAL;
    RsslEnum fidEnumValue;
    RsslFloat fidFloatValue = 0;
    RsslDouble fidDoubleValue = 0;
    RsslQos fidQosValue = RSSL_INIT_QOS;
    RsslBuffer bufferVal;

    //    RsslDataDictionary* dictionary = consumer_->RsslDictionary()->RsslDictionary();
    RsslDictionaryEntry* dictionaryEntry = NULL;

    /* get dictionary entry */
    if (!dictionary_->entriesArray)
    {
        return RSSL_RET_SUCCESS;
    }
    else
    {
        dictionaryEntry = dictionary_->entriesArray[fEntry->fieldId];
    }

    /* return if no entry found */
    if (!dictionaryEntry)
    {
        return RSSL_RET_SUCCESS;
    }

    dataType = dictionaryEntry->rwfType;

    FindFieldResult findFieldResult = fieldmap_->GetTranslatedField(fEntry->fieldId);
    if (!findFieldResult.first)
    {
        return RSSL_RET_SUCCESS;
    }

    const MamaField_t& mamaField = findFieldResult.second;

    // we can either rely on hard coded rmds fids here or use the fieldmap translated names into mama field names
    // in this version we'll work with rmds fids but we could change that

    switch(fEntry->fieldId)
    {
    case 3427:    // "ORDER_PRC" RsslReal

        if ((ret = rsslDecodeReal(dIter, &fidRealValue)) == RSSL_RET_SUCCESS)
        {
            entry->Price(fidRealValue);
        }
        break;

    case 3429:    // "ORDER_SIZE" Rsslreal
    case 4356:    // "ACCUMULATED SIZE"  for market by price

        if ((ret = rsslDecodeReal(dIter, &fidRealValue)) == RSSL_RET_SUCCESS)
        {
            if (fidRealValue.hint == RSSL_RH_EXPONENT0)
            {
                // we expect an int here so RSSL_RH_EXPONENT0
                entry->Size(fidRealValue.value);
            }
        }
        break;

    case 3430:    // "NUMBER OF ORDERS" UINT64

        if ((ret = rsslDecodeUInt(dIter, &fidUIntValue)) == RSSL_RET_SUCCESS)
        {
            entry->NumOrders(fidUIntValue);
        }
        break;

    case 3855:    // QUOTIM_MS    rsslU64
    case 6527:    // LV_TIM_MS     UINT64/INTEGER
        if ((ret = rsslDecodeUInt(dIter, &fidUIntValue)) == RSSL_RET_SUCCESS)
        {
            entry->Time(fidUIntValue);
        }
        break;

    case 3886:    // "ORDER_TONE" string
        ret = rsslDecodeBuffer(dIter, &bufferVal);
        if (ret == RSSL_RET_SUCCESS || ret == RSSL_RET_BLANK_DATA)
        {
            // need to null terminate so extact onto a string
            string strVal(bufferVal.data, bufferVal.length);
            entry->OrderTone(strVal);
        }
        break;

    case 3428:    // "ORDER_SIDE" string

        // order side is an enum, but lets just use the enum value here, rather than decoding to s string
        //       0    undefined. 1 Bid,  2 Ask
        if ((ret = rsslDecodeEnum(dIter, &fidEnumValue)) == RSSL_RET_SUCCESS)
        {
            if (fidEnumValue == 1)
            {
                entry->SideCode('B');
            }
            else if (fidEnumValue == 2)
            {
                entry->SideCode('A');
            }

            // it will have been initialized to 'Z' for unknown so no need to set that
        }
        break;

    case 212:    // "MKT_MKR_ID" string
    case 3435:    // "MMID" string
        ret = rsslDecodeBuffer(dIter, &bufferVal);
        if (ret == RSSL_RET_SUCCESS || ret == RSSL_RET_BLANK_DATA)
        {
            // need to null terminate so extact onto a string
            string strVal(bufferVal.data, bufferVal.length);
            entry->Mmid(strVal);
        }
        break;

    default:
        break;
    }

    if (ret == RSSL_RET_BLANK_DATA)
    {
        //t42log_info("<blank data>\n");
    }

    return RSSL_RET_SUCCESS;
}


mama_status UPAFieldDecoder::AddRsslUintToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslUInt64 UIntVal, RsslFieldId fid)
{
    switch (mamaField.mama_field_type)
    {

        // String
    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {
            char buffer[80];
#ifdef _WIN32  //TODO: in the future we should consider having our own PRIu64 and PRId64 instead of the ones in wombat/port.h which do not work with inttypes and __STDC_FORMAT_MACROS=1 -> hence the conditional code.
            snprintf(buffer, sizeof(buffer)-1, "%I64u" , UIntVal);
#else
            snprintf(buffer, sizeof(buffer)-1, "%llu" , UIntVal);
#endif
            return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, buffer);
        }

        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        {
            return mamaMsg_addBool(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal != 0);
        }

        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        if (CHAR_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type char - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addChar(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, char(UIntVal));

        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        if (INT8_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I8 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addI8(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int8_t(UIntVal));

        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        if (UINT8_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U8 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addU8(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint8_t(UIntVal));

        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        if (INT16_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addI16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int16_t(UIntVal));

        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        if (UINT16_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addU16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint16_t(UIntVal));

        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        if (INT32_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addI32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int32_t(UIntVal));

        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        if (UINT32_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addU32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint32_t(UIntVal));

        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        if (INT64_MAX <= UIntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I64 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, UIntVal);
        }
        return mamaMsg_addI64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int64_t(UIntVal));

        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // don't need size check / warning in this one
        return mamaMsg_addU64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid,UIntVal);

        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
    case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:
        // just convert to float
        return mamaMsg_addF32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (float)UIntVal);

        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        return mamaMsg_addF64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (double)UIntVal);


        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        {
            // create a mama price from the value rendered into double
            mamaPrice p;
            mamaPrice_create(&p);
            mamaPrice_setValue(p, double(UIntVal));

            mama_status stat = mamaMsg_addPrice(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid, p); //p is copied and should be destroyed later on!
            mamaPrice_destroy(p);
            return stat;
        }

        // 64 bit MAMA time
    case AS_MAMA_FIELD_TYPE_TIME:
        {
            // some rmds fields are time since midnight GMT in ms so if requested convert int to mamaDateTime
            mamaDateTime dt;
            mamaDateTime_create(&dt);
            UPADecodeUtils::DateTimeFromMidnightMs(dt, UIntVal);

            mama_status stat = mamaMsg_addDateTime(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dt); //dt is copied and should be destroyed later on!
            mamaDateTime_destroy(dt);
            return stat;
        }
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    };

    return MAMA_STATUS_OK;
}

mama_status UPAFieldDecoder::AddRsslIntToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslInt64 IntVal, RsslFieldId fid )
{
    switch (mamaField.mama_field_type)
    {

        // String
    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:

        {
            char buffer[80];
#ifdef _WIN32  //TODO: in the future we should create our own PRI64, the one from wombat/port.h does not work on linux. for more, see comment in AddRsslUintToMsg()
            snprintf(buffer, sizeof(buffer)-1, "%I64d" , IntVal);
#else
            snprintf(buffer, sizeof(buffer)-1, "%lld", IntVal);
#endif
            return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, buffer);
        }

        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        {
            return mamaMsg_addBool(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal != 0);
        }

        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        if (CHAR_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type char - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addChar(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, char(IntVal));

        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        if (INT8_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I8 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addI8(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int8_t(IntVal));

        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        if (UINT8_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U8 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addU8(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint8_t(IntVal));

        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        if (INT16_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addI16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int16_t(IntVal));

        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        if (UINT16_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addU16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint16_t(IntVal));

        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        if (INT32_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type I32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addI32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int32_t(IntVal));

        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        if (UINT32_MAX <= IntVal)
        {
            t42log_info("Field %s (fid %d) value %d is too big for mama field type U32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, IntVal);
        }
        return mamaMsg_addU32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint32_t(IntVal));

        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        // don't need size check / warning in this one
        return mamaMsg_addI64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int64_t(IntVal));

        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // don't need size check / warning in this one
        return mamaMsg_addU64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid,IntVal);

        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
    case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:
        // just convert to float
        return mamaMsg_addF32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (float)IntVal);

        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        return mamaMsg_addF64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (double)IntVal);


        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // create a mama price from the value rendered into double
        {
            mamaPrice p;
            mamaPrice_create(&p);
            mamaPrice_setValue(p, double(IntVal));

            mama_status stat = mamaMsg_addPrice(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid, p); //p is copied and should be destroyed later on!
            mamaPrice_destroy(p);
            return stat;
        }

        // 64 bit MAMA time
    case AS_MAMA_FIELD_TYPE_TIME:
        {
            // some rmds fields are time since midnight GMT in ms so if requested convert int to mamaDateTime
            mamaDateTime dt;
            mamaDateTime_create(&dt);
            UPADecodeUtils::DateTimeFromMidnightMs(dt, (RsslUInt64)IntVal);


            mama_status stat = mamaMsg_addDateTime(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dt); //dt is copied and should be destroyed later on!
            mamaDateTime_destroy(dt);
            return stat;


        }

        // doesn't really mean anything to convert rssl uint to mama date / time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    };

    return MAMA_STATUS_OK;

}

mama_status UPAFieldDecoder::AddRsslFloatToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslFloat floatVal, RsslFieldId fid)
{
    switch (mamaField.mama_field_type)
    {
        // String
    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {
            char buffer[80];
            sprintf(buffer, "%f", floatVal);
            return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, buffer);
        }

        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
    case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:
        // just convert to float
        return mamaMsg_addF32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);

        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        return mamaMsg_addF64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (double)floatVal);

        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // create a mama price from the value rendered into double
        {
            mamaPrice p;
            mamaPrice_create(&p);
            mamaPrice_setValue(p, double(floatVal));

            mama_status stat = mamaMsg_addPrice(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid, p); //p is copied and should be destroyed later on!
            mamaPrice_destroy(p);
            return stat;
        }

    case AS_MAMA_FIELD_TYPE_I16:
        if (INT16_MAX <= (int16_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addI16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int16_t(floatVal));

        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        if (UINT16_MAX <= (uint16_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addU16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint16_t(floatVal));

        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        if (INT32_MAX <= (int32_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addI32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int32_t(floatVal));

        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        if (UINT32_MAX <= (uint32_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addU32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint32_t(floatVal));

        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        if (INT64_MAX <= (int64_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I64 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addI64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int64_t(floatVal));

        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        if (UINT64_MAX <= (uint64_t)floatVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U64 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, floatVal);
        }
        return mamaMsg_addU64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint64_t(floatVal));

        // none of these conversions is really meaningful
        // (although its possible that there might be cases where we need to convert (with risk of truncation) from float to the longer int types
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
        // 64 bit MAMA time
    case AS_MAMA_FIELD_TYPE_TIME:
        // doesn't really mean anything to convert rssl uint to mama date / time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    };

    return MAMA_STATUS_OK;
}

mama_status UPAFieldDecoder::AddRsslDoubleToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslDouble dblVal, RsslUInt8 hint, RsslFieldId fid)
{
    switch (mamaField.mama_field_type)
    {
        // String
    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {
            char buffer[80];
            sprintf(buffer, "%f", dblVal);
            return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, buffer);
        }

        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
    case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:

        // just convert to float
        return mamaMsg_addF32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, (float)dblVal);

        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        return mamaMsg_addF64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);

        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // create a mama price from the value rendered into double
        {
            mamaPrice p;
            mamaPrice_create(&p);
            mamaPrice_setValue(p, dblVal);
            mamaPricePrecision prec = RsslHintToMamaPrecisionTo((RsslRealHints) hint, mamaField.mama_fid);
            mamaPrice_setPrecision(p, prec);

            mama_status stat = mamaMsg_addPrice(msg, mamaField.mama_field_name.c_str() ,mamaField.mama_fid, p); //p is copied and should be destroyed later on!
            mamaPrice_destroy(p);
            return stat;
        }

    case AS_MAMA_FIELD_TYPE_I16:
        if (INT16_MAX <= (int16_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addI16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int16_t(dblVal));

        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        if (UINT16_MAX <= (uint16_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U16 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addU16(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint16_t(dblVal));

        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        if (INT32_MAX <= (int32_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addI32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int32_t(dblVal));

        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        if (UINT32_MAX <= (uint32_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U32 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addU32(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint32_t(dblVal));

        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        if (INT64_MAX <= (int64_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type I64 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addI64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, int64_t(dblVal));

        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        if (UINT64_MAX <= (uint64_t)dblVal)
        {
            t42log_info("Field %s (fid %d) value %f is too big for mama field type U64 - truncated", mamaField.mama_field_name.c_str(), mamaField.mama_fid, dblVal);
        }
        return mamaMsg_addU64(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, uint64_t(dblVal));

        // none of these conversions is really meaningful
        // (although its possible that there might be cases where we need to convert (with risk of truncation) from float to the longer int types
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
        // 64 bit MAMA time
    case AS_MAMA_FIELD_TYPE_TIME:
        // doesn't really mean anything to convert rssl uint to mama date / time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    };

    return MAMA_STATUS_OK;
}

mama_status UPAFieldDecoder::AddRsslDateToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslDateTime dateVal, RsslFieldId fid, bool isBlank)
{
    if (returnDateTimeAsString_)
    {
        // If configured, return date time as string
        RsslBuffer dateTimeBuffer;
        dateTimeBuffer.data = (char*)alloca(50);
        dateTimeBuffer.length = 50;
        rsslDateTimeToString(&dateTimeBuffer, RSSL_DT_DATE, &dateVal);

        // assuming here that rsslDateTimeToString delivers a null terminated string
        return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
    }

    switch (mamaField.mama_field_type)
    {

    case AS_MAMA_FIELD_TYPE_TIME:
    case RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME:
    case RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME:
        {
            // Build a MAMA DateTime from the field value
            mamaDateTime dt;
            mamaDateTime_create(&dt);

            if (dateVal.date.year != 0 && dateVal.date.month != 0 && dateVal.date.day != 0)
            {
                // Add it not blank date
                mamaDateTime_setEpochTimeExt(dt, utils::time::GetSeconds(dateVal.date.year, dateVal.date.month, dateVal.date.day), 0);
                mamaDateTime_setHints(dt, MAMA_DATE_TIME_HAS_DATE);
            }
            mama_status stat = mamaMsg_addDateTime(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dt); //dt is copied and should be destroyed later on!
            mamaDateTime_destroy(dt);
            return stat;
        }

    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {
            if (isBlank)
            {
                return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, "" );
            }
            else
            {
                RsslBuffer dateTimeBuffer;
                dateTimeBuffer.data = (char*)alloca(50);
                dateTimeBuffer.length = 50;
                rsslDateTimeToString(&dateTimeBuffer, RSSL_DT_DATE, &dateVal);

                // assuming here that rsslDateTimeToString delivers a null terminated string
                return mamaMsg_addString(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
            }
        }


        // none of these other conversions is really meaningful but can add if necessary
        //
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // 64 bit MAMA time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    }
    return MAMA_STATUS_OK;

}

mama_status UPAFieldDecoder::AddRsslTimeToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslDateTime timeVal, RsslFieldId fid, bool isBlank)
{
    if (returnDateTimeAsString_)
    {
        // If configured, return date time as string
        RsslBuffer dateTimeBuffer;
        dateTimeBuffer.data = (char*)alloca(20);
        dateTimeBuffer.length = 20;
        snprintf(dateTimeBuffer.data, 20, "%02d:%02d:%02d",
            timeVal.time.hour, timeVal.time.minute, timeVal.time.second);
        return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
    }

    switch (mamaField.mama_field_type)
    {

    case AS_MAMA_FIELD_TYPE_TIME:
    case RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME:
        {
            mamaDateTime dt;
            mamaDateTime_create(&dt);

            // Build a MAMA DateTime from the field value
            if (isBlank)
            {
                // just clear the date/time
                mamaDateTime_clearDate(dt);
                mamaDateTime_clearTime(dt);
            }
            else
            {
                // Build a MAMA DateTime from the field value
                mamaDateTime_setTime(dt, timeVal.time.hour,timeVal.time.minute, timeVal.time.second, timeVal.time.millisecond * 1000 + timeVal.time.microsecond);
                mama_i64_t seconds = 0;
                mama_u32_t nanoseconds = 0;
                mamaDateTime_getEpochTimeExt(dt, &seconds, &nanoseconds);
                nanoseconds += timeVal.time.nanosecond;
                mamaDateTime_setEpochTimeExt(dt, seconds, nanoseconds);
                // as this is a time only field we dont want a date
                mamaDateTime_clearDate(dt);
            }
            mama_status stat = mamaMsg_addDateTime(msg, mamaField.mama_field_name.c_str(), mamaField.mama_fid, dt); //dt is copied and should be destroyed later on!
            mamaDateTime_destroy(dt);
            return stat;
        }


    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {
            if (isBlank)
            {
                return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid,"" );
            }
            else
            {
                RsslBuffer dateTimeBuffer;
                dateTimeBuffer.data = (char*)alloca(50);
                dateTimeBuffer.length = 50;
                rsslDateTimeToString(&dateTimeBuffer, RSSL_DT_TIME, &timeVal);
                // assuming here that rsslDateTimeToString delivers a null terminated string
                return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
            }
        }

        // none of these other conversions is really meaningful but can add if necessary
        //
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // 64 bit MAMA time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    }

    return MAMA_STATUS_OK;
}

mama_status UPAFieldDecoder::AddRsslDateTimeToMsg(mamaMsg msg, const MamaField_t& mamaField, RsslDateTime dateTimeVal, RsslFieldId fid, bool isBlank)
{
    if (returnDateTimeAsString_)
    {
        // If configured, return date time as string
        RsslBuffer dateTimeBuffer;
        dateTimeBuffer.data = (char*)alloca(50);
        dateTimeBuffer.length = 50;
        rsslDateTimeToString(&dateTimeBuffer, RSSL_DT_DATE, &dateTimeVal);

        // assuming here that rsslDateTimeToString delivers a null terminated string
        return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
    }

    switch (mamaField.mama_field_type)
    {

    case AS_MAMA_FIELD_TYPE_TIME:
    case RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME:
        {
            mamaDateTime dt;
            mamaDateTime_create(&dt);
            // Build a MAMA DateTime from the field value
            if (isBlank)
            {
                // just clear the date/time
                mamaDateTime_clearDate(dt);
                mamaDateTime_clearTime(dt);
            }
            else
            {
                mamaDateTime_setToMidnightToday(dt, NULL);
                // set the date
                if (dateTimeVal.date.year != 0 && dateTimeVal.date.month != 0 && dateTimeVal.date.day != 0)
                {
                    mamaDateTime_setEpochTimeExt(dt, utils::time::GetSeconds(dateTimeVal.date.year, dateTimeVal.date.month, dateTimeVal.date.day), 0);
                    mamaDateTime_setHints(dt, MAMA_DATE_TIME_HAS_DATE);
                }
                // set the time
                mamaDateTime_setTime(dt, dateTimeVal.time.hour, dateTimeVal.time.minute, dateTimeVal.time.second, dateTimeVal.time.millisecond * 1000 + dateTimeVal.time.microsecond);
                mama_i64_t seconds = 0;
                mama_u32_t nanoseconds = 0;
                mamaDateTime_getEpochTimeExt(dt, &seconds, &nanoseconds);
                nanoseconds += dateTimeVal.time.nanosecond;
                mamaDateTime_setEpochTimeExt(dt, seconds, nanoseconds);
            }
            mama_status stat = mamaMsg_addDateTime(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dt); //dt is copied and should be destroyed later on!
            mamaDateTime_destroy(dt);

            return stat;
        }

    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        {

            if (isBlank)
            {
                // just want an empty string
                return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, "" );
            }
            else
            {
                RsslBuffer dateTimeBuffer;
                dateTimeBuffer.data = (char*)alloca(50);
                dateTimeBuffer.length = 50;
                rsslDateTimeToString(&dateTimeBuffer, RSSL_DT_DATE, &dateTimeVal);

                // assuming here that rsslDateTimeToString delivers a null terminated string
                return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, dateTimeBuffer.data );
            }

        }

        // none of these other conversions is really meaningful but can add if necessary
        //
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // 64 bit MAMA time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    }

    return MAMA_STATUS_OK;


}

mama_status UPAFieldDecoder::AddRsslStringToMsg(mamaMsg msg, const MamaField_t& mamaField, const char* strVal, RsslFieldId fid)
{
    switch (mamaField.mama_field_type)
    {
    case AS_MAMA_FIELD_TYPE_STRING:
    case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:

        {
            return mamaMsg_addString(msg,  mamaField.mama_field_name.c_str(), mamaField.mama_fid, strVal);
        }

        // none of these other conversions is really meaningful but can add if necessary. There are many problems associated with converting strings to most of these types
        // so just don't do it. If there are caes where it is required, we can re-consider
        //
    case AS_MAMA_FIELD_TYPE_TIME:
        // Boolean
    case AS_MAMA_FIELD_TYPE_BOOL:
        // Character
    case AS_MAMA_FIELD_TYPE_CHAR :
        // Signed 8 bit integer
    case AS_MAMA_FIELD_TYPE_I8:
        // Unsigned byte
    case AS_MAMA_FIELD_TYPE_U8:
        // Signed 16 bit integer
    case AS_MAMA_FIELD_TYPE_I16:
        // Unsigned 16 bit integer
    case AS_MAMA_FIELD_TYPE_U16:
        // Signed 32 bit integer
    case AS_MAMA_FIELD_TYPE_I32:
        // Unsigned 32 bit integer
    case AS_MAMA_FIELD_TYPE_U32:
        // Signed 64 bit integer
    case AS_MAMA_FIELD_TYPE_I64:
        // Unsigned 64 bit integer
    case AS_MAMA_FIELD_TYPE_U64:
        // 32 bit float
    case AS_MAMA_FIELD_TYPE_F32:
        // 64 bit float
    case AS_MAMA_FIELD_TYPE_F64:
        // MAMA price
    case AS_MAMA_FIELD_TYPE_PRICE:
        // 64 bit MAMA time
    default:
        t42log_warn("Unhandled field type %d for field %s(fid %d)", mamaField.mama_field_type, mamaField.mama_field_name.c_str(), mamaField.mama_fid );
        break;
    }

    return MAMA_STATUS_OK;
}

mamaPricePrecision UPAFieldDecoder::RsslHintToMamaPrecisionTo(RsslRealHints p, uint16_t fid)
{
    switch(p)
    {
    case RSSL_RH_EXPONENT0:
        return MAMA_PRICE_PREC_INT;

    case RSSL_RH_EXPONENT_1:
        return MAMA_PRICE_PREC_10;

    case RSSL_RH_EXPONENT_2:
        return MAMA_PRICE_PREC_100;

    case RSSL_RH_EXPONENT_3:
        return MAMA_PRICE_PREC_1000;

    case RSSL_RH_EXPONENT_4:
        return MAMA_PRICE_PREC_10000;

    case RSSL_RH_EXPONENT_5:
        return MAMA_PRICE_PREC_100000;

    case RSSL_RH_EXPONENT_6:
        return MAMA_PRICE_PREC_1000000;

    case RSSL_RH_EXPONENT_7:
        return MAMA_PRICE_PREC_10000000;

    case RSSL_RH_EXPONENT_8:
        return MAMA_PRICE_PREC_100000000;

    case RSSL_RH_EXPONENT_9:
        return MAMA_PRICE_PREC_1000000000;

    case RSSL_RH_EXPONENT_10:
    case RSSL_RH_EXPONENT_11:
    case RSSL_RH_EXPONENT_12:
    case RSSL_RH_EXPONENT_13:
    case RSSL_RH_EXPONENT_14:
        return MAMA_PRICE_PREC_10000000000;

    // These occur when the original price was in fraction form, e.g., "102 114/256"
    case RSSL_RH_FRACTION_1:        /*!< Fractional denominator operation, equivalent to 1/1. Value undergoes no change. */
        return MAMA_PRICE_PREC_10;
    case RSSL_RH_FRACTION_2:        /*!< Fractional denominator operation, equivalent to 1/2. Adds or removes a denominator of 2. */
        return MAMA_PRICE_PREC_10;
    case RSSL_RH_FRACTION_4:        /*!< Fractional denominator operation, equivalent to 1/4. Adds or removes a denominator of 4. */
        return MAMA_PRICE_PREC_100;
    case RSSL_RH_FRACTION_8:        /*!< Fractional denominator operation, equivalent to 1/8. Adds or removes a denominator of 8. */
        return MAMA_PRICE_PREC_1000;
    case RSSL_RH_FRACTION_16:        /*!< Fractional denominator operation, equivalent to 1/16. Adds or removes a denominator of 16. */
        return MAMA_PRICE_PREC_10000;
    case RSSL_RH_FRACTION_32:        /*!< Fractional denominator operation, equivalent to 1/32. Adds or removes a denominator of 32. */
        return MAMA_PRICE_PREC_100000;
    case RSSL_RH_FRACTION_64:        /*!< Fractional denominator operation, equivalent to 1/64. Adds or removes a denominator of 64. */
        return MAMA_PRICE_PREC_1000000;
    case RSSL_RH_FRACTION_128:        /*!< Fractional denominator operation, equivalent to 1/128. Adds or removes a denominator of 128. */
        return MAMA_PRICE_PREC_10000000;
    case RSSL_RH_FRACTION_256:        /*!< Fractional denominator operation, equivalent to 1/256. Adds or removes a denominator of 256. */
        return MAMA_PRICE_PREC_100000000;

    default:
        t42log_info("Unhandled RsslHint %d converting fid %d\n", p, fid );
        return MAMA_PRICE_PREC_UNKNOWN;
    }
}
