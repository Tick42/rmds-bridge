
/*
* RMDSBridge: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-15 Tick42 Ltd.
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
#if !defined(__FIELDENCODER_H__)
#define __FIELDENCODER_H__

#include "UPAMamaFieldMap.h"
#include <utils/namespacedefines.h>

// Encodes an entire mama message into an rssl field list

class UPAFieldEncoder
{
public:
    UPAFieldEncoder(mamaDictionary dictionary, UpaMamaFieldMap_ptr_t upaFieldMap,
        RsslDataDictionary *rmdsDictionary, const std::string &sourceName, const std::string &symbol);
    bool encode(mamaMsg msg, RsslChannel *chnl, RsslMsg *rsslMsg, RsslBuffer *buffer);

private:
    static void MAMACALLTYPE mamaMsgIteratorCb(const mamaMsg msg, const mamaMsgField  field, void* closure);
    void encodeField(const mamaMsgField &field);
    RsslRealHints MamaPrecisionToRsslHint(mamaPricePrecision p, uint16_t fid);

private:
    mamaDictionary dictionary_;
    const std::string &sourceName_;
    const std::string &symbol_;
    RsslBuffer *rsslMessageBuffer_;
    RsslEncodeIterator itEncode_;
    RsslFieldList fList_;
    UpaMamaFieldMap_ptr_t upaFieldMap_;
    RsslDataDictionary * rmdsDictionary_;
    bool encodeFail_;

    static utils::collection::unordered_set<int> suppressBadEnumWarnings_;
};

#endif // __FIELDENCODER_H__
