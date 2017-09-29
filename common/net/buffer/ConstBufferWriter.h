#ifndef MIDAS_CONST_BUFFER_WRITER_H
#define MIDAS_CONST_BUFFER_WRITER_H

#include <boost/utility.hpp>
#include <vector>
#include "net/buffer/ConstBuffer.h"

using namespace std;

namespace midas {

class ConstBufferWriter : private boost::noncopyable {
public:
    typedef vector<ConstBuffer> BufferVec;
    BufferVec bufferVec[2];
    size_t storeIndex{0};
    bool writing{false};

public:
    explicit ConstBufferWriter(size_t sz) {
        bufferVec[0].reserve(sz);
        bufferVec[1].reserve(sz);
    }

    void store(const ConstBuffer& msg) { bufferVec[storeIndex].push_back(msg); }

    bool is_writing() const { return writing; }

    BufferVec& begin_writing() {
        assert(!writing);
        writing = true;
        return bufferVec[!storeIndex];
    }

    void end_writing() {
        assert(writing);
        bufferVec[!storeIndex].clear();
        storeIndex = !storeIndex;
        writing = false;
    }

    bool more_writing() const {
        assert(!writing);
        return !bufferVec[!storeIndex].empty();
    }

    void undo_writing() {
        assert(writing);
        writing = false;
    }

    void reset() {
        bufferVec[0].clear();
        bufferVec[1].clear();
        storeIndex = 0;
        writing = false;
    }

    bool empty() const { return bufferVec[storeIndex].empty(); }
};
}

#endif
