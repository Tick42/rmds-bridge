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
#if !defined(__MSG_H__)
#define __MSG_H__

// RMDS bridge message implementation

// use the set of types defined by QPID but may not need them all
typedef enum RMDSBridgeMsgType_t
{
    RMDS_MSG_PUB_SUB        =               0x00,
    RMDS_MSG_INBOX_REQUEST,
    RMDS_MSG_PUB_NEW_ITEM_REQUEST,
    RMDS_MSG_INBOX_RESPONSE,
    RMDS_MSG_SUB_REQUEST,
    RMDS_MSG_TERMINATE      =               0xff

} RMDSBridgeMsgType_t;


// accessors for bridge specific data

 mama_status tick42rmdsBridgeMamaMsgImpl_setMsgType (msgBridge msg, RMDSBridgeMsgType_t type);
 mama_status tick42rmdsBridgeMamaMsgImpl_getMsgType (msgBridge msg, RMDSBridgeMsgType_t * type);
 
 mama_status tick42rmdsBridgeMamaMsgImpl_setReplyTo (msgBridge msg, std::string source, std::string symbol);
 mama_status tick42rmdsBridgeMamaMsgImpl_getReplyTo (msgBridge msg, std::string & source, std::string & symbol);

 mama_status tick42rmdsBridgeMamaMsgImpl_increaseReferences(msgBridge msg);
 mama_status tick42rmdsBridgeMamaMsgImpl_decreaseReferences(msgBridge msg);

 mama_status tick42rmdsBridgeMamaMsgImpl_getReferences(msgBridge msg, size_t& references);

 mama_status tick42rmdsBridgeMamaMsgImpl_isDetached(msgBridge msg, bool& detached);

#endif // __MSG_H__
