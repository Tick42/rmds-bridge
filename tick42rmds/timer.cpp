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
#include "tick42rmdsbridgefunctions.h"

extern timerHeap gTimerHeap;

typedef struct rmdsTimerImpl_
{
	timerElement  timerElement_;
	double        interval_;
	mamaTimerCb   action_;
	void*         closure_;
	mamaTimer     parent_;
	wombatQueue   queue_;

    bool         destroying_;
	/* This callback will be invoked whenever the timer has been completely destroyed. */
	mamaTimerCb     mOnTimerDestroyed;

} rmdsTimerImpl_t;
 


static void MAMACALLTYPE
destroy_callback(void* timer, void* closure)
{
	rmdsTimerImpl_t* impl = (rmdsTimerImpl_t*)timer;

	mama_log(MAMA_LOG_LEVEL_FINEST,"bridge time destroy cb %08x", impl);

	if (impl->mOnTimerDestroyed != 0)
	{
		(*impl->mOnTimerDestroyed)(impl->parent_, impl->closure_);
	}

	free (impl);
}

static void MAMACALLTYPE
	timerQueueCb (void* data, void* closure)
{
	rmdsTimerImpl_t* impl = (rmdsTimerImpl_t*)data;

	if (impl->destroying_)
		return;

	if (impl->action_)
		impl->action_ (impl->parent_, impl->closure_);

}

static void
	timerCb (timerElement  timer,
	void*         closure)
{
	rmdsTimerImpl_t* impl = (rmdsTimerImpl_t*)closure;

	if (impl == NULL) return;

	// if we are being shutdown then dont reset
	if (impl->destroying_)
	{
		return;
	}
	/* Mama timers are repeating */

		/* Set the timer for the next firing */
	tick42rmdsBridgeMamaTimer_reset((timerBridge) closure);

	/* Enqueue the callback for handling */
	wombatQueue_enqueue (impl->queue_, timerQueueCb,
		(void*)impl, NULL);
}
  /*=========================================================================
   =                         Mandatory Functions                            =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaTimer_create (timerBridge*  result,
                            void*         nativeQueueHandle,
                            mamaTimerCb   action,
                            mamaTimerCb   onTimerDestroyed,
                            double        interval,
                            mamaTimer     parent,
                            void*         closure)
 {
	 rmdsTimerImpl_t* impl      =   NULL;
	 struct timeval timeout;

	 if (result == NULL) return MAMA_STATUS_NULL_ARG;

	 mama_log (MAMA_LOG_LEVEL_FINEST,
		 "%s Entering with interval [%f].",
		 "rmdsMamaTimer_create ():",
		 interval);

	 *result = NULL;

	 impl = (rmdsTimerImpl_t*)calloc (1, sizeof (rmdsTimerImpl_t));
	 if (impl == NULL) return MAMA_STATUS_NOMEM;

	 impl->queue_    = (wombatQueue)nativeQueueHandle;
	 impl->parent_   = parent;
	 impl->action_   = action;
	 impl->closure_  = closure;
	 impl->interval_ = interval;
	 impl->mOnTimerDestroyed = onTimerDestroyed;
	 impl->destroying_ = false;

	 mama_log(MAMA_LOG_LEVEL_FINEST,"bridge time create %08x", impl);

	 *result = (timerBridge)impl;

	 timeout.tv_sec = (long)interval;
	 timeout.tv_usec = (long)((interval-timeout.tv_sec) * 1000000.0);
	 if (0 != createTimer (&impl->timerElement_,
		 gTimerHeap,
		 timerCb,
		 &timeout,
		 impl))
	 {
		 mama_log (MAMA_LOG_LEVEL_ERROR,
			 "%s Failed to create RMDS timer.",
			 "mamaTimer_create ():");
		 return MAMA_STATUS_TIMER_FAILURE;
	 }

	 return MAMA_STATUS_OK;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaTimer_destroy (timerBridge timer)
 {
	 mama_status    returnStatus = MAMA_STATUS_OK;
	 rmdsTimerImpl_t* impl = NULL;

	 if (timer == NULL)
		 return MAMA_STATUS_NULL_ARG;
	 impl = (rmdsTimerImpl_t*)timer;

	 impl->action_ = NULL;

	 // set flag to say we are being destroyed
	 impl->destroying_ = true;

	 mama_log (MAMA_LOG_LEVEL_FINEST,
		 "timer 0x%8x %s Entering.", timer,
		 "rmdsMamaTimer_destroy ():");

	 if (0 != destroyTimer (gTimerHeap, impl->timerElement_))
	 {
		 mama_log (MAMA_LOG_LEVEL_ERROR,
			 "%s Failed to destroy rmds timer.",
			 "rmdsMamaTimer_destroy ():");
		 returnStatus = MAMA_STATUS_PLATFORM;
	 }

	 wombatQueue_enqueue (impl->queue_, destroy_callback,
		 (void*)impl, NULL);

	 return returnStatus;

 }
 
  /*=========================================================================
   =                        Reccomended Functions                           =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaTimer_reset (timerBridge timer)
 {
	 mama_status status      = MAMA_STATUS_OK;
	 rmdsTimerImpl_t* impl  = NULL;
	 struct timeval timeout;

	 if (timer == NULL)
		 return MAMA_STATUS_NULL_ARG;
	 impl = (rmdsTimerImpl_t*)timer;

	 timeout.tv_sec = (long)impl->interval_;
	 timeout.tv_usec = (long)((impl->interval_-timeout.tv_sec) * 1000000.0);

	 if (0 != destroyTimer (gTimerHeap, impl->timerElement_))
	 {
		 mama_log (MAMA_LOG_LEVEL_ERROR,
			 "%s Failed to destroy rmds timer.",
			 "rmdsMamaTimer_destroy ():");
		 tick42rmdsBridgeMamaTimer_destroy (timer);
		 status = MAMA_STATUS_PLATFORM;
	 }
	 else
	 {
		 if (0 != createTimer (&impl->timerElement_,
			 gTimerHeap,
			 timerCb,
			 &timeout,
			 impl))
		 {
			 mama_log (MAMA_LOG_LEVEL_ERROR,
				 "%s Failed to create rmds timer.",
				 "mamaTimer_create ():");
			 status = MAMA_STATUS_PLATFORM;
		 }
	 }

	 return status;

 }
 
 
 mama_status
 tick42rmdsBridgeMamaTimer_setInterval (timerBridge  timer,
                                 mama_f64_t   interval)
 {
	 rmdsTimerImpl_t* impl  = NULL;

	 if (timer == NULL)
		 return MAMA_STATUS_NULL_ARG;
	 impl = (rmdsTimerImpl_t*)timer;

	 impl->interval_ = interval;

	 return  tick42rmdsBridgeMamaTimer_reset (timer);
 }
 
 
 mama_status
 tick42rmdsBridgeMamaTimer_getInterval (timerBridge timer,
                                 mama_f64_t* interval)
 {
	 rmdsTimerImpl_t* impl  = NULL;

	 if (timer == NULL)
		 return MAMA_STATUS_NULL_ARG;
	 impl = (rmdsTimerImpl_t*)timer;

	 *interval = impl->interval_;
	 return MAMA_STATUS_OK;
 }
 
