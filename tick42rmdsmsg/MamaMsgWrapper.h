/*
* MamaMsgWrapper: The Reuters RMDS Bridge for OpenMama
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

#include <mama/msg.h>

#include <vector>
#include "upapayloadimpl.h"

//////////////////////////////////////////////////////////////////////////
//
class MamaMsgWrapper
{
public:
    MamaMsgWrapper(mamaMsg msg);
    ~MamaMsgWrapper();

    mamaMsg getMamaMsg() const;

private:
    mamaMsg mamaMsg_;
};

//////////////////////////////////////////////////////////////////////////
//
class MamaMsgPayloadWrapper
{
public:

    MamaMsgPayloadWrapper(msgPayload msg);
    ~MamaMsgPayloadWrapper();

    msgPayload getMamaMsgPayload() const;

private:
    msgPayload msgPayload_;
};

//////////////////////////////////////////////////////////////////////////
//
#if 0
class MamaMsgVectorWrapper
{
public:

    MamaMsgVectorWrapper(mamaMsg * vector, mama_size_t len)
        : msgVector_(vector), length_(len)
    {}

    ~MamaMsgVectorWrapper()
    {
        for(int i = 0; i < (int)length_; i++)
        {
            mamaMsg_clear(msgVector_[i]);
            mamaMsg_destroy(msgVector_[i]);
        }

    }

    mamaMsg * getMamaMsgVector() const
    {
        return msgVector_;
    }

    mama_size_t getVectorLength() const
    {
        return length_;
    }

private:


    mamaMsg * msgVector_;
    mama_size_t length_;
};

#else

class MamaMsgVectorWrapper
{
public:

    MamaMsgVectorWrapper()
        : length_(0)
    {}

    ~MamaMsgVectorWrapper();

    void addMessage(const msgPayload& payload)
    {
        payloadvector_.emplace_back(payload);
        ++length_;
    }
    mama_size_t getVectorLength() const
    {
        return length_;
    }

    msgPayload* GetVector()
    {
        return payloadvector_.data();
    }

private:

    typedef std::vector<msgPayload> MsgPayloadVector_t;

    MsgPayloadVector_t payloadvector_;

    //msgPayload * msgVector_;
    mama_size_t length_;
};

#endif
