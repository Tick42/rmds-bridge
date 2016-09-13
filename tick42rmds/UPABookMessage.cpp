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

#include "UPABookMessage.h"
#include "UPAMamaCommonFields.h"
#include <utils/t42log.h>

using namespace std;

static mamaDateTime makeDateTime(mama_u64_t msec)
{
    mama_u64_t e = msec * 1000;  // convert to micros
    mamaDateTime dt;
    mamaDateTime_create(&dt);

    mamaDateTime_setToMidnightToday (dt, NULL);
    mama_u64_t micros = 0;
    mamaDateTime_getEpochTimeMicroseconds (dt, &micros);
    micros += e;
    mamaDateTime_setEpochTimeMicroseconds (dt, micros);

    return dt;
}

static mamaPricePrecision RsslHintToMamaPrecisionTo( RsslRealHints p, uint16_t fid)
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
      return MAMA_PRICE_PREC_10000000000;

   default:
      t42log_info("Unhandled RsslHint %d converting fid %d\n", p, fid );
      return MAMA_PRICE_PREC_UNKNOWN;
   }
}

UPABookByOrderMessage::UPABookByOrderMessage(void)
{

}


UPABookByOrderMessage::~UPABookByOrderMessage(void)
{
}

bool UPABookByOrderMessage::AddEntry( UPABookEntry_ptr_t entry )
{

    // look up the level on the key (price)
    // add a new one if needed
    // and insert the entry

    t42log_debug("adding entry price %llu, order id %s\n", entry->Price().value, entry->Orderid().c_str());
    RsslInt    key = entry->Price().value;

    UPALevel_ptr_t level;


    // look up the entry in the ordermap

    OrderMap_t::iterator it = orderMap_.find(entry->Orderid());

    if (it == orderMap_.end() )
    {
        // add to the map
        orderMap_[entry->Orderid()] = entry;
    }

    if (entry->SideCode()== 'A')
    {
        // its ASK
        LevelMap_t::iterator it = levelMapAsk_.find(key);

        if (it == levelMapAsk_.end())
        {
            // don't have a level for this key
            level = UPALevel_ptr_t(new UPALevel(entry->Price(), entry->Time(), 'A', entry->Size(), fieldmap_));
            //level->setFids(fids_);
            levelMapAsk_[key] = level;
        }
        else
        {
            level = it->second;
        }
    }
    else if (entry->SideCode()== 'B')
    {
        // its BID
        LevelMap_t::iterator it = levelMapBid_.find(key);

        if (it == levelMapBid_.end())
        {
            // don't have a level for this key
            level = UPALevel_ptr_t(new UPALevel(entry->Price(), entry->Time(), 'B', entry->Size(),fieldmap_));
            levelMapBid_[key] = level;
        }
        else
        {
            level = it->second;
        }
    }
    else
    {
        // todo access the symbol name and other data and insert into wwarning message
        t42log_warn("Unexpected level side code, ignoring update");
        return false;
    }


    if (level.get() != 0)
    {
        level->AddEntry(entry);
    }
    else
    {
        t42log_warn("null level");
    }

    return true;

}

bool UPABookByOrderMessage::UpdateEntry( UPABookEntry_ptr_t entry )
{
    t42log_debug("updating entry price %llu, order id %s\n", entry->Price().value, entry->Orderid().c_str());


    // look up the order id
    OrderMap_t::iterator it = orderMap_.find(entry->Orderid());
    if (it == orderMap_.end() )
    {
        // should be an update to an existing order
        // so log a warning
        // todo add symbol name to log message
        t42log_warn("received update order for non existent order id %s - ignored", entry->Orderid().c_str());
        return false;
    }

    UPABookEntry_ptr_t existingEntry = it->second;

    // now we need to check if either the price or the size has changed

    // if its the price we need to remove from the existing level and add to a new one. If its the size then we just need to update in the existing level.
    // If we process the price change first then  that will handle an update that is both price and size


    if (existingEntry->Price().value != entry->Price().value)
    {

        RsslInt    key = existingEntry->Price().value;
        UPALevel_ptr_t oldLevel;

        if (existingEntry->SideCode()== 'A')
        {
            // its ASK
            LevelMap_t::iterator it = levelMapAsk_.find(key);

            if (it == levelMapAsk_.end())
            {
                // dont have a level for this key
                t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
                return false;
            }

            // need to set the side code because an rssl entry update may not contain it
            entry->SideCode('A');

            oldLevel = it->second;
        }
        else if (existingEntry->SideCode()== 'B')
        {
            // its BID
            LevelMap_t::iterator it = levelMapBid_.find(key);

            if (it == levelMapBid_.end())
            {
                // dont have a level for this key
                t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
                return false;
            }

            // need to set the side code because an rssl entry update may not contain it
            entry->SideCode('B');
            oldLevel = it->second;
        }
        else
        {
            // todo access the symbol name and other data and insert into wwarning message
            t42log_warn("Unexpected level side code, ignoring update");
            return false;
        }

        // Now we have the level (and side) of the existing entry for this order id
        // so add a delete entry for the order
        existingEntry->ActionCode('D');
        oldLevel->AddEntry(existingEntry);

        // now we need to add the updated entry at a new level

        entry->ActionCode('A');
        AddEntry(entry);


        // then replace the entry in the orderid map.

        orderMap_[entry->Orderid()] = entry;
    }
    else
    {
        // no change to the level

        RsslInt    key = existingEntry->Price().value;

        if (existingEntry->SideCode()== 'A')
        {
            // its ASK
            LevelMap_t::iterator it = levelMapAsk_.find(key);

            if (it == levelMapAsk_.end())
            {
                // dont have a level for this key
                t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
                return false;
            }

            // need to set the side code because an rssl entry update may not contain it
            entry->SideCode('A');

            // and add the update entry tpo the level
            it->second->AddEntry(entry);
        }
        else if (existingEntry->SideCode()== 'B')
        {
            // its BID
            LevelMap_t::iterator it = levelMapBid_.find(key);

            if (it == levelMapBid_.end())
            {
                // dont have a level for this key
                t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
                return false;
            }

            // need to set the side code because an rssl entry update may not contain it
            entry->SideCode('B');

            // and add the update entry tpo the level
            it->second->AddEntry(entry);
        }
        else
        {
            // todo access the symbol name and other data and insert into wwarning message
            t42log_warn("Unexpected level side code, ignoring update");
            return false;
        }



    }

    return true;

}




bool UPABookByOrderMessage::RemoveEntry(UPABookEntry_ptr_t entry )
{


    // look up the order id
    OrderMap_t::iterator it = orderMap_.find(entry->Orderid());
    if (it == orderMap_.end() )
    {
        // should be an update to an existing order
        // so log a warning
        // todo add symbol name to log message
        t42log_warn("received delete order for non existent order id %s - ignored", entry->Orderid().c_str());
        return false;
    }

    UPABookEntry_ptr_t existingEntry = it->second;

    // have to get the order level key from the existing entry as the new 'delete' entry doesnt have a price in it, just the order id
    RsslInt    key = existingEntry->Price().value;

    // remove from the order map
    orderMap_.erase(it);


    // now find the level for this order
    UPALevel_ptr_t level;

    if (existingEntry->SideCode()== 'A')
    {
        // its ASK
        LevelMap_t::iterator it = levelMapAsk_.find(key);

        if (it == levelMapAsk_.end())
        {
            // dont have a level for this key
            t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
            return false;
        }

        // need to set the side code because an rssl entry update may not contain it
        entry->SideCode('A');

        level = it->second;
    }
    else if (existingEntry->SideCode()== 'B')
    {
        // its BID
        LevelMap_t::iterator it = levelMapBid_.find(key);

        if (it == levelMapBid_.end())
        {
            // dont have a level for this key
            t42log_warn("received update order for order at non-existent level id %s - ignored", entry->Orderid().c_str());
            return false;
        }

        // need to set the side code because an rssl entry update may not contain it
        entry->SideCode('B');
        level = it->second;
    }
    else
    {
        // todo access the symbol name and other data and insert into wwarning message
        t42log_warn("Unexpected level side code, ignoring update");
        return false;
    }

    // set the action to delete
    existingEntry->ActionCode('D');

    level->AddEntry(existingEntry);



    return true;

}

bool UPABookByOrderMessage::BuildMamdaMessage( mamaMsg msg )
{
    // need to accumulate the number of levels in the message as we go as there will be multiple keys for the same level value
    int numLevels = 0;
    const BookFields &bookFields = UpaMamaCommonFields::BookFields();

    // grow the levelMsgs vector if we need to
    // just work on the sum of the bid and ask levels
    numLevelMsgs_ = (RsslUInt16)(levelMapBid_.size() + levelMapAsk_.size());

    if (numLevelMsgs_ > levelMsgs_.size())
    {
        size_t originalSize = levelMsgs_.size();
        levelMsgs_.resize(numLevelMsgs_);
        // create a new mamaMsg for each of the new elements
        for(size_t index = originalSize; index < numLevelMsgs_; index++)
        {
            mamaMsg newMsg;
            mamaMsg_createForPayload(&newMsg, MAMA_PAYLOAD_TICK42RMDS);
            levelMsgs_[index] = newMsg;
        }
    }

    // walk the map and process each book level
    // do the ask side
    LevelMap_t::iterator it = levelMapAsk_.begin();

    while(it != levelMapAsk_.end())
    {
        UPALevel_ptr_t level = it->second;

        // Mamda wants to identify the level with a mamaPrice
        RsslReal levelPrice = level->Price();
        RsslDouble dblVal;
        rsslRealToDouble(&dblVal, &levelPrice);

        mamaPrice p;
        mamaPrice_create(&p);
        mamaPrice_setValue(p, dblVal);
        mamaPricePrecision prec = RsslHintToMamaPrecisionTo((RsslRealHints) levelPrice.hint, 653);
        mamaPrice_setPrecision(p, prec);

        mamaMsg levelMsg = levelMsgs_[numLevels];

        //t42log_debug("created message for level   at 0x%08x", (void*)levelMsg);

        // 654|wPlSide
        mamaMsg_addChar(levelMsg, bookFields.wPlSide.mama_field_name.c_str(), bookFields.wPlSide.mama_fid, 'A');

        // 653|wPlPrice
        mamaMsg_addPrice(levelMsg, bookFields.wPlPrice.mama_field_name.c_str(), bookFields.wPlPrice.mama_fid, p); //p is copied and should be destroyed later on!
        mamaPrice_destroy(p);

        // 658|wPlTime
        mamaDateTime dt =  makeDateTime((mama_u64_t)level->Time());
        mamaMsg_addDateTime(levelMsg, bookFields.wPlTime.mama_field_name.c_str(), bookFields.wPlTime.mama_fid, dt);
        mamaDateTime_destroy(dt);

        if (level->NumOrders() == 0)
        {
            // level is empty so set a delete message and remove the level
            // 652|wPlAction
            mamaMsg_addChar(levelMsg, bookFields.wPlAction.mama_field_name.c_str(), bookFields.wPlAction.mama_fid, 'D');

            // 657|wPlNumEntries
            mamaMsg_addI32(levelMsg, bookFields.wPlNumEntries.mama_field_name.c_str(), bookFields.wPlNumEntries.mama_fid, 0);
            mamaMsg_addI32(levelMsg, bookFields.wPlNumAttach.mama_field_name.c_str(), bookFields.wPlNumAttach.mama_fid, 0);

            level->ClearMessageVector(levelMsg);
            ++it;
        }
        else
        {
            // add the entries for this level
            mamaMsg_addChar(levelMsg, bookFields.wPlAction.mama_field_name.c_str(), bookFields.wPlAction.mama_fid,  level->ActionCode());

            int numEntries = level->AddEntriesToMsg(levelMsg);
            // for some reason mamda reads this as f32
            mamaMsg_addI32(levelMsg, bookFields.wPlNumEntries.mama_field_name.c_str(), bookFields.wPlNumEntries.mama_fid, numEntries);

            ++it;
        }

        levelMsgs_[numLevels] = levelMsg;
        ++numLevels;
    }

    // then the bid side

    it = levelMapBid_.begin();

    while(it != levelMapBid_.end())
    {
        UPALevel_ptr_t level = it->second;

        // Mamda wants to identify the level with a mamaPrice
        RsslReal levelPrice = level->Price();
        RsslDouble dblVal;
        rsslRealToDouble(&dblVal, &levelPrice);

        mamaPrice p;
        mamaPrice_create(&p);
        mamaPrice_setValue(p, dblVal);
        mamaPricePrecision prec = RsslHintToMamaPrecisionTo((RsslRealHints) levelPrice.hint, 653);
        mamaPrice_setPrecision(p, prec);

        mamaMsg levelMsg = levelMsgs_[numLevels];

        //t42log_debug("created message for level   at 0x%08x", (void*)levelMsg);

        // 654|wPlSide
        mamaMsg_addChar(levelMsg, bookFields.wPlSide.mama_field_name.c_str(), bookFields.wPlSide.mama_fid, 'B');
        // 653|wPlPrice
        mamaMsg_addPrice(levelMsg, bookFields.wPlPrice.mama_field_name.c_str(), bookFields.wPlPrice.mama_fid, p);
        mamaPrice_destroy(p);

        mamaDateTime dt =  makeDateTime((mama_u64_t)level->Time());
        mamaMsg_addDateTime(levelMsg, bookFields.wPlTime.mama_field_name.c_str(), bookFields.wPlTime.mama_fid, dt);
        mamaDateTime_destroy(dt);

        if (level->NumOrders() == 0)
        {
            // level is empty so set a delete message and remove the level
            // 652|wPlAction
            mamaMsg_addChar(levelMsg, bookFields.wPlAction.mama_field_name.c_str(), bookFields.wPlAction.mama_fid, 'D');

            // 657|wPlNumEntries
            mamaMsg_addI32(levelMsg, bookFields.wPlNumEntries.mama_field_name.c_str(), bookFields.wPlNumEntries.mama_fid, 0);
            mamaMsg_addI32(levelMsg, bookFields.wPlNumAttach.mama_field_name.c_str(), bookFields.wPlNumAttach.mama_fid, 0);

            ++it;

            level->ClearMessageVector(levelMsg);
        }
        else
        {
            // add the entries for this level
            mamaMsg_addChar(levelMsg, bookFields.wPlAction.mama_field_name.c_str(), bookFields.wPlAction.mama_fid,  level->ActionCode());

            int numEntries = level->AddEntriesToMsg(levelMsg);
            // for some reason mamda reads this as float32
            mamaMsg_addI32(levelMsg, bookFields.wPlNumEntries.mama_field_name.c_str(), bookFields.wPlNumEntries.mama_fid, numEntries);

            ++it;
        }

        // just let the vector grow on demand - we can preallocate if this has a big impact on performance

        levelMsgs_[numLevels] = levelMsg;
        ++numLevels;
    }

    // number of levels - is the number of entries int he level map
    // 651|wNumLevels
    mamaMsg_addI32(msg, bookFields.wNumLevels.mama_field_name.c_str(), bookFields.wNumLevels.mama_fid, numLevels);

    //699|wPriceLevels|
    mamaMsg_addVectorMsg(msg, bookFields.wPriceLevels.mama_field_name.c_str(), bookFields.wPriceLevels.mama_fid, levelMsgs_.data(), numLevels );

    return true;
}

bool UPABookByOrderMessage::StartUpdate()
{
    // clear down all the deltas in each level

    LevelMap_t::iterator it = levelMapBid_.begin();
    while(it != levelMapBid_.end())
    {
        it->second->ClearDeltas();
        ++it;
    }

    it = levelMapAsk_.begin();
    while(it != levelMapAsk_.end())
    {
        it->second->ClearDeltas();
        ++it;
    }

    return true;
}

bool UPABookByOrderMessage::EndUpdate(mamaMsg msg)
{
    // clean out any empty levels

    LevelMap_t::iterator it = levelMapBid_.begin();
    while(it != levelMapBid_.end())
    {

        if (it->second->NumOrders()== 0)
        {
            it->second->ClearMessageVector(msg);
            LevelMap_t::iterator eraseIt = it;
            ++it;
            levelMapBid_.erase(eraseIt);
        }
        else
        {
            ++it;
        }

    }


    it = levelMapAsk_.begin();
    while(it != levelMapAsk_.end())
    {
        LevelMap_t::iterator eraseIt = it;

        if (it->second->NumOrders()== 0)
        {
            it->second->ClearMessageVector(msg);
            LevelMap_t::iterator eraseIt = it;
            ++it;
            levelMapAsk_.erase(eraseIt);
        }
        else
        {
            ++it;
        }

    }


    return true;
}

/////////////////////
//
// UPABookEntry
//
////////////////////

UPABookEntry::UPABookEntry(void)
{
    actionCode_ = 'Z';
    sideCode_ = 'Z';
    size_ = 0;
    numOrders_ = 0;
    price_.value = 0;
    price_.hint = RSSL_RH_EXPONENT0;
    haveOrderTone_ = false;
    haveMmid_ = false;
}


UPABookEntry::~UPABookEntry(void)
{
}

char UPABookEntry::ActionCode() const
{
    return actionCode_;
}

void UPABookEntry::ActionCode( char val )
{
    actionCode_ = val;
}

char UPABookEntry::SideCode() const
{
    return sideCode_;
}

void UPABookEntry::SideCode( char val )
{
    sideCode_ = val;
}

string UPABookEntry::Orderid() const
{
    return orderid_;
}

void UPABookEntry::Orderid( std::string val )
{
    orderid_ = val;
}

// ----------------------------------------------
string UPABookEntry::OrderTone() const
{
    return orderTone_;
}

void UPABookEntry::OrderTone( std::string val )
{
    orderTone_ = val;
    haveOrderTone_ = true;
}

bool UPABookEntry::HaveOrderTone()
{
    return haveOrderTone_;
}

// ----------------------------------------------
string UPABookEntry::Mmid() const
{
    return mmid_;
}

void UPABookEntry::Mmid( std::string val )
{
    mmid_ = val;
    haveMmid_ = true;
}

bool UPABookEntry::HaveMmid()
{
    return haveMmid_;
}

// ----------------------------------------------
RsslInt UPABookEntry::Size() const
{
    return size_;
}

void UPABookEntry::Size( RsslInt val )
{
    size_ = val;
}

RsslState UPABookEntry::Status() const
{
    return status_;
}

void UPABookEntry::Status( RsslState val )
{
    status_ = val;
}

RsslUInt64 UPABookEntry::Time() const
{
    return time_;
}

void UPABookEntry::Time( RsslUInt64 val )
{
    time_ = val;
}

RsslReal UPABookEntry::Price() const
{
    return price_;
}

void UPABookEntry::Price( RsslReal val )
{
    price_ = val;
}


void UPABookEntry::Price( std::string val )
{
    // set the price from a string
    RsslBuffer b;
    b.data = const_cast<char *>(val.c_str()); // nothing is going to change
    b.length =(RsslUInt32) val.length();
    rsslNumericStringToReal(&price_, &b);

}

RsslUInt64 UPABookEntry::NumOrders() const
{
    return numOrders_;
}
void UPABookEntry::NumOrders(RsslUInt64 val)
{
    numOrders_ = val;
}

// the reuters sample providers dont include a side code in their data (only in the OMM fieldlist header structure) so we need to test whether
// the side code has been decoded from the OMM field list
bool UPABookEntry::HasSide()
{
    return sideCode_ != 'Z';
}

void UPALevel::AddEntry( UPABookEntry_ptr_t entry )
{

    if (entry->ActionCode()== 'D')
    {
        // if itd a delete then erase the order from this level
        orders_.erase(entry->Orderid());
    }
    else if (entry->ActionCode()== 'A')
    {
        // if its an add then add it
        orders_.insert(OrderIDSet_t::value_type(entry->Orderid()));
    }
    // and if its an update then it should already be there



    entryListDelta_.push_back(entry);
}

void UPALevel::ClearDeltas()
{
    // just clear down the list of deltas
    entryListDelta_.erase(entryListDelta_.begin(), entryListDelta_.end());
}

int UPALevel::AddEntriesToMsg( mamaMsg & msg )
{
    int numMsgs = 0;
    const BookFields &bookFields = UpaMamaCommonFields::BookFields();

    // grow the vector if we need to
    size_t originalSize = entryMsgs_.size();
    size_t numEntryMsgs = entryListDelta_.size();
    if (entryListDelta_.size() > entryMsgs_.size())
    {
        entryMsgs_.resize(numEntryMsgs);

        for(size_t index = originalSize; index < numEntryMsgs; index++)
        {
            mamaMsg newMsg;
            mamaMsg_createForPayload(&newMsg, MAMA_PAYLOAD_TICK42RMDS);
            entryMsgs_[index] = newMsg;
        }
    }

    while(entryListDelta_.size() > 0)
    {
        // get the next entry
        UPABookEntry_ptr_t entry = entryListDelta_.front();
        entryListDelta_.pop_front();

        // create a msg for it
        mamaMsg entryMsg;
        entryMsg = entryMsgs_[numMsgs];
        //t42log_debug("created message for level delta  at 0x%08x", (void*)entryMsg);

        entryMsgs_[numMsgs] = entryMsg;

        // and populate the message
        // 683|wEntryAction
        // 681|wEntryId
        // 682|wEntrySize
        // 685|wEntryTime
        // 686|wEntryStatus

        mamaMsg_addString(entryMsg, bookFields.wEntryId.mama_field_name.c_str(), bookFields.wEntryId.mama_fid, entry->Orderid().c_str());
        mamaMsg_addChar(entryMsg, bookFields.wEntryAction.mama_field_name.c_str(), bookFields.wEntryAction.mama_fid, entry->ActionCode());
        mamaMsg_addI64(entryMsg, bookFields.wEntrySize.mama_field_name.c_str(), bookFields.wEntrySize.mama_fid, (mama_u64_t)entry->Size());

        // todo add entry time and status
        ++numMsgs;
    }

    // 700|wPlEntries
    mamaMsg_addVectorMsg(msg, bookFields.wPlEntries.mama_field_name.c_str(), bookFields.wPlEntries.mama_fid, entryMsgs_.data(), numMsgs );
    // 659|wPlNumAttach|18
    mamaMsg_addI32(msg,bookFields.wPlNumAttach.mama_field_name.c_str(), bookFields.wPlNumAttach.mama_fid, numMsgs);

    return numMsgs;

}


UPALevel::~UPALevel()
{
    entryListDelta_.erase(entryListDelta_.begin(), entryListDelta_.end());

    orders_.erase(orders_.begin(), orders_.end());

    for(size_t index = 0; index < entryMsgs_.size(); index++)
    {
        mamaMsg_destroy(entryMsgs_[index]);
    }
}

UPALevel::UPALevel(RsslReal price, RsslUInt64 time, char sideCode, RsslInt size,UpaMamaFieldMap_ptr_t fieldmap)
   : price_(price), time_(time), dirty_(true), sideCode_(sideCode), fieldmap_(fieldmap)
   , actionCode_('A'), size_(size) // initialise the action code in the constructor to Add
{
    // create an initial set of 10 messages - this will grow if we need to
    entryMsgs_.resize(10);
    for(size_t index = 0; index < entryMsgs_.size(); index++)
    {
        mamaMsg newMsg;
      mamaMsg_createForPayload(&newMsg, MAMA_PAYLOAD_TICK42RMDS);
        entryMsgs_[index] = newMsg;
    }
}

void UPALevel::ClearMessageVector(mamaMsg msg)
{
    // just add an empty  message vector
    const BookFields &bookFields = UpaMamaCommonFields::BookFields();
    mamaMsg_addVectorMsg(msg, bookFields.wPlEntries.mama_field_name.c_str(), bookFields.wPlEntries.mama_fid, entryMsgs_.data(), 0 );
}



////////////////////////////////////////////////////////////////
//
// Book by price
//
///////////////////////////////////////////////////////////////

UPABookByPriceMessage::UPABookByPriceMessage()
    {

    }
UPABookByPriceMessage::~UPABookByPriceMessage()
    {

    }

bool UPABookByPriceMessage::StartUpdate()
    {
        entries_.erase(entries_.begin(), entries_.end());
        return true;
    }


bool UPABookByPriceMessage::AddEntry(UPABookEntry_ptr_t entry)
    {
        entries_.push_back(entry);
        return true;
    }


bool UPABookByPriceMessage::BuildMamdaMessage(mamaMsg msg)
    {
    int numLevels = 0;
    const BookFields &bookFields = UpaMamaCommonFields::BookFields();

    // grow the levelMsgs vector if we need to
    size_t numLevelMsgs = entries_.size();

    if (numLevelMsgs > levelMsgs_.size())
    {
        size_t originalSize = levelMsgs_.size();
        levelMsgs_.resize(numLevelMsgs);
        // create a new mamaMsg for each of the new elements
        for(size_t index = originalSize; index < numLevelMsgs; index++)
        {
            mamaMsg newMsg;
            mamaMsg_createForPayload(&newMsg, MAMA_PAYLOAD_TICK42RMDS);
            mamaMsg_addI32(newMsg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_UNKNOWN);
            levelMsgs_[index] = newMsg;
        }
    }


    // So walk the list of entries and create a level message for each

    while(entries_.size() > 0)
    {
        // remove the front of the list
        UPABookEntry_ptr_t entry = entries_.front();
        entries_.pop_front();

        // Mamda wants to identify the level with a mamaPrice
        RsslReal levelPrice = entry->Price();
        RsslDouble dblVal;
        rsslRealToDouble(&dblVal, &levelPrice);

        mamaPrice p;
        mamaPrice_create(&p);
        mamaPrice_setValue(p, dblVal);
        mamaPricePrecision prec = RsslHintToMamaPrecisionTo((RsslRealHints) levelPrice.hint, 653);
        mamaPrice_setPrecision(p, prec);

        mamaMsg levelMsg;
        levelMsg = levelMsgs_[numLevels];

        // This is for TSE
        if (entry->HaveOrderTone())
        {
            mamaMsg_addString(levelMsg, bookFields.OrderTone.mama_field_name.c_str(), bookFields.OrderTone.mama_fid, entry->OrderTone().c_str());
        }

        if (entry->HaveMmid())
        {
            mamaMsg_addString(levelMsg,UpaMamaCommonFields::CommonFields().wPartId.mama_field_name.c_str(), UpaMamaCommonFields::CommonFields().wPartId.mama_fid, entry->Mmid().c_str());
        }

        // 654|wPlSide
        mamaMsg_addChar(levelMsg, bookFields.wPlSide.mama_field_name.c_str(),bookFields.wPlSide.mama_fid, entry->SideCode());
        // 653|wPlPrice
        mamaMsg_addPrice(levelMsg, bookFields.wPlPrice.mama_field_name.c_str(),bookFields.wPlPrice.mama_fid, p); //p is copied and should be destroyed later on!
        mamaPrice_destroy(p);

        // add the entries for this level
        mamaMsg_addChar(levelMsg, bookFields.wPlAction.mama_field_name.c_str(),bookFields.wPlAction.mama_fid, entry->ActionCode());
        mamaMsg_addI64(levelMsg, bookFields.wPlSize.mama_field_name.c_str(),bookFields.wPlSize.mama_fid, (mama_u64_t)entry->Size());

        mamaDateTime dt =  makeDateTime((mama_u64_t)entry->Time());
        mamaMsg_addDateTime(levelMsg,bookFields.wPlTime.mama_field_name.c_str(),bookFields.wPlTime.mama_fid, dt);
        mamaDateTime_destroy(dt);

        levelMsgs_[numLevels] = levelMsg;

        if (entry->ActionCode() == 'D')
        {
            // if its a delete, we want to set the numentries to 0
            mamaMsg_addI32(levelMsg,bookFields.wPlNumEntries.mama_field_name.c_str(),bookFields.wPlNumEntries.mama_fid, 0 );
        }
        else
        {
            mamaMsg_addI32(levelMsg, bookFields.wPlNumEntries.mama_field_name.c_str(),bookFields.wPlNumEntries.mama_fid, (mama_i32_t)entry->NumOrders() );
        }

        ++numLevels;
    }

    // 4714|wBookType|18
    mamaMsg_addI32(msg, bookFields.wBookType.mama_field_name.c_str(),bookFields.wBookType.mama_fid, 2);
    // number of levels - is the number of entries int he level map
    // 651|wNumLevels
    mamaMsg_addI32(msg, bookFields.wNumLevels.mama_field_name.c_str(),bookFields.wNumLevels.mama_fid, numLevels );

    // 657|wPlNumEntries|18
    // no entries on the price level, its an aggregate
    mamaMsg_addI32(msg, bookFields.wPlNumEntries.mama_field_name.c_str(),bookFields.wPlNumEntries.mama_fid, 0 );

    //699|wPriceLevels|
    mamaMsg_addVectorMsg(msg,bookFields.wPriceLevels.mama_field_name.c_str(),bookFields.wPriceLevels.mama_fid, levelMsgs_.data(), numLevels );

    return true;

    }


