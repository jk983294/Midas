#ifndef MIDAS_MUTABLE_BUFFER_STREAM_H
#define MIDAS_MUTABLE_BUFFER_STREAM_H

#include <ostream>
#include "net/buffer/MutableBuffer.h"

namespace midas {

class MutableBufferOStream : public std::basic_streambuf<char, std::char_traits<char>> {
public:
    typedef std::char_traits<char> TCTraits;
    typedef std::basic_streambuf<char, TCTraits> TBase;

    MutableBuffer& buffer;

public:
    MutableBufferOStream(MutableBuffer& b) : buffer(b) {
        // record start of buffer and its length
        setp(buffer.mutable_data(), buffer.mutable_data() + buffer.capacity());
        pbump(buffer.size());
    }

    int sync() {
        buffer.store(pcount() - buffer.size());
        return true;
    }

    int_type overflow(int_type c = TCTraits::eof()) {
        if (c == TCTraits::eof()) return TCTraits::not_eof(c);

        // expand buffer
        if (pptr() == epptr()) {
            sync();
            buffer.store(&c, 1);

            // begin new segment
            setp(buffer.mutable_data(), buffer.mutable_data() + buffer.capacity());
            pbump(buffer.size());
        }
        return TCTraits::to_int_type(c);
    }

    // how many characters is in stream buffer
    size_t pcount() const { return pptr() ? pptr() - pbase() : 0; }
};
}

#endif  // MIDAS_MUTABLE_BUFFER_STREAM_H
