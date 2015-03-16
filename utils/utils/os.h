#pragma once
#ifndef __UTILS_OS_H__
#define __UTILS_OS_H__

#include <string>

namespace utils { namespace os {

/**   
 * @brief get the current OS logged in user name
 *
 * @param username: [output] a string to hold the given user name.
 * @return true is succeed
 */
bool getUserName(std::string &username);

/**   
 * @brief register a name for a thread with Visual Studio (Windows only)
 *
 * @param threadId: the threadID to register against
 * @param threadName: the friendly name for this thread
 */
void setThreadName(int threadID, const char *threadName);

} /*namespace utils*/ } /*namespace os*/

#endif //__UTILS_OS_H__
