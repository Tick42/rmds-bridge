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
#include "UPAAsMamaFieldType.h"

// map RSSL field types to mama field types

bool UpaToMamaFieldType(RsslUInt8 rwfType, RsslUInt8 mfType, UpaAsMamaFieldType &to)
{
    switch (rwfType)
    {
    default:    
    case RSSL_DT_QOS:    
    case RSSL_DT_STATE:
    case RSSL_DT_ARRAY: //TODO: Later on should covert to mama arrays. For an example of array decoding, see fieldListEncDec.c
        to = AS_MAMA_FIELD_TYPE_UNKNOWN;
        return false;
    case RSSL_DT_BUFFER: //TODO: MAMA_FIELD_TYPE_OPAQUE might be a good candidate
    case RSSL_DT_MAP:
        to = AS_MAMA_FIELD_TYPE_OPAQUE;
        break;
    case RSSL_DT_UINT:
        to = AS_MAMA_FIELD_TYPE_U64;
        break;
    case RSSL_DT_INT:
        to = AS_MAMA_FIELD_TYPE_I64;
        break;
    case RSSL_DT_FLOAT:
        to = AS_MAMA_FIELD_TYPE_F32;
        break;
    case RSSL_DT_DOUBLE:
        to = AS_MAMA_FIELD_TYPE_F64;    
        break;
    case RSSL_DT_REAL:
        // we need to look at the mf type. if its price or integer then set the target type as such, otherwise
        // fall back to string
        if (RSSL_MFEED_PRICE == mfType)
        {
            to = AS_MAMA_FIELD_TYPE_PRICE;
        }
        else if (RSSL_MFEED_INTEGER == mfType)
        {
            to = AS_MAMA_FIELD_TYPE_I64;
        }
        else
        {
        to = AS_MAMA_FIELD_TYPE_STRING;
        }
        break;
    case RSSL_DT_ENUM:
        to = RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING;
        break;
    case RSSL_DT_DATE:
        to = RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME;
        break;
    case RSSL_DT_TIME:
        to = RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME;
        break;
    case RSSL_DT_DATETIME:
        to = AS_MAMA_FIELD_TYPE_TIME;
        break;
        break;
    case RSSL_DT_UTF8_STRING:  // TODO: check down conversion in mama
    case RSSL_DT_RMTES_STRING: // TODO: check down conversion in mama
        //TODO: might be good to add here some log
    case RSSL_DT_ASCII_STRING:
        to = AS_MAMA_FIELD_TYPE_STRING;    
        break;
    }
    return true;
}


