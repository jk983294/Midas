#ifndef COMMON_MIDAS_LOCK_H
#define COMMON_MIDAS_LOCK_H

#include <tbb/mutex.h>
#include <tbb/spin_mutex.h>
#include <mutex>
#include <thread>

/**
 * works like lock_guard, but provide wrapper for other locks
 */

namespace midas {

template <class T>
class ScopedLock {
    T& lock_m;

public:
    ScopedLock(T& lock) : lock_m(lock) { lock_m.lock(); }
    ~ScopedLock() { lock_m.unlock(); }
};

class NoLock {
    std::mutex lock_m;

public:
    typedef NoLock mutex;
    typedef midas::ScopedLock<NoLock> scoped_lock;

    NoLock() {}
    ~NoLock() {}

    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }

    void lock() const {}
    void unlock() const {}
    bool try_lock() const { return true; }

private:
    NoLock(const NoLock&) = delete;
    NoLock& operator=(const NoLock&) = delete;
};

struct TbbLock {
    typedef tbb::mutex mutex;
    typedef mutex::scoped_lock scoped_lock;
};

struct TbbSpinLock {
    typedef tbb::spin_mutex mutex;
    typedef mutex::scoped_lock scoped_lock;
};

class StdLock {
public:
    typedef std::mutex mutex;
    typedef std::lock_guard<mutex> scoped_lock;
};

#if (defined MIDAS_LOCK_TYPE_TBB) && (defined MIDAS_LOCK_TYPE_TBB_SPIN)
#error "Cannot define tbb lock and tbb spin lock together"
#elif (defined MIDAS_LOCK_TYPE_TBB)
typedef midas::TbbLock LockType;
#elif (defined MIDAS_LOCK_TYPE_TBB_SPIN)
typedef midas::TbbSpinLock LockType;
#elif (defined MIDAS_LOCK_TYPE_NOLOCK)
typedef midas::NoLock LockType;
#else
typedef midas::StdLock LockType;
#endif
}

#endif
