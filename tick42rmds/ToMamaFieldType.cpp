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
#include "ToMamaFieldType.h"

bool ToMamaFieldType(const std::string &from, mamaFieldType &to)
{
    /** Sub message*/
    if         (from == "MAMA_FIELD_TYPE_MSG") to = MAMA_FIELD_TYPE_MSG;

    /** Opaque binary */
    else if (from == "MAMA_FIELD_TYPE_OPAQUE") to = MAMA_FIELD_TYPE_OPAQUE;

    /** String */
    else if (from == "MAMA_FIELD_TYPE_STRING") to = MAMA_FIELD_TYPE_STRING;

    /** Boolean */
    else if (from == "MAMA_FIELD_TYPE_BOOL") to = MAMA_FIELD_TYPE_BOOL;

    /** Character */
    else if (from == "MAMA_FIELD_TYPE_CHAR") to = MAMA_FIELD_TYPE_CHAR;

    /** Signed 8 bit integer */
    else if (from == "MAMA_FIELD_TYPE_I8") to = MAMA_FIELD_TYPE_I8;

    /** Unsigned byte */
    else if (from == "MAMA_FIELD_TYPE_U8") to = MAMA_FIELD_TYPE_U8;

    /** Signed 16 bit integer */
    else if (from == "MAMA_FIELD_TYPE_I16") to = MAMA_FIELD_TYPE_I16;

    /** Unsigned 16 bit integer */
    else if (from == "MAMA_FIELD_TYPE_U16") to = MAMA_FIELD_TYPE_U16;

    /** Signed 32 bit integer */
    else if (from == "MAMA_FIELD_TYPE_I32") to = MAMA_FIELD_TYPE_I32;

    /** Unsigned 32 bit integer */
    else if (from == "MAMA_FIELD_TYPE_U32") to = MAMA_FIELD_TYPE_U32;

    /** Signed 64 bit integer */
    else if (from == "MAMA_FIELD_TYPE_I64") to = MAMA_FIELD_TYPE_I64;

    /** Unsigned 64 bit integer */
    else if (from == "MAMA_FIELD_TYPE_U64") to = MAMA_FIELD_TYPE_U64;

    /** 32 bit float */
    else if (from == "MAMA_FIELD_TYPE_F32") to = MAMA_FIELD_TYPE_F32;

    /** 64 bit float */
    else if (from == "MAMA_FIELD_TYPE_F64") to = MAMA_FIELD_TYPE_F64;

    /** 64 bit MAMA time */
    else if (from == "MAMA_FIELD_TYPE_TIME") to = MAMA_FIELD_TYPE_TIME;

    /** MAMA price */
    else if (from == "MAMA_FIELD_TYPE_PRICE") to = MAMA_FIELD_TYPE_PRICE;

    /** Array type support */
    else if (from == "MAMA_FIELD_TYPE_VECTOR_I8") to = MAMA_FIELD_TYPE_VECTOR_I8;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_U8") to = MAMA_FIELD_TYPE_VECTOR_U8;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_I16") to = MAMA_FIELD_TYPE_VECTOR_I16;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_U16") to = MAMA_FIELD_TYPE_VECTOR_U16;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_I32") to = MAMA_FIELD_TYPE_VECTOR_I32;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_U32") to = MAMA_FIELD_TYPE_VECTOR_U32;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_I64") to = MAMA_FIELD_TYPE_VECTOR_I64;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_U64") to = MAMA_FIELD_TYPE_VECTOR_U64;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_F32") to = MAMA_FIELD_TYPE_VECTOR_F32;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_F64") to = MAMA_FIELD_TYPE_VECTOR_F64;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_STRING") to = MAMA_FIELD_TYPE_VECTOR_STRING;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_MSG") to = MAMA_FIELD_TYPE_VECTOR_MSG;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_TIME") to = MAMA_FIELD_TYPE_VECTOR_TIME;
    else if (from == "MAMA_FIELD_TYPE_VECTOR_PRICE") to = MAMA_FIELD_TYPE_VECTOR_PRICE;
    else if (from == "MAMA_FIELD_TYPE_QUANTITY") to = MAMA_FIELD_TYPE_QUANTITY;

    /** Collection */
    else if (from == "MAMA_FIELD_TYPE_COLLECTION") to = MAMA_FIELD_TYPE_COLLECTION;

    /** Unknown */
    else if (from == "MAMA_FIELD_TYPE_UNKNOWN") to = MAMA_FIELD_TYPE_UNKNOWN;
    else {
        return false;
    }
    return true;
}

bool ToMamaFieldType(UpaAsMamaFieldType from, mamaFieldType &to)
{
    bool result = false;
    if (from >= AS_MAMA_FIELD_TYPE_MSG && from <= AS_MAMA_FIELD_TYPE_UNKNOWN)
    {
        to = (mamaFieldType)from;
        result = true;
    }
    else
    {
        switch (from)
        {
        case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:
            to = MAMA_FIELD_TYPE_F32;
            break;
        case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
            to = MAMA_FIELD_TYPE_STRING;
            break;
        case RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME:
            to = MAMA_FIELD_TYPE_TIME;
            break;
        case RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME:
             to = MAMA_FIELD_TYPE_TIME;
            break;
        }

        switch (from)
        {
        case RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32:
        case RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING:
        case RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME:
        case RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME:
            result = true;
            break;
        }

    }
    return result;
}

