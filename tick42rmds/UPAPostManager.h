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

#include "rtr/rsslTransport.h"

#include "rmdsBridgeTypes.h"

#include <utils/thread/lock.h>

class PublisherPostMessageReply;
typedef boost::shared_ptr<PublisherPostMessageReply> PublisherPostMessageReply_ptr_t;

// hold postID / AckID keys to correlate posts with Acks
class UPAPostManager
{
public:
    UPAPostManager()
    {
        UPAPostManager::nextPostId_ = 0;
        maxPostId_ = (std::numeric_limits<RsslUInt32>::max)();
    }

    // add a post to the list and get the ID
    RsslUInt32 AddPost(const UPABridgePoster_ptr_t& poster, const PublisherPostMessageReply_ptr_t& reply);

    // get the poster back from the AckId
    //UPABridgePoster_ptr_t RemovePost(RsslUInt32 AckId);
    bool RemovePost(RsslUInt32 AckId, UPABridgePoster_ptr_t & poster, PublisherPostMessageReply_ptr_t& reply);

private:
    // hold information on key passed as to rmds as postID and returned as AckId
    // this will allow us to route acks to the poster that originated the post
    class UPAPostRecord
    {

    public:
        UPAPostRecord(RsslUInt32 postId, const UPABridgePoster_ptr_t& poster, const PublisherPostMessageReply_ptr_t& reply)
            : poster_(poster), id_(postId), reply_(reply) {}

        UPABridgePoster_ptr_t Poster() const { return poster_; }
        RsslUInt32 Id() const { return id_; }
        const PublisherPostMessageReply_ptr_t& Reply() const { return reply_; }

    private:
        RsslUInt32 id_;
        UPABridgePoster_ptr_t poster_;
        PublisherPostMessageReply_ptr_t reply_;

    };


    RsslUInt32 GetNextPostId()
    {
        if (nextPostId_ < maxPostId_ )
        {
            return nextPostId_ ++;
        }

        // otherwise need to restart at 0
        nextPostId_ = 0;

        return nextPostId_++;
    }

    typedef std::list<UPAPostRecord *> PostList_t;
    PostList_t postList_;

    mutable utils::thread::lock_t postListLock_;

    RsslUInt32 maxPostId_;
    RsslUInt32 nextPostId_;
};

