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

#include <utils/environment.h>
#include <utils/filesystem.h>
#include "RMDSFileSystem.h"

std::string GetActualPath( std::string path )
{
	using namespace utils;
	using namespace utils::filesystem;
	std::string actual_path=path;
	bool exist=false;

	if (has_relative_path(path))
	{
		std::string abs_path = absolute_path(path);
		if (path_exists(abs_path))
		{
			actual_path = abs_path;
			exist = true;
		}
		else
		{
			std::string env_prefix = environment::getWombatPath();
			abs_path = complete_path(env_prefix, path);
			if (path_exists(abs_path))
			{
				actual_path = abs_path;
				exist = true;
			}
		}
	}
	else
	{
		exist = path_exists(path);
	}

	if (exist)
		mama_log (MAMA_LOG_LEVEL_NORMAL, "getActualPath: The actual path of [%s] is [%s].",path.c_str(), actual_path.c_str());
	else
		actual_path = "";

	return actual_path;
}

