#pragma once
#ifndef __UTILS_MAMA_TYPES_H__
#define __UTILS_MAMA_TYPES_H__

#include <mama/mama.h>

namespace utils { namespace mama {

/**
 * Support returning value and status for mama wrapper methods
 * gives ability to work in more functional way using the wrappers
 * For example, if wrapper method declared like that:
 * returned<int> someMethod()
 * it means that it can be used like the following 2 examples:
 * auto value = someMethod().value;
 * or
 * auto valueWstatus = someMethod();
 * if (valueWstatus.status == MAMA_STATUS_OK) do_this();
 *
 */
template <typename T>
struct returned
{
	T value;
	mama_status status;

	returned() : value(T()), status(MAMA_STATUS_OK) {}
	returned(T value, mama_status status) : value(value), status(status) {}
	returned &operator=(const T &rhs) 
	{
		if (this!=&rhs) 
		{
			this->value = rhs.value;
			this->status = rhs.status;
		}
		return *this;
	}
};

} /*namespace utils*/ } /*namespace mama*/
#endif //__UTILS_MAMA_TYPES_H__