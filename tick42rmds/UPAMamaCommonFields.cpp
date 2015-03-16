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
#include "UPAMamaCommonFields.h"

struct BookFields UpaMamaCommonFields::bookFields_;
struct CommonFields UpaMamaCommonFields::commonFields_;
bool UpaMamaCommonFields::initialized_ = false;

void UpaMamaCommonFields::initialize()
{
    // initialize the common fields
    bookFields_.wBookType.mama_fid = 169;
    bookFields_.wBookType.mama_field_name = "wBookType";
    bookFields_.wBookType.mama_field_type = AS_MAMA_FIELD_TYPE_U8;

    bookFields_.wNumLevels.mama_fid = 651;
    bookFields_.wNumLevels.mama_field_name = "wNumLevels";
    bookFields_.wNumLevels.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wPlAction.mama_fid = 652;
    bookFields_.wPlAction.mama_field_name = "wPlAction";
    bookFields_.wPlAction.mama_field_type = AS_MAMA_FIELD_TYPE_CHAR;

    bookFields_.wPlPrice.mama_fid = 653;
    bookFields_.wPlPrice.mama_field_name = "wPlPrice";
    bookFields_.wPlPrice.mama_field_type = AS_MAMA_FIELD_TYPE_PRICE;

    bookFields_.wPlSide.mama_fid = 654;
    bookFields_.wPlSide.mama_field_name = "wPlSide";
    bookFields_.wPlSide.mama_field_type = AS_MAMA_FIELD_TYPE_CHAR;

    bookFields_.wPlSize.mama_fid = 655;
    bookFields_.wPlSize.mama_field_name = "wPlSize";
    bookFields_.wPlSize.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wPlSizeChange.mama_fid = 656;
    bookFields_.wPlSizeChange.mama_field_name = "wPlSizeChange";
    bookFields_.wPlSizeChange.mama_field_type = AS_MAMA_FIELD_TYPE_I32;

    bookFields_.wPlNumEntries.mama_fid = 657;
    bookFields_.wPlNumEntries.mama_field_name = "wPlNumEntries";
    bookFields_.wPlNumEntries.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wPlTime.mama_fid = 658;
    bookFields_.wPlTime.mama_field_name = "wPlTime";
    bookFields_.wPlTime.mama_field_type = AS_MAMA_FIELD_TYPE_TIME;

    bookFields_.wPlNumAttach.mama_fid = 659;
    bookFields_.wPlNumAttach.mama_field_name = "wPlNumAttach";
    bookFields_.wPlNumAttach.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wBookTime.mama_fid = 671;
    bookFields_.wBookTime.mama_field_name = "wBookTime";
    bookFields_.wBookTime.mama_field_type = AS_MAMA_FIELD_TYPE_TIME;

    bookFields_.wEntryId.mama_fid = 681;
    bookFields_.wEntryId.mama_field_name = "wEntryId";
    bookFields_.wEntryId.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wEntrySize.mama_fid = 682;
    bookFields_.wEntrySize.mama_field_name = "wEntrySize";
    bookFields_.wEntrySize.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wEntryAction.mama_fid = 683;
    bookFields_.wEntryAction.mama_field_name = "wEntryAction";
    bookFields_.wEntryAction.mama_field_type = AS_MAMA_FIELD_TYPE_CHAR;

    bookFields_.wEntryReason.mama_fid = 684;
    bookFields_.wEntryReason.mama_field_name = "wEntryReason";
    bookFields_.wEntryReason.mama_field_type = AS_MAMA_FIELD_TYPE_I32;

    bookFields_.wEntryTime.mama_fid = 685;
    bookFields_.wEntryTime.mama_field_name = "wEntryTime";
    bookFields_.wEntryTime.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wEntryStatus.mama_fid = 686;
    bookFields_.wEntryStatus.mama_field_name = "wEntryStatus";
    bookFields_.wEntryStatus.mama_field_type = AS_MAMA_FIELD_TYPE_U32;

    bookFields_.wPriceLevels.mama_fid = 699;
    bookFields_.wPriceLevels.mama_field_name = "wPriceLevels";
    bookFields_.wPriceLevels.mama_field_type = AS_MAMA_FIELD_TYPE_VECTOR_MSG;

    bookFields_.wPlEntries.mama_fid = 700;
    bookFields_.wPlEntries.mama_field_name = "wPlEntries";
    bookFields_.wPlEntries.mama_field_type = AS_MAMA_FIELD_TYPE_VECTOR_MSG;

    bookFields_.OrderTone.mama_fid = 3886;
    bookFields_.OrderTone.mama_field_name = "ORDER_TONE";
    bookFields_.OrderTone.mama_field_type = AS_MAMA_FIELD_TYPE_STRING;
    
    // initialize the common fields

    commonFields_.wSymbol.mama_field_name = "wSymbol";
    commonFields_.wSymbol.mama_fid = 470;
    commonFields_.wSymbol.mama_field_type = AS_MAMA_FIELD_TYPE_STRING;

    commonFields_.wIssueSymbol.mama_field_name = "wIssueSymbol";
    commonFields_.wIssueSymbol.mama_fid = 305;
    commonFields_.wIssueSymbol.mama_field_type = AS_MAMA_FIELD_TYPE_STRING;


    commonFields_.wIndexSymbol.mama_field_name = "wIndexSymbol";
    commonFields_.wIndexSymbol.mama_fid = 293;
    commonFields_.wIndexSymbol.mama_field_type = AS_MAMA_FIELD_TYPE_STRING;


    commonFields_.wPartId.mama_field_name = "wPartId";
    commonFields_.wPartId.mama_fid = 429;
    commonFields_.wPartId.mama_field_type = AS_MAMA_FIELD_TYPE_STRING;


    commonFields_.wSeqNum.mama_field_name = "wSeqNum";
    commonFields_.wSeqNum.mama_fid = 498;
    commonFields_.wSeqNum.mama_field_type = AS_MAMA_FIELD_TYPE_I32;


    commonFields_.wSrcTime.mama_field_name = "wSrcTime";
    commonFields_.wSrcTime.mama_fid = 465;
    commonFields_.wSrcTime.mama_field_type = AS_MAMA_FIELD_TYPE_TIME;


    commonFields_.wLineTime.mama_field_name = "wLineTime";
    commonFields_.wLineTime.mama_fid = 1174;
    commonFields_.wLineTime.mama_field_type = AS_MAMA_FIELD_TYPE_TIME;


    commonFields_.wActivityTime.mama_field_name = "wActivityTime";
    commonFields_.wActivityTime.mama_fid = 102;
    commonFields_.wActivityTime.mama_field_type = AS_MAMA_FIELD_TYPE_TIME;
    initialized_ = true;
}
