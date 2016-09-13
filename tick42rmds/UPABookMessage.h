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

#ifndef __UPABOOKMESSAGE_H__
#define __UPABOOKMESSAGE_H__

#include "utils/namespacedefines.h"

//////////////////////////////////////////////////////////////////////////
//
// aggregate contents of rssl mbo or mbp response message to transform into mama message
//
///////

#include "UPAMamaFieldMap.h"

// this is the basic book entry
class UPABookEntry
{
public:
    UPABookEntry();
    ~UPABookEntry();

    // accessors
    char ActionCode() const;
    void ActionCode(char val);
    char SideCode() const;
    void SideCode(char val);
    std::string Orderid() const;
    void Orderid(std::string val);

    std::string OrderTone() const;
    void OrderTone(std::string val);
    bool HaveOrderTone();

    std::string Mmid() const;
    void Mmid(std::string val);
    bool HaveMmid();

    RsslInt Size() const;
    void Size(RsslInt val);
    RsslUInt64 Time() const;
    void Time(RsslUInt64 val);
    RsslState Status() const;
    void Status(RsslState val);
    RsslReal Price() const;
    void Price(RsslReal val);
    void Price( std::string val );
    RsslUInt64 NumOrders() const;
    void NumOrders(RsslUInt64 val);

    bool HasSide();

private:
    char actionCode_;    // 683|wEntryAction
    char sideCode_;        // 654|wPlSide|
    std::string orderid_;    // 681|wEntryId
    std::string orderTone_;  // ???
    std::string mmid_;        // xxx|wPartId
    RsslInt size_;        // 682|wEntrySize
    RsslReal price_;    // 653|wPlPrice
    RsslUInt64 time_;    // 685|wEntryTime
    RsslState status_;    // 686|wEntryStatus
    RsslUInt64 numOrders_; //// 657|wPlNumEntries
    bool haveOrderTone_;    // since ORDER_TONE is TSE, we only add it if it is in msg
    bool haveMmid_;            // since MMID is not always there we only add it ...
};


typedef boost::shared_ptr<UPABookEntry> UPABookEntry_ptr_t;

typedef std::list<UPABookEntry_ptr_t> EntryList_t;


// Some data sources use an encoding for the price point and dont include the fields in all messages.
// We need to be able to maintain a map where the price points are keyed on the encoding.
// items are inserted on an ADD message at the price point and removed on a DELETE message at the price point
class PricePoint
{
public:
    PricePoint(RsslReal price, char side)
        :price_(price), sideCode_(side)
    {
    }

    void assign(const UPABookEntry& src)
    {
        Price(src.Price());
        SideCode(src.SideCode());
    }

    RsslReal Price() const { return price_; }
    void Price(RsslReal val) { price_ = val; }
    char SideCode() const { return sideCode_; }
    void SideCode(char val) { sideCode_ = val; }

private:
    RsslReal price_;    // 653|wPlPrice
    char sideCode_;        // 654|wPlSide|
};

typedef boost::shared_ptr<PricePoint> PricePoint_ptr_t;

typedef utils::collection::unordered_map<std::string, PricePoint_ptr_t> PricePointMap_t;
typedef utils::collection::unordered_set<std::string> OrderIDSet_t;

class UPALevel
{
public:
    UPALevel(RsslReal price, RsslUInt64 time, char sideCode, RsslInt size, UpaMamaFieldMap_ptr_t fieldmap);

    ~UPALevel();

    void AddEntry(UPABookEntry_ptr_t entry);

    void ClearDeltas();

    char ActionCode() const
    {
        return actionCode_;
    }

    void ActionCode(char val)
    {
        actionCode_ = val;
    }

    char SideCode() const
    {
        return sideCode_;
    }

    void SideCode(char val)
    {
        sideCode_ = val;
    }

    bool Dirty() const { return dirty_; }
    void Dirty(bool val) { dirty_ = val; }

    int NumOrders() const
    {
        return (int)orders_.size();
    }

   RsslUInt64 Time() const { return time_; }

    RsslReal Price() const { return price_; }
    void Price(RsslReal val) { price_ = val; }

    int AddEntriesToMsg(mamaMsg & msg);

    void ClearMessageVector(mamaMsg msg);

private:
    RsslUInt64 time_;
    RsslReal price_;
    RsslInt size_;

    // this is the set of deltas on the level
    EntryList_t entryListDelta_;

    char actionCode_;    // 652|wPlAction
    char sideCode_;        // 654|wPlSide

    bool dirty_;

    // need to maintains a set of current order ids in this level so that we can tell when the level is emprt and can be removed
    OrderIDSet_t orders_;

    std::vector<mamaMsg> entryMsgs_;

    UpaMamaFieldMap_ptr_t fieldmap_;
};

typedef boost::shared_ptr<UPALevel> UPALevel_ptr_t;

typedef utils::collection::unordered_map<RsslInt, UPALevel_ptr_t> LevelMap_t;
// accumulate the entries keyed on price in here and render a mama message from it


// todo rename to UPABookByOrderMessage
class UPABookByOrderMessage
{
public:
    UPABookByOrderMessage(void);
    ~UPABookByOrderMessage(void);

    void Fieldmap(UpaMamaFieldMap_ptr_t val) { fieldmap_ = val; }
    bool StartUpdate();
    bool EndUpdate(mamaMsg msg);

    // add an entry from the rssl message
    bool AddEntry(UPABookEntry_ptr_t entry);

    // update an entry fromn the rssl message
    bool UpdateEntry(UPABookEntry_ptr_t entry);

    bool RemoveEntry(UPABookEntry_ptr_t entry );

    // build the mamda message
    bool BuildMamdaMessage(mamaMsg msg);

private:
    typedef utils::collection::unordered_map<std::string, UPABookEntry_ptr_t> OrderMap_t;
    OrderMap_t orderMap_;

    std::vector<mamaMsg> levelMsgs_;

    uint16_t numLevelMsgs_;

    LevelMap_t levelMapBid_;
    LevelMap_t levelMapAsk_;

    UpaMamaFieldMap_ptr_t fieldmap_;

};

// this is a lot simpler as there is just a simple list of entries for each price
class UPABookByPriceMessage
{
public:
    UPABookByPriceMessage();
    ~UPABookByPriceMessage();

    void Fieldmap(UpaMamaFieldMap_ptr_t val) { fieldmap_ = val; }

    bool StartUpdate();

    bool AddEntry(UPABookEntry_ptr_t entry);

    bool BuildMamdaMessage(mamaMsg msg);

    const EntryList_t& getEntryList() const { return entries_; }

private:

    std::vector<mamaMsg> levelMsgs_;
    EntryList_t entries_;

    UpaMamaFieldMap_ptr_t fieldmap_;

};


#endif //__UPABOOKMESSAGE_H__
