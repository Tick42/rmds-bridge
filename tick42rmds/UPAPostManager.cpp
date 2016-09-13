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
#include "UPAPostManager.h"

#include <utils/t42log.h>


// When we post to the RMDS we want a PostID that is inserted into the Rssl Message. This is returned in the response message as the Ack ID
// we can use this identify the poster and route the ack message to it

RsslUInt32 UPAPostManager::AddPost(UPABridgePoster_ptr_t poster, const PublisherPostMessageReply_ptr_t& reply)
{
    RsslUInt32 ret = GetNextPostId();

    // create a new post record and add it to the list
    UPAPostRecord * record = new UPAPostRecord(ret, poster, reply);

    utils::thread::T42Lock l(&postListLock_);
    postList_.push_back(record);

    return ret;
}

bool UPAPostManager::RemovePost(RsslUInt32 AckId, UPABridgePoster_ptr_t & poster, PublisherPostMessageReply *& reply)
{
    bool ret = false;
    // search the list for the post ID

    utils::thread::T42Lock l(&postListLock_);
    // as we can expect the responses to arrive in more or less the same order as the posts it will normally be the head of the list
    PostList_t::iterator it = postList_.begin();

    int listItemsSearched = 0;
    while (it != postList_.end())
    {
        if ((*it)->Id() == AckId)
        {
            // we've found what we are looking for so....
            // grab the data
            UPAPostRecord * record = *it;
            poster = record->Poster();
            reply = record->Reply();
            // clean up
            delete record;
            postList_.erase(it);
            ret = true;
            // and drop out of the loop
            break;
        }

        ++listItemsSearched;
        ++it;

    }

    // log something to see if the assumption that reposnses will be in the same order as posts is actually valid
    // if it is exactly the same order then the repsonse will all ways be on the list head. This will give us an idea if it isnt
    if (listItemsSearched > 0)
    {
        t42log_debug("UPAPostManager searched %d items to match post ID %d \n", listItemsSearched, AckId);
    }
    return ret;
}
