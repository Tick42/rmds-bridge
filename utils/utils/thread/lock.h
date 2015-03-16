#ifndef __THREAD_LOCK_H__
#define __THREAD_LOCK_H__

#include <mama/log.h>
#include <wlock.h>
#include <stdexcept>

namespace utils { namespace thread {

class lock_fail_init : public std::runtime_error
{
public:
	lock_fail_init() : std::runtime_error("Failed to initialize interlocked variable") {}
	virtual ~lock_fail_init() throw() {}
};

/*
 * lock_t is a simple lock thin wrapper class over the wlock functions
 */
class lock_t 
{
	wLock mLock;
public:
	lock_t () 
	{
		if (!(mLock = wlock_create()))
			throw lock_fail_init();
	} 
	~lock_t () {wlock_destroy( mLock );}
	void lock() {wlock_lock( mLock );}
	void unlock() {wlock_unlock( mLock );}
};

// and a wrapper around that so we can create on stack or in a scope
class T42Lock
{
public:

	T42Lock(lock_t * lock)
	{
		lock_ = lock;
		lock_->lock();
	}


	~T42Lock()
	{
		lock_->unlock();
	}

private:
	lock_t * lock_;


};

} /*namespace utils*/ } /*namespace thread*/

#endif //__THREAD_LOCK_H__