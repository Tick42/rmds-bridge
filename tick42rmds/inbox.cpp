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
 
#include "inboximpl.h"
#include "tick42rmdsbridgefunctions.h"
#include "rmdsdefs.h"
 
static const size_t uuidStringLen = 36;

typedef struct rmdsInboxImpl_t
{
	char                      inbox_[MAX_SUBJECT_LENGTH];
	mamaSubscription          _subscription;
	void*                     closure_;
	mamaInboxMsgCallback      msgCB_;
	mamaInboxErrorCallback    errCB_;
	mamaInboxDestroyCallback  inboxDestroyCB_;
	mamaInbox                 parent_;
} rmdsInboxImpl_t;


#define rmdsInbox(inbox) ((rmdsInboxImpl_t*)(inbox))
#define CHECK_INBOX(inbox) \
	do {  \
	if (rmdsInbox(inbox) == 0) return MAMA_STATUS_NULL_ARG; \
	} while(0)


static void MAMACALLTYPE
	rmdsInbox_onMsg(
	mamaSubscription    subscription,
	mamaMsg             msg,
	void*               closure,
	void*               itemClosure)
{
	if (!rmdsInbox(closure)) return;

	if (rmdsInbox(closure)->msgCB_)
		(rmdsInbox(closure)->msgCB_)(msg, rmdsInbox(closure)->closure_);
}

static void MAMACALLTYPE
	rmdsInbox_onCreate(
	mamaSubscription    subscription,
	void*               closure)
{
}

static void MAMACALLTYPE
	rmdsInbox_onDestroy(
	mamaSubscription    subscription,
	void*               closure)
{
	if (!rmdsInbox(closure)) return;

	if (rmdsInbox(closure)->inboxDestroyCB_)
		(rmdsInbox(closure)->inboxDestroyCB_)(rmdsInbox(closure)->parent_, rmdsInbox(closure)->closure_);
}

static void MAMACALLTYPE
	rmdsInbox_onError(
	mamaSubscription    subscription,
	mama_status         status,
	void*               platformError,
	const char*         subject,
	void*               closure)
{
	if (!rmdsInbox(closure)) return;

	if (rmdsInbox(closure)->errCB_)
		(rmdsInbox(closure)->errCB_)(status, rmdsInbox(closure)->closure_);
}

mama_status
	tick42rmdsBridgeMamaInbox_createByIndex (inboxBridge*           bridge,
	mamaTransport          transport,
	int                    tportIndex,
	mamaQueue              queue,
	mamaInboxMsgCallback   msgCB,
	mamaInboxErrorCallback errorCB,
	mamaInboxDestroyCallback onInboxDestroyed,
	void*                  closure,
	mamaInbox              parent)
{
	rmdsInboxImpl_t* impl = NULL;
	mama_status status = MAMA_STATUS_OK;
	if (!bridge || !transport || !queue || !msgCB) return MAMA_STATUS_NULL_ARG;
	impl = (rmdsInboxImpl_t*)calloc(1, sizeof(rmdsInboxImpl_t));
	if (!impl)
		return MAMA_STATUS_NOMEM;

	impl->closure_   = closure;
	impl->msgCB_     = msgCB;
	impl->errCB_     = errorCB;
	impl->parent_    = parent;
	impl->inboxDestroyCB_ = onInboxDestroyed;

	*bridge = (inboxBridge) impl;
	return MAMA_STATUS_OK;
}

mama_status
	tick42rmdsBridgeMamaInbox_create (inboxBridge*           bridge,
	mamaTransport          transport,
	mamaQueue              queue,
	mamaInboxMsgCallback   msgCB,
	mamaInboxErrorCallback errorCB,
	mamaInboxDestroyCallback onInboxDestroyed,
	void*                  closure,
	mamaInbox              parent)
{
	return tick42rmdsBridgeMamaInbox_createByIndex (bridge,
		transport,
		0,
		queue,
		msgCB,
		errorCB,
		onInboxDestroyed,
		closure,
		parent);
}

mama_status
	tick42rmdsBridgeMamaInbox_destroy (inboxBridge inbox)
{
	CHECK_INBOX(inbox);
	mamaSubscription_destroy(rmdsInbox(inbox)->_subscription);
	mamaSubscription_deallocate(rmdsInbox(inbox)->_subscription);
	free(rmdsInbox(inbox));
	return MAMA_STATUS_OK;
}


const char*
	rmdsInboxImpl_getReplySubject(inboxBridge inbox)
{
	if (!rmdsInbox(inbox))
		return NULL;
	return rmdsInbox(inbox)->inbox_;
}


mama_status
	rmdsMamaInbox_send( mamaInbox inbox, mamaMsg msg )
{
	if (!rmdsInbox(inbox))
		return MAMA_STATUS_INVALID_ARG;

	inboxBridge ib = mamaInboxImpl_getInboxBridge(inbox);
	rmdsInbox(ib)->msgCB_(msg, rmdsInbox(ib)->closure_);

	return MAMA_STATUS_OK;
}
