#ifndef MIDAS_CONST_BUFFER_SEQUENCE_H
#define MIDAS_CONST_BUFFER_SEQUENCE_H

#include <tbb/atomic.h>
#include <tbb/concurrent_vector.h>
#include <iterator>
#include <memory>
#include <string>
#include "ConstBuffer.h"

using namespace std;

namespace midas {

class ConstBufferSequence {
public:
    typedef tbb::concurrent_vector<ConstBuffer> TBufferSequence;
    typedef ConstBuffer value_type;
    typedef TBufferSequence::const_iterator const_iterator;

    shared_ptr<TBufferSequence> bufferSeq;
    tbb::atomic<long> byteSize;  // number of pending bytes
    size_t source{0};            // id identify source of data
    uint64_t time{0};            // receive time in nano second since epoch

public:
    ConstBufferSequence() : bufferSeq(new TBufferSequence) { byteSize = 0; }

    void reserve(size_t size) { bufferSeq->reserve(size); }

    size_t size() const { return bufferSeq->size(); }

    bool empty() const { return byteSize == 0; }

    long push_back(const ConstBuffer& buffer) {
        bufferSeq->push_back(buffer);
        return byteSize.fetch_and_add(buffer.size());
    }

    void clear() {
        byteSize = 0;
        time = 0;
        bufferSeq->clear();
    }

    const_iterator begin() const { return bufferSeq->begin(); }
    const_iterator end() const { return bufferSeq->end(); }
};
}

#endif
