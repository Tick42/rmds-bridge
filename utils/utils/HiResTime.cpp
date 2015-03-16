#include "HiResTime.h"

#include <math.h>

#include <sys/timeb.h>
#include <mama/mama.h>

HiResTime::HiResTime(void)
{
}


HiResTime::~HiResTime(void)
{
}

void HiResTime::SetMamaDateTime( mamaDateTime& dt ) const
{
	struct tm *today;


#ifdef WIN32
	// put win32 time stuff here is we do the port
	LARGE_INTEGER perfval;
	QueryPerformanceCounter(&perfval);

	__time64_t ltime;
	_time64( &ltime );

	struct __timeb64 timebuffer;
	_ftime64( &timebuffer );

	today = _localtime64( &(timebuffer.time ));
	unsigned short millitm = timebuffer.millitm;
	__int64 Time = timebuffer.time;
	perfval.QuadPart -= calloverhead;
	double perfvald = (double) perfval.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL );
	time_t ltime;
	time(&ltime);
	today = localtime(&ltime);
#endif

	double usec = perfvald/freqd;

	double usecCOrrected = usec+epocdiff;

	mamaDateTime_setDate(dt, today->tm_year + 1900, today->tm_mon + 1, today->tm_mday);

#ifdef WIN32


	mamaDateTime_setTime(dt, today->tm_hour, today->tm_min, today->tm_sec, perfvald/freqd+epocdiff );
#else

	mamaDateTime_setTime(dt, today->tm_hour, today->tm_min, today->tm_sec,tv.tv_usec );
#endif
	mamaDateTime_setPrecision(dt, MAMA_DATE_TIME_PREC_MICROSECONDS);


}

double HiResTime::GetTime() const
{
	// return the time value from the perf counter
	LARGE_INTEGER perfval;
	QueryPerformanceCounter(&perfval);

	perfval.QuadPart -= calloverhead;
	double perfvald = (double) perfval.QuadPart;
	return perfvald/freqd+epocdiff;

}

void HiResTime::Initialise()
{
#ifdef WIN32
	// determine resolution of Performance Counter
	LARGE_INTEGER c1,c2;
	__time64_t ltime, ctime;
	//fprintf(stderr,"mamalistenc2 : calibrating system timer\n");

	// determine frequency of perf timer
	QueryPerformanceFrequency(&freq);
	freqd = (double) freq.QuadPart;
	//fprintf(stderr,"mamalistenc2 : System clock %f GHz\n",freqd/1000000000.0);
	// to avoid the issue where the epoc timer changes during the perf counter synchronisation
	// read epoc time until the second changes
	_time64( &ctime );	// get unix epoc time
	_time64( &ltime );
	while( ctime == ltime ) _time64( &ltime );

	QueryPerformanceCounter(&c1);
	QueryPerformanceCounter(&c2);
	calloverhead = c2.QuadPart - c1.QuadPart;	// cost of calling above system call.
	// need to calculate epoc offset for perf counter
	modf((double) ltime - ((double) c1.QuadPart/ freqd),&epocdiff) ;
#endif//
}