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
// functions for sending Rssl Messages and Pings
//
#include "stdafx.h"

#include "UPAMessage.h"
#include <utils/t42log.h>

extern "C"
{
	extern	fd_set wrtfds; /* located in application */
}

// Send a message on a channel
RsslRet SendUPAMessage(RsslChannel* UPAChannel, RsslBuffer* msgBuffer)
{
	RsslError error;
	RsslRet	retval = 0;
	RsslUInt32 bytesWritten = 0;
	RsslUInt32 uncompressedBytesWritten = 0;
	RsslUInt8 writeFlags = RSSL_WRITE_NO_FLAGS;

	/* send the request */
	if ((retval = rsslWrite(UPAChannel, msgBuffer, RSSL_HIGH_PRIORITY, writeFlags, &bytesWritten, &uncompressedBytesWritten, &error)) > RSSL_RET_FAILURE)
	{
		// set write fd if there's still data queued 
		// flush is done by application 
		if (retval > RSSL_RET_SUCCESS)
		{
			FD_SET(UPAChannel->socketId, &wrtfds);
		}
	}
	else
	{
		if (retval == RSSL_RET_WRITE_CALL_AGAIN)
		{
			 // call flush and write again 
			while (retval == RSSL_RET_WRITE_CALL_AGAIN)
			{
				if ((retval = rsslFlush(UPAChannel, &error)) < RSSL_RET_SUCCESS)
				{
					t42log_error("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
				}
				retval = rsslWrite(UPAChannel, msgBuffer, RSSL_HIGH_PRIORITY, writeFlags, &bytesWritten, &uncompressedBytesWritten, &error);
			}
			 //set write fd if there's still data queued 
			 //flush is done by application 
			if (retval > RSSL_RET_SUCCESS)
			{
				FD_SET(UPAChannel->socketId, &wrtfds);
			}
		}
		else if (retval == RSSL_RET_WRITE_FLUSH_FAILED && UPAChannel->state != RSSL_CH_STATE_CLOSED)
		{
			// set write fd if flush failed 
			// flush is done by application 
			FD_SET(UPAChannel->socketId, &wrtfds);
		}
		else	// close the connection and return
		{
			// rsslWrite failed, release buffer 
			t42log_error("rsslWrite() failed with return code %d - <%s>\n", retval, error.text);
			rsslReleaseBuffer(msgBuffer, &error);
			return RSSL_RET_FAILURE;
		}
	}

	t42log_debug("SendMessage - %lu bytes written, %lu bytes uncompressed \n", bytesWritten, uncompressedBytesWritten);

	if ((retval = rsslFlush(UPAChannel, &error)) < RSSL_RET_SUCCESS)
	{
		//printf("rsslFlush() failed with return code %d - <%s>\n", retval, error.text);
	}
	else if (retval == RSSL_RET_SUCCESS)
	{
		 //clear write fd 
		FD_CLR(UPAChannel->socketId, &wrtfds);
	}


	return RSSL_RET_SUCCESS;
}


 //* Sends a ping to a channel.  Returns success if
 //* send ping succeeds or failure if it fails.
 //* chnl - The channel to send a ping to
 //
RsslRet SendPing(RsslChannel* UPAChannel)
{
	RsslError error;
	RsslRet ret = 0;

	if ((ret = rsslPing(UPAChannel, &error)) < RSSL_RET_SUCCESS)
	{
		t42log_error("rsslPing(): Failed on fd=%d with code %d\n", UPAChannel->socketId, ret);
		return ret;
	}
	else if (ret > RSSL_RET_SUCCESS)
	{
		 //set write fd if there's still data queued 
		 //flush is done by application 
		FD_SET(UPAChannel->socketId, &wrtfds);
	}

	//printf("sent ping\n");

	return RSSL_RET_SUCCESS;
}

