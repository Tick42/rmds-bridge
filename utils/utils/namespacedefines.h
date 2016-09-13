#pragma once

#ifndef NAMESPACEDEFINES_H
#define NAMESPACEDEFINES_H

#if defined(STD_NAMESPACES) || !defined(BOOST_NAMESPACES)
#include <unordered_map>
#include <unordered_set>

#elif defined(BOOST_NAMESPACES)
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#endif

namespace utils
{
namespace collection
{

#if defined(STD_NAMESPACES) || !defined(BOOST_NAMESPACES)

// from std namespace
using std::unordered_map;
using std::unordered_set;

#elif defined(BOOST_NAMESPACES)

using boost::unordered_map;
using boost::unordered_set;

#endif

}
}

#endif // NAMESPACEDEFINES_H

