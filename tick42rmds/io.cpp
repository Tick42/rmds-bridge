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
 
  /*=========================================================================
   =                        Reccomended Functions                           =
   =========================================================================*/
 
 mama_status
 tick42rmdsBridgeMamaIo_create(ioBridge*   result,
                        void*       nativeQueueHandle,
                        uint32_t    descriptor,
                        mamaIoCb    action,
                        mamaIoType  ioType,
                        mamaIo      parent,
                        void*       closure)
 {
	 //return MAMA_STATUS_NOT_IMPLEMENTED;
	 return MAMA_STATUS_UNSUPPORTED_IO_TYPE;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaIo_destroy (ioBridge io)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
 
 mama_status
 tick42rmdsBridgeMamaIo_getDescriptor (ioBridge io, uint32_t *result)
 {
     return MAMA_STATUS_NOT_IMPLEMENTED;
 }
 
