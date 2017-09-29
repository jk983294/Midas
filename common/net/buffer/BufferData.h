#ifndef MIDAS_BUFFER_DATA_H
#define MIDAS_BUFFER_DATA_H

#include <tbb/atomic.h>
#include <tbb/cache_aligned_allocator.h>
#include <tbb/scalable_allocator.h>
#include <boost/intrusive_ptr.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <memory>
#include <string>

namespace midas {

/**
 * BufferData = | BufferPrefix | payload |
 * allocate(n), note the n is payload size, actually sizeof(BufferPrefix) + n get allocated
 * BufferData.data() goes to the payload address directly
 */

typedef void* (*AllocatorFunctor)(std::size_t);
typedef void (*DeleterFunctor)(void*);

/**
 * data
 */
class BufferData;

class BufferPrefix {
public:
    size_t capacity;  // payload size
    size_t size{0};   // used space
    tbb::atomic<int32_t> references;
    size_t source{0};  // optional, id which identify source of data
    uint64_t time{0};  // receive time, nanosec since epoch
    AllocatorFunctor allocator;
    DeleterFunctor deleter;
    BufferData* next{nullptr};

    BufferData* data() { return reinterpret_cast<BufferData*>(this + 1); }
    BufferPrefix(size_t c, AllocatorFunctor a, DeleterFunctor d) : capacity(c), allocator(a), deleter(d) {
        references = 0;
    }
};

// dynamically allocate buffer data, works like union, used to access pool or allocated memory
class BufferData {
public:
    char data[1];
    BufferPrefix& prefix() { return *(reinterpret_cast<BufferPrefix*>(this) - 1); }
    int32_t add_ref() { return prefix().references.fetch_and_increment(); }
    int32_t remove_ref() { return prefix().references.fetch_and_decrement(); }
};

typedef boost::intrusive_ptr<BufferData> BufferDataPtr;

inline void intrusive_ptr_add_ref(BufferData* p) { p->add_ref(); }
inline void intrusive_ptr_release(BufferData* p) {
    if (p->remove_ref() == 1) {
        p->prefix().deleter(p);
    }
}

inline void heap_deleter(void* ptr) {
    BufferData* p = reinterpret_cast<BufferData*>(ptr);
    delete[](char*) & p->prefix();
}

inline void* heap_allocator(size_t n) {
    return (::new (::new char[sizeof(BufferPrefix) + n]) BufferPrefix(n, heap_allocator, heap_deleter))->data();
}

// for tbb non-pool
inline void tbb_deleter(void* ptr) {
    BufferData* p = reinterpret_cast<BufferData*>(ptr);
    ::scalable_free((void*)&p->prefix());
}

inline void* tbb_allocator(size_t n) {
    char* buffer = (char*)::scalable_malloc(sizeof(BufferPrefix) + n);
    BufferPrefix* prefix = new (buffer) BufferPrefix(n, tbb_allocator, tbb_deleter);
    return prefix->data();
}

// for tbb pool
template <size_t S, typename Tag>
inline void pool_deleter(void* ptr) {
    static const size_t poolSize = sizeof(BufferPrefix) + S;
    BufferData* p = reinterpret_cast<BufferData*>(ptr);
    boost::singleton_pool<Tag, poolSize>::free((void*)&p->prefix());
}

template <size_t S, typename Tag>
inline void* pool_allocator(size_t n) {
    static const size_t poolSize = sizeof(BufferPrefix) + S;
    char* buffer = (char*)boost::singleton_pool<Tag, poolSize>::malloc();
    BufferPrefix* prefix = new (buffer) BufferPrefix(n, pool_allocator<S, Tag>, pool_deleter<S, Tag>);
    return prefix->data();
}

// batch allocate, every time allocate chunkSize * S space
template <size_t S, typename Tag, typename Allocator, size_t chunkSize>
class FastBufferPool {
public:
    static const size_t poolSize = sizeof(BufferPrefix) + S;
    static tbb::atomic<BufferData*> first;
    static Allocator allocator;

public:
    static void free(void* ptr) {
        BufferData* p = reinterpret_cast<BufferData*>(ptr);
        BufferData* oldFirst = nullptr;
        do {
            oldFirst = first;
            p->prefix().next = oldFirst;
        } while (first.compare_and_swap(p, oldFirst) != oldFirst);
    }

    static void* malloc(size_t n) {
        BufferData* oldFirst = nullptr;
        do {
            oldFirst = first;
            if (oldFirst == nullptr) break;  // empty

            oldFirst->add_ref();  // increase reference to avoid ABA issue
            if (oldFirst != static_cast<BufferData*>(first)) {
                oldFirst->remove_ref();
                continue;
            }

            if (first.compare_and_swap(oldFirst->prefix().next, oldFirst) == oldFirst) break;  // find it

            oldFirst->remove_ref();
        } while (true);

        if (!oldFirst) {  // allocate new buffer
            register char* p = allocator.allocate(chunkSize * poolSize);
            for (size_t i = 0; i < chunkSize; ++i) {
                BufferData* newBuffer =
                    (::new (p) BufferPrefix(S, FastBufferPool::malloc, FastBufferPool::free))->data();
                if (i)
                    FastBufferPool::free(newBuffer);  // add to list
                else
                    oldFirst = newBuffer;

                p += poolSize;
            }
        } else {
            oldFirst->remove_ref();
            oldFirst->prefix().size = 0;
            oldFirst->prefix().next = nullptr;
        }
        return (void*)oldFirst;
    }

    static tbb::atomic<BufferData*> null() {
        static tbb::atomic<BufferData*> _null;
        _null = nullptr;
        return _null;
    }
};

template <size_t S, typename Tag, typename Allocator, size_t chunkSize>
tbb::atomic<BufferData*> FastBufferPool<S, Tag, Allocator, chunkSize>::first =
    FastBufferPool<S, Tag, Allocator, chunkSize>::null();

template <size_t S, typename Tag, typename Allocator, size_t chunkSize>
Allocator FastBufferPool<S, Tag, Allocator, chunkSize>::allocator;
}

#endif
