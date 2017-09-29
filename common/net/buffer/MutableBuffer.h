#ifndef MIDAS_MUTABLE_BUFFER_H
#define MIDAS_MUTABLE_BUFFER_H

#include <boost/asio/buffer.hpp>
#include <climits>
#include "BufferData.h"
#include "BufferDataAllocator.h"

namespace midas {

class MutableBuffer;
// BufferDataPtr buffer_cast_helper(const MutableBuffer& b);

// reference counted modifiable buffer
class MutableBuffer {
public:
    BufferDataPtr bdp;
    boost::asio::mutable_buffer writeBuffer;  // point to free buffer
    boost::asio::const_buffer readBuffer;     // point to stored content

public:
    explicit MutableBuffer(size_t s)
        : bdp(new (heap_allocator(s)) BufferData), writeBuffer(bdp->data, s), readBuffer(bdp->data, 0) {}

    template <typename AllocatorType>
    explicit MutableBuffer(size_t s, const AllocatorType& a)
        : bdp(new (a(s)) BufferData), writeBuffer(bdp->data, s), readBuffer(bdp->data, 0) {}

    // copy from rhs to this object from given rhsOffset to this offset
    MutableBuffer(const MutableBuffer& rhs, size_t rhsOffset, size_t offset = 0)
        : bdp(new (rhs.allocator()(rhs.capacity())) BufferData),
          writeBuffer(bdp->data + offset, capacity() - offset),
          readBuffer(bdp->data, offset) {
        bdp->prefix().size = offset;
        if (rhs.size() > rhsOffset) {
            size_t len = rhs.size() - rhsOffset;
            assert(len <= capacity() - offset);
            memcpy(bdp->data + offset, rhs.bdp->data + rhsOffset, len);
            store(len);
        }
    }

    void store(size_t s) {
        bdp->prefix().size += s;
        readBuffer = boost::asio::const_buffer(bdp->data, size());
        writeBuffer = boost::asio::mutable_buffer(bdp->data + size(), capacity() - size());
    }

    void store(const void* data, size_t s) {
        size_t newSize = size() + s;
        if (capacity() < newSize) {
            // reallocate 50% larger buffer
            size_t newCapacity = newSize + newSize / 2;
            BufferDataPtr newData(new (heap_allocator(newCapacity)) BufferData);
            size_t oldSize = size();
            memcpy(newData->data, bdp->data, oldSize);
            newData->prefix().size = oldSize;

            bdp = newData;
            readBuffer = boost::asio::const_buffer(bdp->data, size());
            writeBuffer = boost::asio::mutable_buffer(bdp->data + size(), capacity() - size());
        }
        // append given data
        memcpy(boost::asio::buffer_cast<char*>(writeBuffer), data, s);
        store(s);
    }

    void time(uint64_t value) { bdp->prefix().time = value; }
    uint64_t time() const { return bdp->prefix().time; }
    void source(size_t value) { bdp->prefix().source = value; }
    size_t source() const { return bdp->prefix().source; }
    size_t size() const { return bdp->prefix().size; }
    size_t capacity() const { return bdp->prefix().capacity; }
    size_t writeCapacity() const { return bdp->prefix().capacity - bdp->prefix().size; }
    const char* data() const { return bdp->data; }
    char* mutable_data() const { return bdp->data; }
    char* write_ptr() const { return bdp->data + bdp->prefix().size; }

    // reset buffer content
    void clear() {
        bdp->prefix().size = 0;
        readBuffer = boost::asio::const_buffer(bdp->data, 0);
        writeBuffer = boost::asio::mutable_buffer(bdp->data, capacity());
    }

    void swap(MutableBuffer& b) {
        MutableBuffer tmp(*this);
        *this = b;
        b = tmp;
    }

    // get pointer to free / not used part of buffer
    template <typename T>
    T write_buffer() {
        return boost::asio::buffer_cast<T>(writeBuffer);
    }

    template <typename Tag>
    static MutableBuffer fast_allocate(size_t s) {
        static const size_t fastSize1 = BufferSize<128>::size;
        typedef FastPoolAllocator<fastSize1, Tag> PoolAlloc1;
        static const size_t fastSize2 = BufferSize<256>::size;
        typedef FastPoolAllocator<fastSize2, Tag> PoolAlloc2;
        static const size_t fastSize3 = BufferSize<512>::size;
        typedef FastPoolAllocator<fastSize3, Tag> PoolAlloc3;

        if (s < fastSize1)
            return MutableBuffer(s, PoolAlloc1());
        else if (s < fastSize2)
            return MutableBuffer(s, PoolAlloc2());
        else if (s < fastSize3)
            return MutableBuffer(s, PoolAlloc3());
        else
            return MutableBuffer(s);
    }

    // for MutableBufferSequence
    typedef boost::asio::mutable_buffer value_type;
    typedef const boost::asio::mutable_buffer* const_iterator;
    const_iterator begin() const { return &writeBuffer; }
    const_iterator end() const { return &writeBuffer + 1; }

private:
    MutableBuffer(BufferDataPtr buffer)
        : bdp(buffer),
          writeBuffer(bdp->data + bdp->prefix().size, bdp->prefix().capacity - bdp->prefix().size),
          readBuffer(bdp->data, bdp->prefix().size) {}

    friend BufferDataPtr buffer_cast_helper(const MutableBuffer& b);

    AllocatorFunctor allocator() const {
        if (bdp) return bdp->prefix().allocator;
        return &heap_allocator;
    }

    DeleterFunctor deleter() const {
        if (bdp) return bdp->prefix().deleter;
        return &heap_deleter;
    }
};

inline BufferDataPtr buffer_cast_helper(const MutableBuffer& b) { return b.bdp; }
}

#endif
