#ifndef __INTERLOCKEDINT_H__
#define __INTERLOCKEDINT_H__

#include <wombat/wInterlocked.h>
#include <mama/log.h>
#include <stdexcept>

namespace utils { namespace thread {


class interlockedInt_fail_init : public std::runtime_error
{
public:
    interlockedInt_fail_init() : std::runtime_error("Failed to initialize interlocked variable") {}
    virtual ~interlockedInt_fail_init() throw() {}
};

/**
 * A thin wrapper over the wInterlockedInt
 */

class interlockedInt_t
{
    wInterlockedInt interlockedInt_;
public:
    interlockedInt_t() : interlockedInt_(0/*defaultValParam*/)  // first clean the value of the interlocked variable
    {
        if (!initialize())
            throw interlockedInt_fail_init();
    }

    ~interlockedInt_t()
    {
        destory();
    }
    /**
     * This function will destroy the internal interlocked integer value.
     *
     * @return true on success.
     */
    bool destory() {return (wInterlocked_destroy(&interlockedInt_) == 0);}
    /**
     * This function will initialize the internal interlocked integer value.
     *
     * @return true on success.
     */
    bool initialize()
    {
        return  (wInterlocked_initialize(&interlockedInt_) == 0);
    }
    /**
     * This function will atomically decrement the internal interlocked integer value.
     *
     * @return The decremented integer.
     */
    inline int decrement() { return wInterlocked_decrement(&interlockedInt_);}
    /**
     * This function will atomically increment the internal interlocked integer value.
     *
     * @return The decremented integer.
     */
    inline int increment() { return wInterlocked_increment(&interlockedInt_); }
    /**
     * This function will return the value of the interlocked variable.
     *
     * @return The value itself.
     */
    inline int read() {return wInterlocked_read(&interlockedInt_);}
    /**
     * This function will atomically set the internal interlocked 32-bit integer value.
     *
     * @param[in] newValue The new value to set.
     * @return The updated integer.
     */
    inline int set(int newValue) { return wInterlocked_set(newValue, &interlockedInt_); }
private:
    //InterlockedIntVariable variable cannot change or copy ownership of it's own internal variable!
    interlockedInt_t(const interlockedInt_t &rhs);
    interlockedInt_t &operator =(const interlockedInt_t &rhs);
};


} /*namespace utils*/ } /*namespace thread*/
#endif //__INTERLOCKEDINT_H__
