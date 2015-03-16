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
#ifndef __RMDS_UPADECODEUTILS_H__
#define __RMDS_UPADECODEUTILS_H__

class UPADecodeUtils
{
public:
	UPADecodeUtils(void);
	~UPADecodeUtils(void);
/**   
 * @brief get a date in the format of dd mmm yyyy and sets date, month and year to the corresponding integer values. The function will not check for if the string has correct values and depends on the RMDS input to correct.
 *
 * @param dateString: [input] Date string in the format "dd mmm yyyy"
 * @param date: [output] The date result.
 * @param month: [output] The month result.
 * @param year: [output] The year result.
 */
inline static void ToDateMonthYear(const char *dateString, int &date, int &month, int &year)
{
	date = (dateString[0]-'0')*10 + (dateString[1]-'0');
	year = (dateString[7]-'0')*1000 + (dateString[8]-'0')*100 + (dateString[9]-'0')*10 + (dateString[10]-'0');
	// Parsing the month is done only through the most important letters that differentiate between the months 
	// Other way to look at it is like creating a hash function keyed on the first letter of the month and then deal 
	// with collusions by checking the second and third letter in order to create a unique key
	// Date Format:  dd Mmm yyyy
	//                         1
	//               01234567890
	// It is considered to have only ONE format of the date!
	switch (dateString[3])
	{
	case 'A':
		if (dateString[4] == 'p')
			month = 4;                           //April
		else 
			month = 8;                           //August 
		break;
	case 'D':
		month = 12;                              //December
		break;
	case 'F':
		month = 2;                               //February
		break;
	case 'J':
		if (dateString[4] == 'a')
			month = 1;                           //January
		else if (dateString[5] == 'n')
			month = 6;                           //June
		else
			month = 7;                           //July
		break;
	case 'M':
		if (dateString[5] == 'r')
			month = 3;                           //March
		else
			month = 5;                           //May
		break;
	case 'N':
		month = 11;                              //November
		break;
	case 'O':
		month = 10;                              //October
		break;
	case 'S':
		month = 9;                               //September
		break;
	default:
		month = 0;                               //no month - parsing error!
	}

}

static bool DateTimeFromMidnightMs(mamaDateTime dateTime, RsslUInt64 ms_from_midnight);

};

#endif //__RMDS_UPADECODEUTILS_H__