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
#ifndef RMDS_INBOX_H__
#define RMDS_INBOX_H__

#include "bridge.h"

const char*
rmdsInboxImpl_getReplySubject(inboxBridge inbox);

/**
 * Send message to inbox
 * 
 * @param inbox The inbox to send the message
 * @param msg The address of the mamaMsg where the result is to be written
 */
mama_status
rmdsMamaInbox_send( mamaInbox inbox,
                    mamaMsg msg );

#endif /* RMDS_INBOX_H__ */
