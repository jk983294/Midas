#ifndef MIDAS_BUFFER_MANAGER_H
#define MIDAS_BUFFER_MANAGER_H

#include <tbb/atomic.h>
#include "net/buffer/ConstBuffer.h"
#include "net/buffer/ConstBufferSequence.h"

namespace midas {

// manage output buffers for multi producer(deliver) single consumer(socket write) case
class BufferManager {
public:
    tbb::atomic<uint8_t> writePosition;
    tbb::atomic<uint8_t> refCount[2];
    ConstBufferSequence outputBuffers[2];  // one for producer, one for consumer

public:
    explicit BufferManager(size_t capacity = 32768) {
        outputBuffers[0].reserve(capacity);
        outputBuffers[1].reserve(capacity);
        writePosition = 0;
        refCount[0] = 0;
        refCount[1] = 0;
    }

    /**
     * push buffer from multiple producer and avoid conflict with buffer swap
     * @param msg
     * @return original buffer size
     */
    long push_back(const ConstBuffer& msg) {
        int pos = 0;
        // put barrier on write buffer
        while (true) {
            pos = writePosition;  // get current position
            refCount[pos].fetch_and_increment();
            if (pos == writePosition) break;
            // release barrier
            refCount[pos].fetch_and_decrement();
        }
        long s = outputBuffers[pos].push_back(msg) + msg.size();
        // release barrier
        refCount[pos].fetch_and_decrement();
        return s;
    }

    /**
     * swap buffer position, valid iff single consumer
     * @return
     */
    const ConstBufferSequence& swap() {
        int writePos = writePosition;
        int readPos = !writePos;
        // clear consumed buffer
        outputBuffers[readPos].clear();
        // swap buffer positions
        writePosition.fetch_and_store(readPos);  // full memory barrier required

        // wait while all push completes on the new read buffer (previous write buffer)
        while (refCount[writePos])
            ;

        return outputBuffers[writePos];  // safe to consume
    }

    bool empty() const { return outputBuffers[writePosition].empty(); }

    long pending_bytes() const { return outputBuffers[writePosition].byteSize; }

    long pending_buffers() const { return outputBuffers[writePosition].size(); }
};
}

#endif
