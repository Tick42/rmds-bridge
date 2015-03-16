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
#ifndef __UPASTREAMMANAGER_H__
#define __UPASTREAMMANAGER_H__

#include "rtr/rsslState.h"
#include "rmdsBridgeTypes.h"


// manage generation of stream IDs
// The 
class UPAItem;

class UPAItem
{

public: 
   UPAItem(RsslUInt32 streamId, UPASubscription_ptr_t sub);

   ~UPAItem()
   {
   }

   UPASubscription_ptr_t Subscription() { return sub_; }

private:

   RsslUInt32 streamId_;
   std::string itemName_;

   UPASubscription_ptr_t sub_;

} ;



class UPAStreamManager
{
public:
   UPAStreamManager();
   ~UPAStreamManager();

   // subscriber items
   RsslUInt32 AddItem(UPASubscription_ptr_t sub);

   UPAItem_ptr_t GetItem(RsslUInt32 streamId);

   bool ReleaseStreamId(RsslUInt32 streamId);

   // manage the pending items count
   // at the moment this all runs on the single upa thread so no need for serializing access
   RsslUInt64 countPendingItems() const
   {
      return pendingItems_.size();
   }

   void addPendingItem(UPASubscription *sub)
   {
      pendingItems_.insert(sub);
   }

   void removePendingItem(UPASubscription *sub)
   {
      pending_items_t::iterator it = pendingItems_.find(sub);
      if (pendingItems_.end() != it)
      {
         pendingItems_.erase(it);
      }
   }

   // The number of items that have been opened
   RsslUInt64 OpenItems() const
   {
      return  openItems_;
   }

   RsslUInt64 IncOpenItems()
   {
      return ++openItems_;
   }

   RsslUInt64 DecOpenItems()
   {
      return --openItems_;
   }

   // The number of items that are waiting to be closed
   RsslUInt64 PendingCloses() const
   {
      return  pendingCloses_;
   }

   RsslUInt64 IncPendingCloses()
   {
      return ++pendingCloses_;
   }

   RsslUInt64 DecPendingCloses()
   {
      return --pendingCloses_;
   }

private:
   UPAItem_ptr_t * ItemArray_;

   RsslUInt32 nextIndex_;
   RsslUInt32 nextPubIndex_;
   typedef std::unordered_set<UPASubscription *> pending_items_t;
   pending_items_t pendingItems_;
   RsslUInt64 openItems_;
   RsslUInt64 pendingCloses_;

   // free list
   typedef std::list<RsslUInt32> streamIdList_t;
   streamIdList_t freeStreamIds_;
};

#endif //__UPASTREAMMANAGER_H__