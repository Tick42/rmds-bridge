#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <mama/log.h>
#include <wombat/port.h>
#include <stdexcept>

namespace utils { namespace thread {


class semaphore_fail_init : public std::runtime_error
{
public:
    semaphore_fail_init() : std::runtime_error("Failed to initialize semaphore variable") {}
    virtual ~semaphore_fail_init() throw() {}
};

/**
* semaphore_t is a thin wrapper over the POSIX like semaphore as implemented in Wombat library
*/
class semaphore_t
{
    mutable wsem_t semaphore_; // a mutex is always mutable
public:
    semaphore_t() {if (!init (0, 1)) throw semaphore_fail_init();}
    ~semaphore_t() {destroy();}
    /*dummy relates to POSIX pshared parameter that doesn't work on linux systems and therefore is not supported here.*/
    semaphore_t(int dummy, int count) {if (!init(dummy, count)) throw semaphore_fail_init();}
    inline bool init(
        int dummy, int count) { return (wsem_init (&semaphore_, dummy, count) == 0);
    }
    inline bool destroy() { return (wsem_destroy (&semaphore_) == 0);}
    inline bool post() {
        return (wsem_post (&semaphore_) ==0);
    } //TODO
    inline bool wait() { return (wsem_wait (&semaphore_)==0); }
    inline bool timedwait(unsigned int ts) {return (wsem_timedwait (&semaphore_, ts) ==0);}
    inline bool trywait() {return (wsem_trywait (&semaphore_)==0);}
    inline bool getvalue(int* items) {return  (wsem_getvalue (&semaphore_, items)==0);}
private:
    //semaphore_t variable cannot change or copy ownership of it's own internal mutex!
    semaphore_t(const semaphore_t &rhs);
    semaphore_t&operator =(const semaphore_t &rhs);
};


} /*namespace utils*/ } /*namespace thread*/

#endif //__SEMAPHORE_H__
