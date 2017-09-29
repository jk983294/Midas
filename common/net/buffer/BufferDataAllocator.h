#ifndef MIDAS_BUFFER_DATA_ALLOCATOR_H
#define MIDAS_BUFFER_DATA_ALLOCATOR_H

#include <climits>
#include "BufferData.h"
#include "net/common/NetData.h"

namespace midas {

template <size_t S>
struct BufferSize {
    static const size_t size = S - sizeof(BufferPrefix);
};

struct HeapAllocator {
    static const size_t max_size = ULLONG_MAX;
    void* operator()(size_t n) const { return heap_allocator(n); }
};

struct TbbAllocator {
    static const size_t max_size = ULLONG_MAX;
    void* operator()(size_t n) const { return tbb_allocator(n); }
};

template <size_t S, typename Tag = DefaultPoolTag>
struct PoolAllocator {
    static const size_t max_size = S;  // for chunk allocation
    void* operator()(size_t n) const { return pool_allocator<S, Tag>(n); }
};

template <size_t S, typename Tag = DefaultPoolTag, typename AllocatorType = tbb::cache_aligned_allocator<char>,
          size_t ChunkSize = 256>
struct FastPoolAllocator {
    static const size_t max_size = S;  // for chunk allocation
    void* operator()(size_t n) const { return FastBufferPool<S, Tag, AllocatorType, ChunkSize>::malloc(n); }
};

template <typename AllocT1 = HeapAllocator, typename AllocT2 = HeapAllocator, typename AllocT3 = HeapAllocator>
struct Allocator {
    typedef char value_type;
    typedef char* pointer;
    typedef const char* const_pointer;
    typedef char& reference;
    typedef const char& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    AllocT1 alloc1;
    AllocT2 alloc2;
    AllocT3 alloc3;
    HeapAllocator heapAllocator;

    Allocator() {}
    ~Allocator() {}

    pointer operator()(size_t n) const { return; }

    pointer allocate(size_type n, const_pointer = 0) const {
        BufferData* data(
            new (n <= AllocT1::max_size
                     ? alloc1(n)
                     : (n <= AllocT2::max_size ? alloc2(n) : (n <= AllocT3::max_size ? alloc3(n) : heapAllocator(n))))
                BufferData);
        data->add_ref();
        return data->data;
    }

    void deallocate(pointer& p, size_type n) const {
        BufferData* data = reinterpret_cast<BufferData*>(p);
        if (data->remove_ref() == 1) data->prefix().deleter(data);
    }
};
}

#endif
