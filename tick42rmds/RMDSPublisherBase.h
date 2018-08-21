
/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2014 Tick42 Ltd.
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

#include "RMDSPublisherSource.h"
#include "UPATransportNotifier.h"

// base class for interactive (RMDSPublisher) and non-interactive (RMDSNIPublisher)
// carries stuff for sources and for mama bridge context

class RMDSPublisherBase
{
public:
    RMDSPublisherBase(UPATransportNotifier &notify)
        : notify_(notify)
    { }

    virtual ~RMDSPublisherBase()
    { }

    virtual bool Initialize(mamaBridge bridge, mamaTransport transport, const std::string &transport_name) = 0;
    virtual bool Start() = 0;

    RMDSPublisherSource* GetSource() const
    {
        return publisherSource_;
    }

    // get the RsslChannel.
    // todo refactor.
    virtual RsslChannel* GetChannel() = 0;

    mamaQueue RequestQueue() const
    {
        return upaPublisherQueue_;
    }

    // get hold of the subscriber. This is where new item messages get sent to and where we get dictionaries and field map from
    RMDSSubscriber_ptr_t Subscriber() const
    {
        return subscriber_;
    }

    virtual bool SolicitedMessages() const
    {
        return true;
    }

    const UpaMamaFieldMap_ptr_t& FieldMap() const
    {
        return UpaMamaFieldMap_;
    }

    unsigned int MaxMessageSize() const
    {
        return maxMessageSize_;
    }

protected:
    // transport notification callbacks
    UPATransportNotifier notify_;

    RMDSPublisherSource *publisherSource_;

    // access to mama
    mamaBridge bridge_;
    mamaTransport transport_;
    std::string transportName_;
    unsigned int maxMessageSize_;

    mamaQueue upaPublisherQueue_;

    // subscriber we send new item requests to and get dictionary from
    RMDSSubscriber_ptr_t subscriber_;

    UpaMamaFieldMap_ptr_t UpaMamaFieldMap_;

    virtual bool createUpaMamaFieldMap();

};

