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
#include "UPAStreamManager.h"
#include "UPASubscription.h"
#include "UPABridgePoster.h"

// for the moment just have a simple flat array of stream ids. later we can dynamically allocate in blocks if we want to
const RsslUInt32 StartStreamID = 16;
const unsigned int NumStreamIds = 0x40000; // 256k for the moment


inline RsslUInt32 Index2StreamId(RsslUInt32 Index)
{
    return Index+StartStreamID;
}

inline RsslUInt32 StreamId2Index( RsslUInt32 streamId)
{
    return streamId - StartStreamID;
}

void* UPAStreamManager::Track(void* p)
{
    UPAStreamManager* s = (UPAStreamManager*) p;
    const StatisticsLogger_ptr_t& statsLogger_ = StatisticsLogger::GetStatisticsLogger();
    while (true)
    {
        sleep(10);

        int inactive = 0;
        int live = 0;
        int ing = 0;
        int stale = 0;
        int other = 0;
        long sum = 0;
        {
            utils::thread::T42Lock lock(&s->streamLock_);
            pending_items_t::iterator it = s->pendingItems_.begin();
            while (s->pendingItems_.end() != it)
            {
                UPASubscription* sub = *it++;
                sum += (long) sub;
                UPASubscription::UPASubscriptionState state = sub->GetSubscriptionState();
                if (state == UPASubscription::SubscriptionStateSubscribing) ing++;
                else if (state == UPASubscription::SubscriptionStateInactive) inactive++;
                else if (state == UPASubscription::SubscriptionStateLive) live++;
                else if (state == UPASubscription::SubscriptionStateStale) stale++;
                else
                {
                    fprintf(stderr, "### state=%d\n", state);    // debug code only
                    other++;
                }
            }
        }
        fprintf(stderr, "PENDING: o=%d q=%d wait=%d live=%d inactive=%d subscribing=%d stale=%d other=%d sum=%ld\n",
            s->openItems_, statsLogger_->GetRequestQueueLength(), s->pendingItems_.size(), live, inactive, ing, stale, other, sum);
    }
    return NULL;
}

UPAStreamManager::UPAStreamManager()
    : pendingItems_(0)
   , openItems_(0)
   , pendingCloses_(0)
{
    ItemArray_ = new UPAItem_ptr_t [NumStreamIds];
    ::memset(ItemArray_, 0, sizeof(UPAItem_ptr_t *)*NumStreamIds);

    nextIndex_ = 0;
    nextPubIndex_ = 1;

    // wthread_t tid;
    // wthread_create(&tid, 0, &UPAStreamManager::Track, this);
}


UPAStreamManager::~UPAStreamManager(void)
{
    delete [] ItemArray_;
}

RsslUInt32 UPAStreamManager::AddItem(const UPASubscription_ptr_t& sub )
{
    utils::thread::T42Lock lock(&streamLock_);

    RsslUInt32 index;
    static bool firstUseOfQueue = true;

    if (nextIndex_ >= NumStreamIds)
    {
        if (firstUseOfQueue)
        {
            firstUseOfQueue = false;
            t42log_info("Begin using the StreamManager queue, MaxStreamIds=%d", NumStreamIds);
        }

        // first check to see if there is a free slot in the free list
        if (freeStreamIds_.size() > 0)
        {
            // grab the front of the list
            index = freeStreamIds_.front();
            freeStreamIds_.pop();
        }
        else
        {
            t42log_warn("!No more streamIDs\n");
            return 0;
        }
    }
    else
    {
         index = nextIndex_++;
    }

    RsslUInt32 streamId = Index2StreamId(index);
    UPAItem_ptr_t newItem = UPAItem_ptr_t(new UPAItem(streamId, sub));
    ItemArray_[index] = newItem;
    return streamId;
}

UPAItem_ptr_t UPAStreamManager::GetItem( RsslUInt32 streamId )
{
    RsslUInt32 index = StreamId2Index(streamId);
    return ItemArray_[index];
}

bool UPAStreamManager::ReleaseStreamId( RsslUInt32 streamId )
{
    utils::thread::T42Lock lock(&streamLock_);

    RsslUInt32 index = StreamId2Index(streamId);
    ItemArray_[index].reset();

    // add the index to the free list
    freeStreamIds_.push(index);
    return true;
}



UPAItem::UPAItem(RsslUInt32 streamId, const UPASubscription_ptr_t& sub )
    : sub_(sub), itemName_(sub->Symbol()), streamId_(streamId)
{

}

