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
#ifndef __UPAASMAMAFIELDTYPE_H__
#define __UPAASMAMAFIELDTYPE_H__

#include <string>
#include <rtr/rsslTypes.h>
#include <mama/types.h>

#include "UPAAsMamaFieldTypeEnum.h"

bool UpaToMamaFieldType(RsslUInt8 rwfType, RsslUInt8 mfType, UpaAsMamaFieldType& to);

/*
 *  MamaField_t encapsulate one OpenMAMA field information (name, FID & type)
 *    It reflects the columns "mama_field_name","mama_fid","mama_field_type" in the fieldmap.csv
 */

struct MamaField_t
{
    mama_fid_t          mama_fid;
    UpaAsMamaFieldType  mama_field_type;
    std::string         mama_field_name;
    bool                has_no_mama_fid;
};

#endif //__UPAASMAMAFIELDTYPE_H__
