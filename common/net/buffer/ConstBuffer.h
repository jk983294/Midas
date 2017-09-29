#ifndef MIDAS_CONST_BUFFER_H
#define MIDAS_CONST_BUFFER_H

#include <boost/asio/buffer.hpp>
#include <memory>
#include <string>
#include "BufferData.h"
#include "MutableBuffer.h"

using namespace std;

namespace midas {

class ConstBuffer;
BufferDataPtr buffer_cast_helper(const ConstBuffer& b);

// reference counted non-modifiable buffer
class ConstBuffer {
public:
    BufferDataPtr bdp;
    boost::asio::const_buffer readBuffer;  // point to stored content

public:
    ConstBuffer() {}
    ConstBuffer(const string& str) : readBuffer(str.data(), str.size()) {}
    ConstBuffer(const boost::asio::const_buffer& str) : readBuffer(str) {}
    ConstBuffer(const void* p, size_t s) : readBuffer(p, s) {}
    ConstBuffer(const MutableBuffer& b) : bdp(buffer_cast_helper(b)), readBuffer(b.data(), b.size()) {}
    ConstBuffer(const MutableBuffer& b, size_t s)
        : bdp(buffer_cast_helper(b)), readBuffer(b.data(), std::min(s, b.size())) {}
    ConstBuffer(const MutableBuffer& b, size_t offset, size_t s)
        : bdp(buffer_cast_helper(b)), readBuffer(b.data() + offset, std::min(s, b.size() - offset)) {}
    ConstBuffer(const ConstBuffer& b, size_t s) : bdp(b.bdp), readBuffer(b.data(), std::min(s, b.size())) {}
    ConstBuffer(const ConstBuffer& b, size_t offset, size_t s)
        : bdp(b.bdp), readBuffer(b.data() + offset, std::min(s, b.size() - offset)) {}
    ConstBuffer(BufferDataPtr b) : bdp(b), readBuffer((b ? b->data : nullptr), (b ? b->prefix().size : 0)) {}

    uint64_t time() const { return bdp ? bdp->prefix().time : 0; }
    size_t source() const { return bdp ? bdp->prefix().source : 0; }
    size_t size() const { return boost::asio::buffer_size(readBuffer); }
    const char* data() const { return boost::asio::buffer_cast<const char*>(readBuffer); }

    bool empty() const { return boost::asio::buffer_size(readBuffer) == 0; }

    void swap(ConstBuffer& b) {
        bdp.swap(b.bdp);
        boost::asio::const_buffer tmp = readBuffer;
        readBuffer = b.readBuffer;
        b.readBuffer = tmp;
    }

    const boost::asio::const_buffer getReadBuffer() const { return readBuffer; }
    operator const boost::asio::const_buffer() const { return readBuffer; }

    // for ConstBufferSequence
    typedef boost::asio::const_buffer value_type;
    typedef const boost::asio::const_buffer* const_iterator;
    const_iterator begin() const { return &readBuffer; }
    const_iterator end() const { return &readBuffer + 1; }

private:
    friend BufferDataPtr buffer_cast_helper(const ConstBuffer& b);
};

inline BufferDataPtr buffer_cast_helper(const ConstBuffer& b) { return b.bdp; }
}

#endif
