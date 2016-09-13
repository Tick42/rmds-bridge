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
#ifndef __UPAMAMACOMMONFIELDS_H__
#define __UPAMAMACOMMONFIELDS_H__

#include "UPAAsMamaFieldType.h"

// wrapper around mama common fields and book fields
//
// provides a single place to define these 

struct BookFields
{
    MamaField_t OrderTone;        // 3886|ORDER_TONE
    MamaField_t wNumLevels;        // 651|wNumLevels
    MamaField_t wPlAction;        // 652|wPlAction
    MamaField_t wPlPrice;        // 653|wPlPrice
    MamaField_t wPlSide;        // 654|wPlSide
    MamaField_t wPlSize;        // 655|wPlSize
    MamaField_t wPlSizeChange;    // 656|wPlSizeChange
    MamaField_t wPlNumEntries;    // 657|wPlNumEntries
    MamaField_t wPlTime;        // 658|wPlTime
    MamaField_t wPlNumAttach;    // 659|wPlNumAttach
    MamaField_t wBookTime;        // 671|wBookTime
    MamaField_t wEntryId;        // 681|wEntryId
    MamaField_t wEntrySize;        // 682|wEntrySize
    MamaField_t wEntryAction;    // 683|wEntryAction
    MamaField_t wEntryReason;    // 684|wEntryReason
    MamaField_t wEntryTime;        // 685|wEntryTime
    MamaField_t wEntryStatus;    // 686|wEntryStatus
    MamaField_t wPriceLevels;    // 699|wPriceLevels
    MamaField_t wPlEntries;        // 700|wPlEntries

    MamaField_t wBookType;        // 4714|wBookType
    MamaField_t TIMACT_MS;


};

struct CommonFields
{
    MamaField_t wSymbol;        // 470
    MamaField_t wIssueSymbol;    // 305
    MamaField_t wIndexSymbol;    // 293
    MamaField_t wPartId;        // 429
    MamaField_t wSeqNum;        // 498
    MamaField_t wSrcTime;        // 465
    MamaField_t wLineTime;        // 1174
    MamaField_t wActivityTime;    // 102
    MamaField_t wPubId;            //495
};

class UpaMamaCommonFields
{
public:
    static const struct BookFields& BookFields()
    {
        if (!initialized_)
        {
            initialize();
        }
        return bookFields_;
    }

    static const struct CommonFields& CommonFields()
    {
        if (!initialized_)
        {
            initialize();
        }
        return commonFields_;
    }

private:
    static void initialize();

private:
    static struct BookFields bookFields_;
    static struct CommonFields commonFields_;
    static bool initialized_;
};

#endif //__UPAMAMACOMMONFIELDS_H__
