#include "stdafx.h"

#include <ctime>
#include "utils/time.h"

#ifdef WIN32
#else
#endif

namespace utils
{
namespace time
{

#ifdef WIN32
static void FileTimeToTimeT(time_t& result, const FILETIME& input)
{
   /* Convert FILETIME one 64 bit number so we can work with it. */
    ULARGE_INTEGER ull;
    ull.LowPart = input.dwLowDateTime;
    ull.HighPart = input.dwHighDateTime;

    result = ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

time_t GetSeconds(uint32_t year, uint32_t month, uint32_t day)
{
    time_t result = 0;
#ifdef WIN32
    SYSTEMTIME st;
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wDayOfWeek = -1;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    FileTimeToTimeT(result, ft);
#else
    struct tm dateTime;
    dateTime.tm_year = year - 1900;
    dateTime.tm_mon = month - 1;
    dateTime.tm_mday = day;
    dateTime.tm_hour = 0;
    dateTime.tm_min = 0;
    dateTime.tm_sec = 0;
    dateTime.tm_isdst = 0;

    result = timegm(&dateTime);
#endif

    return result;
}

}
}
