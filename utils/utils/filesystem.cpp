/*
* Utils: Tick42 Middleware Utilities
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
#include <utils/filesystem.h>

namespace utils { namespace filesystem {

std::string absolute_path(std::string path)
{
	boost::filesystem::path abs_path = boost::filesystem::absolute(path);
	return abs_path.string();
}

bool path_exists(std::string path)
{
	return boost::filesystem::exists(path);
}

bool has_relative_path(std::string path)
{
	boost::filesystem::path fs_path = path;
	return fs_path.has_relative_path();
}

bool has_complete_path(std::string path)
{
	boost::filesystem::path fs_path = path;
	return fs_path.is_absolute();
}

std::string complete_path(const std::string &path_prefix, const std::string &path_suffix)
{

	boost::filesystem::path fs_path;
	fs_path = boost::filesystem::path(path_prefix) / boost::filesystem::path(path_suffix);
	return fs_path.string();
}

} /*namespace utils*/ } /*namespace filesystem*/ 
