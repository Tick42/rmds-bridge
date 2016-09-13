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
#pragma once
#ifndef __UPAASMAMAFIELDTYPEENUM_H__
#define __UPAASMAMAFIELDTYPEENUM_H__

/*
 * UpaToMamaFieldType is based on mamaFieldType_ and starts its
 * values from: MAMA_FIELD_TYPE_UNKNOWN + 1
 */
enum UpaAsMamaFieldType
{
    /** Sub message*/
    AS_MAMA_FIELD_TYPE_MSG           =    MAMA_FIELD_TYPE_MSG, 

    /** Opaque binary */
    AS_MAMA_FIELD_TYPE_OPAQUE        =    MAMA_FIELD_TYPE_OPAQUE,

    /** String */
    AS_MAMA_FIELD_TYPE_STRING        =    MAMA_FIELD_TYPE_STRING,

    /** Boolean */
    AS_MAMA_FIELD_TYPE_BOOL          =    MAMA_FIELD_TYPE_BOOL, 

    /** Character */
    AS_MAMA_FIELD_TYPE_CHAR          =   MAMA_FIELD_TYPE_CHAR, 

    /** Signed 8 bit integer */
    AS_MAMA_FIELD_TYPE_I8            =   MAMA_FIELD_TYPE_I8, 

    /** Unsigned byte */
    AS_MAMA_FIELD_TYPE_U8            =   MAMA_FIELD_TYPE_U8,

    /** Signed 16 bit integer */
    AS_MAMA_FIELD_TYPE_I16           =   MAMA_FIELD_TYPE_I16,

    /** Unsigned 16 bit integer */
    AS_MAMA_FIELD_TYPE_U16           =   MAMA_FIELD_TYPE_U16,

    /** Signed 32 bit integer */
    AS_MAMA_FIELD_TYPE_I32           =   MAMA_FIELD_TYPE_I32,

    /** Unsigned 32 bit integer */
    AS_MAMA_FIELD_TYPE_U32           =   MAMA_FIELD_TYPE_U32,

    /** Signed 64 bit integer */
    AS_MAMA_FIELD_TYPE_I64           =   MAMA_FIELD_TYPE_I64,

    /** Unsigned 64 bit integer */
    AS_MAMA_FIELD_TYPE_U64           =   MAMA_FIELD_TYPE_U64,

    /** 32 bit float */
    AS_MAMA_FIELD_TYPE_F32           =   MAMA_FIELD_TYPE_F32, 

    /** 64 bit float */
    AS_MAMA_FIELD_TYPE_F64           =   MAMA_FIELD_TYPE_F64,

    /** 64 bit MAMA time */
    AS_MAMA_FIELD_TYPE_TIME          =   MAMA_FIELD_TYPE_TIME,

    /** MAMA price */
    AS_MAMA_FIELD_TYPE_PRICE         =   MAMA_FIELD_TYPE_PRICE,

    /** Array type support */
    AS_MAMA_FIELD_TYPE_VECTOR_I8     =   MAMA_FIELD_TYPE_VECTOR_I8, 
    AS_MAMA_FIELD_TYPE_VECTOR_U8     =   MAMA_FIELD_TYPE_VECTOR_U8, 
    AS_MAMA_FIELD_TYPE_VECTOR_I16    =   MAMA_FIELD_TYPE_VECTOR_I16, 
    AS_MAMA_FIELD_TYPE_VECTOR_U16    =   MAMA_FIELD_TYPE_VECTOR_U16, 
    AS_MAMA_FIELD_TYPE_VECTOR_I32    =   MAMA_FIELD_TYPE_VECTOR_I32, 
    AS_MAMA_FIELD_TYPE_VECTOR_U32    =   MAMA_FIELD_TYPE_VECTOR_U32,
    AS_MAMA_FIELD_TYPE_VECTOR_I64    =   MAMA_FIELD_TYPE_VECTOR_I64, 
    AS_MAMA_FIELD_TYPE_VECTOR_U64    =   MAMA_FIELD_TYPE_VECTOR_U64,
    AS_MAMA_FIELD_TYPE_VECTOR_F32    =   MAMA_FIELD_TYPE_VECTOR_F32, 
    AS_MAMA_FIELD_TYPE_VECTOR_F64    =   MAMA_FIELD_TYPE_VECTOR_F64,
    AS_MAMA_FIELD_TYPE_VECTOR_STRING =   MAMA_FIELD_TYPE_VECTOR_STRING,
    AS_MAMA_FIELD_TYPE_VECTOR_MSG    =   MAMA_FIELD_TYPE_VECTOR_MSG,
    AS_MAMA_FIELD_TYPE_VECTOR_TIME   =   MAMA_FIELD_TYPE_VECTOR_TIME,
    AS_MAMA_FIELD_TYPE_VECTOR_PRICE  =   MAMA_FIELD_TYPE_VECTOR_PRICE,
    AS_MAMA_FIELD_TYPE_QUANTITY      =   MAMA_FIELD_TYPE_QUANTITY,

    /** Collection */
    AS_MAMA_FIELD_TYPE_COLLECTION    =   MAMA_FIELD_TYPE_COLLECTION,

    AS_MAMA_FIELD_TYPE_UNKNOWN       =  MAMA_FIELD_TYPE_UNKNOWN,
    RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32 = MAMA_FIELD_TYPE_UNKNOWN + 1,
    RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING,
    RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME,
    RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME,
};

#endif //__UPAASMAMAFIELDTYPEENUM_H__

