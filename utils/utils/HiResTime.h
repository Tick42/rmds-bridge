#pragma once

#ifdef WIN32
//#include <Windows.h>
#include <time.h>
#endif

#include <mama/mama.h>

class HiResTime
{
public:
    HiResTime(void);
    ~HiResTime(void);

    void Initialise();
    void SetMamaDateTime(mamaDateTime& dt) const;

    double GetTime() const;

private:
#ifndef WIN32
    struct itimerval ivalue;
    struct itimerval ovalue;
#else
    LARGE_INTEGER freq;
    double freqd;
    double epocdiff;
    __int64 calloverhead;
#endif
};

