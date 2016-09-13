
/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
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

#include "UPAMamaFieldMap.h"
#include "UPABookMessage.h"
#include "UPASubscription.h"
#include "RMDSSubscriber.h"

// decodes a field from a rssl message and inserts it into a mama message

class UPAFieldDecoder
{
public:
    UPAFieldDecoder(UPAConsumer_ptr_t consumer, UpaMamaFieldMap_ptr_t fieldmap, const std::string & SourceName, const std::string & Symbol)
        :consumer_(consumer), fieldmap_(fieldmap), sourceName_(SourceName), symbol_(Symbol)
    {
        dictionary_ = consumer_->RsslDictionary()->RsslDictionary();

        string transportName = consumer->GetOwner()->GetTransportName();
        boost::shared_ptr<TransportConfig_t> enhancedConfig_ = boost::make_shared<TransportConfig_t>(transportName);
        string dateAsString = enhancedConfig_->getString("returnDateAndTimeAsString");
        string ansiAsOpaque = enhancedConfig_->getString("returnAnsiAsOpaque");
    
        returnDateTimeAsString_ = (dateAsString == "true");
        returnAnsiAsOpaque_ = (ansiAsOpaque == "true");
    }

    RsslRet DecodeFieldEntry(RsslFieldEntry* fEntry, RsslDecodeIterator *dIter, mamaMsg msg);

    RsslRet DecodeBookFieldEntry(RsslFieldEntry* fEntry, RsslDecodeIterator *dIter, UPABookEntry_ptr_t entry);

protected:

    // set of functions to add rssl fields to mama messages using the mama type from the field map
    //
    // The functions provide a basic "take the decoded field and insert it into the mama message with type specified in the field map"
    // They can be over-ridden if more complex logic is required
    virtual mama_status AddRsslUintToMsg(mamaMsg msg, MamaField_t & mamaField, RsslUInt64 UIntVal, RsslFieldId fid);
    virtual mama_status AddRsslIntToMsg(mamaMsg msg, MamaField_t & mamaField, RsslInt64 IntVal,RsslFieldId fid);
    virtual mama_status AddRsslFloatToMsg(mamaMsg msg, MamaField_t & mamaField, RsslFloat floatVal, RsslFieldId fid);
    virtual mama_status AddRsslDoubleToMsg(mamaMsg msg, MamaField_t & mamaField, RsslDouble dblVal, RsslUInt8 hint, RsslFieldId fid);
    virtual mama_status AddRsslDateToMsg(mamaMsg msg, MamaField_t & mamaField, RsslDateTime dateVal, RsslFieldId fid, bool isBlank = false);
    virtual mama_status AddRsslTimeToMsg(mamaMsg msg, MamaField_t & mamaField, RsslDateTime timeVal, RsslFieldId fid, bool isBlank = false);
    virtual mama_status AddRsslDateTimeToMsg(mamaMsg msg, MamaField_t & mamaField, RsslDateTime dateTimeVal, RsslFieldId fid, bool isBlank = false);
    virtual mama_status AddRsslStringToMsg(mamaMsg msg, MamaField_t & mamaField, std::string strVal, RsslFieldId fid);




private:

    // for lookup
    RsslDataDictionary* dictionary_;
    UpaMamaFieldMap_ptr_t fieldmap_;

    // for diagnostics
    const std::string & sourceName_;
    const std::string & symbol_;

    mamaPricePrecision RsslHintToMamaPrecisionTo(RsslRealHints p, uint16_t fid);

    UPAConsumer_ptr_t consumer_;
    bool returnDateTimeAsString_;            // return marketfeed dates and times as string rather than MamaDateTime
    bool returnAnsiAsOpaque_;                // return ANSI pages as Mama Opaque
};

