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

#ifndef RMDS_VALUE_TYPE_H
#define RMDS_VALUE_TYPE_H

#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>

#include <vector>
#include <string>
#include <cstdio>

#include "rmdsBridgeTypes.h"
#include "MamaDateTimeWrapper.h"
#include "MamaPriceWrapper.h"

struct mama_datetime
{
	uint64_t dt;
	friend std::ostream & operator<<(std::ostream& out, const mama_datetime & val);	
};

//////////////////////////////////////////////////////////////////////////
//
std::ostream& operator<<(std::ostream& out, const MamaMsgVectorWrapper_ptr_t v);

//////////////////////////////////////////////////////////////////////////
//
															   //which()
typedef boost::variant  <int8_t						//0   | MAMA_FIELD_TYPE_I8
					    ,uint8_t 							//1   | MAMA_FIELD_TYPE_U8
					    ,int16_t 							//2   | MAMA_FIELD_TYPE_I16
					    ,uint16_t 							//3   | MAMA_FIELD_TYPE_U16
					    ,int32_t 							//4	| MAMA_FIELD_TYPE_I32
					    ,uint32_t 							//5	| MAMA_FIELD_TYPE_U32
					    ,int64_t 							//6	| MAMA_FIELD_TYPE_I64
					    ,uint64_t 							//7	| MAMA_FIELD_TYPE_U64
					    ,float 								//8	| MAMA_FIELD_TYPE_F32
					    ,double								//9	| MAMA_FIELD_TYPE_F64
					    ,MamaDateTimeWrapper_ptr_t	//10	| MAMA_FIELD_TYPE_TIME
					    ,MamaPriceWrapper_ptr_t		//11	| MAMA_FIELD_TYPE_PRICE
					    ,MamaMsgPayloadWrapper_ptr_t	//12	| ?
					    ,MamaMsgVectorWrapper_ptr_t	//13	| ?
					    ,std::string						//14	| MAMA_FIELD_TYPE_STRING
					    ,bool								//15	| MAMA_FIELD_TYPE_BOOL
					    ,char								//16	| MAMA_FIELD_TYPE_CHAR
						> ValueType_t;
						
/*************************************************************************************************
 *                                     ValueTypeToString                                         *
 *************************************************************************************************/
struct ValueTypeToString : public boost::static_visitor<std::string>
{

	inline std::string operator()(int8_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%d",value);
		result = buf;
		return result;
	}
	inline std::string operator()(uint8_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%u",value);
		result = buf;
		return result;
	}
	inline std::string operator()(int16_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%d",value);
		result = buf;
		return result;
	}
	inline std::string operator()(uint16_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%u",value);
		result = buf;
		return result;
	}
	inline std::string operator()(int32_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%ld",value);
		result = buf;
		return result;
	}
	inline std::string operator()(uint32_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%lu",value);
		result = buf;
		return result;
	}
	inline std::string operator()(int64_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%lld",value);
		result = buf;
		return result;
	}
	inline std::string operator()(uint64_t value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%llu",value);
		result = buf;
		return result;
	}
	inline std::string operator()(float value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%f",value);
		result = buf;
		return result;
	}
	inline std::string operator()(double value) const
	{
		std::string result;
		char buf[50]={0};
		snprintf(buf,sizeof(buf)-1,"%f",value);
		result = buf;
		return result;
	}
	/**
	 * Get the date and/or time as a string.  If no date information is
	 * available, no date is printed in the resulting string.  The format
	 * for dates is YYYY-mm-dd (the ISO 8601 date format) and the format
	 * for times is HH:MM:SS.mmmmmmm (where the precision of the
	 * subseconds may vary depending upon any precision hints available).
	 *
	 * The timezone must to set to UTC if calling this from multiple
	 * threads concurrently to avoid contention in strftime.
	 */
	inline std::string operator()(MamaDateTimeWrapper_ptr_t value) const
	{
		char buf[56]={0};
		memset (buf, 0, sizeof(buf));
		mamaDateTime_getAsString(value->getMamaDateTime(),buf, sizeof(buf));
		return buf;
	}
	inline std::string operator()(MamaPriceWrapper_ptr_t value) const
	{
		char buf[56]={0};
		memset (buf, 0, sizeof(buf));
		mamaPrice_getAsString(value->getMamaPrice(),buf,sizeof(buf));
		return buf;
	}
	inline std::string operator()(MamaMsgPayloadWrapper_ptr_t value) const
	{
		return "MamaMsgPayload_N/A";
	}
	inline std::string operator()(MamaMsgVectorWrapper_ptr_t value) const
	{
		return "MamaMsgVector_N/A";
	}
	inline std::string operator()(std::string value) const
	{
		return value;
	}
	inline std::string operator()(bool value) const
	{
		return value ? "true" : "false";
	}
	inline std::string operator()(char value) const
	{
		char buf [2] = {0};
		buf[0]=value; 
		buf[1]=0;
		return buf;
	}
};

#endif //RMDS_VALUE_TYPE_H

