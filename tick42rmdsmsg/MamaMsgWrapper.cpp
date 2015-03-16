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
#include "stdafx.h"
#include "MamaMsgWrapper.h"

#include "upamsgimpl.h"
#include "upapayload.h"

MamaMsgWrapper::MamaMsgWrapper(mamaMsg msg)
{
	mamaMsg_ = msg; 
}

MamaMsgWrapper::~MamaMsgWrapper(void)
{
	mamaMsg_destroy(mamaMsg_); 
}

mamaMsg MamaMsgWrapper::getMamaMsg() const
{
	return mamaMsg_;
}

MamaMsgPayloadWrapper::MamaMsgPayloadWrapper( msgPayload msgPayload )
{
	msgPayload_ = msgPayload;
}

MamaMsgPayloadWrapper::~MamaMsgPayloadWrapper(void)
{
	msgPayload_destroy((void*)msgPayload_);
}

msgPayload MamaMsgPayloadWrapper::getMamaMsgPayload() const
{
	return msgPayload_;
}

MamaMsgVectorWrapper::~MamaMsgVectorWrapper()
{
	for(int i = 0; i < (int)length_; i++)
	{
		msgPayload p = payloadvector_[i];


		//// we've had to take the payload out of the message here but need to destroy the parent otherwise it wont ever get destroyed
		//UpaPayload* payload = reinterpret_cast<UpaPayload*>(p);
		//mamaMsg m = payload->getParent();
		//mamaMsg_destroy(m);


		//tick42rmdsmsgPayload_destroy(msgPayload(p)); 
	}

}
