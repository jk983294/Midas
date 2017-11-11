#ifndef MIDAS_PAYLOAD_H
#define MIDAS_PAYLOAD_H

#include <net/buffer/ConstBuffer.h>
#include <net/buffer/MutableBuffer.h>
#include <stdlib.h>
#include <boost/asio/buffer.hpp>
#include <cstddef>
#include <type_traits>
#include "utils/MidasUtils.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

/**
 * this is for object based msg not string based msg
 * this is designed for SequentialConsumerStrategy not BatchConsumerStrategy
 * since we don't know how to merge msgs
 */
template <class T>
class PayloadObject {
public:
    typedef T ElementType;

    bool isValid{false};
    bool hasMoreData{false};
    uint64_t rcvt{0};
    int64_t id{0};
    long sequenceValue{-1};
    T data_;

public:
    /**
     * bufferSize has no effect here, purely for compatible with string based payload
     */
    PayloadObject(const uint32_t bufferSize = 0) {}

    PayloadObject(const PayloadObject& other)
        : isValid(other.isValid),
          hasMoreData(other.hasMoreData),
          rcvt(other.rcvt),
          id(other.id),
          sequenceValue(other.sequenceValue),
          data_(other.data_) {}

    ~PayloadObject() {}

    PayloadObject& operator=(const PayloadObject& other) {
        if (this == &other) return *this;

        rcvt = other.rcvt;
        id = other.id;
        sequenceValue = other.sequenceValue;
        data_ = other.data_;
        return *this;
    }

    void set_sequence_value(const long value, long cursorWhenAdded) {
        sequenceValue = value;
        (void)cursorWhenAdded;
    }

    long get_sequence_value() const { return sequenceValue; }

    bool set_value(const T& d, const uint64_t rcvt_, uint64_t id_) {
        rcvt = rcvt_;
        id = id_;
        data_ = d;
        return true;
    }

    const T& get_data() const { return data_; }

    bool is_valid() { return isValid; }

    void set_is_valid(bool isValid_) { isValid = isValid_; }

    void reset() { hasMoreData = false; }

    uint64_t get_rcvt() const { return rcvt; }
    int64_t get_id() const { return id; }

    void set_has_more_data(bool hasMoreData_) { hasMoreData = hasMoreData_; }

    bool has_more_data() { return hasMoreData; }

    template <class U>
    friend ostream& operator<<(ostream& s, const PayloadObject<U>);
};

template <class T>
ostream& operator<<(ostream& s, const PayloadObject<T> payload) {
    s << "id_ " << payload.get_id() << " ,rcvt " << payload.get_rcvt() << " ," << payload.get_data();
    return s;
}

/**
 * string based msg
 */
class Payload {
public:
    typedef std::true_type BroadcastSupported;  // Payload can be used in broadcast or single dispatch mode

    /**
     * constructor
     * @param bufferSize set to 0 to not deep copy the data
     */
    Payload(const uint32_t bufferSize = 0) : recvBufferSize(bufferSize) {
        if (bufferSize) {
            data_ = new char[bufferSize];
            memset(data_, 0, bufferSize);
        }
    }

    Payload(const Payload& other)
        : rcvt(other.rcvt),
          recvBufferSize(other.recvBufferSize),
          size(other.size),
          sequenceValue(other.sequenceValue),
          id(other.id),
          isValid(other.isValid) {
        if (other.recvBufferSize) {
            // deep copy
            data_ = new char[other.recvBufferSize];
            memcpy(data_, other.data_, other.size);
        } else {
            data_ = other.data_;
        }
    }

    ~Payload() {
        if (recvBufferSize) {
            delete[] data_;
        }
    }

    Payload& operator=(const Payload& other) {
        if (this == &other) return *this;

        if (recvBufferSize) {
            delete[] data_;
        }

        sequenceValue = other.sequenceValue;
        rcvt = other.rcvt;
        id = other.id;
        recvBufferSize = other.recvBufferSize;
        size = other.size;
        isValid = other.isValid;

        if (other.recvBufferSize) {
            // deep copy
            data_ = new char[other.recvBufferSize];
            memcpy(data_, other.data_, other.size);
        } else {
            data_ = other.data_;
        }

        return *this;
    }

    void set_sequence_value(const long value, long cursorWhenAdded) {
        sequenceValue = value;
        (void)cursorWhenAdded;
    }

    long get_sequence_value() const { return sequenceValue; }

    bool set_value(const char* p, const long sz, const uint64_t rcvt_, uint64_t id_) {
        if (midas::is_unlikely_hint(recvBufferSize > 0 && sz > recvBufferSize)) {
            MIDAS_LOG_WARNING("incoming size of " << sz << " too large, **DROPPED MESSAGE**");
            isValid = false;
            return false;
        }

        size = sz;
        rcvt = rcvt_;
        id = id_;

        if (!recvBufferSize) {
            data_ = const_cast<char*>(p);
        } else {
            // deep copy
            memcpy(data_, p, sz);
        }

        isValid = true;
        return true;
    }

    bool is_valid() { return isValid; }

    void set_is_valid(bool isValid_) { isValid = isValid_; }

    void reset() { hasMoreData = false; }

    /**
     * use in BatchConsumerStrategy so a payload can be dynamically grown to accommodate multiple other payloads
     * @param other
     */
    void append(const Payload* other) {
        /**
         * buffer size not enough, allocate new buffer, free old small buffer
         */
        if (midas::is_unlikely_hint(size + other->get_size() > recvBufferSize)) {
            bool deleteOld = false;
            if (recvBufferSize) {
                deleteOld = true;
            }
            recvBufferSize = (size + other->get_size()) * 1.5;
            char* newBuffer = new char[recvBufferSize];
            memcpy(newBuffer, data_, size);
            if (deleteOld) {
                delete[] data_;
            }

            data_ = newBuffer;
        }

        memcpy(data_ + size, other->buffer(), other->get_size());
        size += other->get_size();
    }

    const char* buffer() const { return data_; }
    uint32_t get_size() const { return size; }
    uint64_t get_rcvt() const { return rcvt; }
    int64_t get_id() const { return id; }

    void set_has_more_data(bool hasMoreData_) { hasMoreData = hasMoreData_; }

    bool has_more_data() { return hasMoreData; }

protected:
    uint64_t rcvt{0};
    char* data_;
    uint32_t recvBufferSize;
    uint32_t size{0};

private:
    long sequenceValue{-1};
    int64_t id{0};
    bool isValid{false};

    /**
     * indicate that there are more data to be consumed, allowing the consumer of the data
     * to make use of a more efficient mode of handling this payload
     */
    bool hasMoreData = false;
};

/**
 * const_buffer is used to pass data between the producers and consumers
 * storing a mutable buffer - and exposing a const char * to the user
 * i.e. the user will not be able to modify its data
 * suitable for non broadcast dispatch policy only.
 * Use broadcast payload if you need to redistribute to multiple consumers
 */
template <typename allocator_type = HeapAllocator>
class BufferPayload {
public:
    typedef std::false_type BroadcastSupported;  // Payload can be only be used in single dispatch mode

    BufferPayload(const uint32_t bufferSize = 512) : m_mutableBuffer(bufferSize, m_allocator) {}

    BufferPayload(const void* data, size_t size, uint64_t rcvt, int64_t id) : m_mutableBuffer(size, m_allocator) {
        set_value(data, size, rcvt, id);
    }

    BufferPayload(const BufferPayload& other)
        : id(other.id),
          rcvt(other.rcvt),
          m_mutableBuffer(other.m_mutableBuffer, 0),
          sequenceValue(other.sequenceValue),
          isValid(other.isValid) {}

    virtual ~BufferPayload() {}

    BufferPayload& operator=(const BufferPayload& other) {
        if (this == &other) return *this;

        sequenceValue = other.sequenceValue;
        rcvt = other.rcvt;
        id = other.id;
        isValid = other.isValid;
        m_mutableBuffer.clear();
        m_mutableBuffer.store(other.data(), other.size());

        return *this;
    }

    void set_sequence_value(const long value, long cursorWhenAdded) {
        sequenceValue = value;
        (void)cursorWhenAdded;
    }

    long get_sequence_value() const { return sequenceValue; }

    // this function is called to load this message into the disruptor ring
    bool set_value(BufferPayload& other) {
        if (midas::is_unlikely_hint(!other.isValid())) {
            MIDAS_LOG_WARNING("Incoming payload is not valid, **DROPPED MESSAGE**");
            isValid = false;
            return false;
        }

        m_mutableBuffer.swap(other.m_mutableBuffer);

        rcvt = other.rcvt;
        id = other.id;

        isValid = true;
        return true;
    }

    bool is_valid() const { return isValid; }

    void set_is_valid(bool isValid_) { isValid = isValid_; }

    void append(const BufferPayload* other) { m_mutableBuffer.store(other->data(), other->size()); }

    const char* data() const { return m_mutableBuffer.data(); }
    size_t get_size() const { return m_mutableBuffer.size(); }

    boost::asio::mutable_buffers_1 buffer() { return boost::asio::buffer(m_mutableBuffer.writeBuffer()); }

    void reset() {
        m_mutableBuffer.clear();
        isValid = false;
        hasMoreData = false;
    }

    // pass through to the mutable buffer store function
    void store(std::size_t s) {
        m_mutableBuffer.store(s);
        isValid = true;
    }

    void store(const void* data, std::size_t s) {
        m_mutableBuffer.store(data, s);
        isValid = true;
    }

    std::size_t capacity() const { return m_mutableBuffer.capacity(); }

    std::size_t writeCapacity() const { return m_mutableBuffer.writeCapacity(); }

    void swapBuffer(BufferPayload& other) { m_mutableBuffer.swap(other.m_mutableBuffer); }

    void swapBuffer(MutableBuffer& otherMutableBuffer) { m_mutableBuffer.swap(otherMutableBuffer); }

    int64_t get_id() const { return id; }

    uint64_t get_rcvt() const { return rcvt; }

    // helper function to set the buffer - deep copy from raw buffer
    void set_value(const void* buffer, size_t len, uint64_t rcvt_, int64_t id_) {
        if (!buffer) {
            isValid = false;
            return;
        }

        m_mutableBuffer.clear();
        store(buffer, len);

        rcvt = rcvt_;
        id = id_;

        isValid = true;
    }

    // helper function to set the buffer - deep copy from const buffer
    void set_value(ConstBuffer buffer, uint64_t rcvt, int64_t id) { set_value(buffer.data(), buffer.size(), rcvt, id); }

    void set_has_more_data(bool hasMoreData_) { hasMoreData = hasMoreData_; }

    bool has_more_data() { return hasMoreData; }

    int64_t id = 0;
    uint64_t rcvt = 0;

protected:
    MutableBuffer m_mutableBuffer;

private:
    long sequenceValue{-1};
    bool isValid{false};

    // this variable can be used to indicate that there are more data to be consumed, allowing the
    // consumer of the data to make use of a more efficient mode of handling this payload
    bool hasMoreData{false};

    allocator_type m_allocator;
};

/**
 * const_buffer is used to pass data between the producers and consumers
 * storing a mutable buffer - and exposing a const char * to the user
 * i.e. the user will not be able to modify its data
 * This payload is suitable broadcast dispatch policy
 */
template <typename allocator_type = HeapAllocator>
class BroadcastPayload {
public:
    typedef std::true_type BroadcastSupported;  // Payload can be used in broadcast or single dispatch mode

    BroadcastPayload(const uint32_t bufferSize = 512) : m_mutableBuffer(bufferSize, m_allocator) {}

    BroadcastPayload(const void* data, size_t size, uint64_t rcvt, int64_t id) : m_mutableBuffer(size, m_allocator) {
        set_value(data, size, rcvt, id);
    }

    BroadcastPayload(const BroadcastPayload& other)
        : id(other.id),
          rcvt(other.rcvt),
          m_mutableBuffer(other.m_mutableBuffer, 0),
          sequenceValue(other.sequenceValue),
          isValid(other.isValid) {}

    virtual ~BroadcastPayload() {}

    BroadcastPayload& operator=(const BroadcastPayload& other) {
        if (this == &other) return *this;

        sequenceValue = other.sequenceValue;
        rcvt = other.rcvt;
        id = other.id;
        isValid = other.isValid;
        reset();
        m_mutableBuffer.store(other.data(), other.size());

        return *this;
    }

    void set_sequence_value(const long value, long cursorWhenAdded) {
        sequenceValue = value;
        (void)cursorWhenAdded;
    }

    long get_sequence_value() const { return sequenceValue; }

    // this function is called to load this message into the disruptor ring
    bool set_value(BroadcastPayload& other) {
        if (midas::is_unlikely_hint(!other.isValid())) {
            MIDAS_LOG_WARNING("Incoming payload is not valid, **DROPPED MESSAGE**");
            isValid = false;
            return false;
        }

        m_mutableBuffer = other.m_mutableBuffer;  // copy ref
        rcvt = other.rcvt;
        id = other.id;

        isValid = true;
        return true;
    }

    bool is_valid() const { return isValid; }

    void set_is_valid(bool isValid_) { isValid = isValid_; }

    void append(const BroadcastPayload* other) { m_mutableBuffer.store(other->data(), other->size()); }

    const char* data() const { return m_mutableBuffer.data(); }
    size_t get_size() const { return m_mutableBuffer.size(); }

    boost::asio::mutable_buffers_1 buffer() { return boost::asio::buffer(m_mutableBuffer.writeBuffer()); }

    void reset() {
        m_mutableBuffer = midas::MutableBuffer(capacity() > 0 ? capacity() : 512, m_allocator);
        isValid = false;
        hasMoreData = false;
    }

    // pass through to the mutable buffer store function
    void store(std::size_t s) {
        m_mutableBuffer.store(s);
        isValid = true;
    }

    void store(const void* data, std::size_t s) {
        m_mutableBuffer.store(data, s);
        isValid = true;
    }

    std::size_t capacity() const { return m_mutableBuffer.capacity(); }

    std::size_t writeCapacity() const { return m_mutableBuffer.writeCapacity(); }

    void swapBuffer(BroadcastPayload& other) {
        // this buffer is ref counted - swap only when really necessary
        m_mutableBuffer.swap(other.m_mutableBuffer);
    }

    void swapBuffer(midas::MutableBuffer& otherMutableBuffer) {
        // this buffer is ref counted - swap only when really necessary
        m_mutableBuffer.swap(otherMutableBuffer);
    }

    int64_t get_id() const { return id; }

    uint64_t get_rcvt() const { return rcvt; }

    // helper function to set the buffer - deep copy from raw buffer
    void set_value(const void* buffer, size_t len, uint64_t rcvt_, int64_t id_) {
        if (!buffer) {
            isValid = false;
            return;
        }

        reset();
        store(buffer, len);

        rcvt = rcvt_;
        id = id_;

        isValid = true;
    }

    // helper function to set the buffer - deep copy from const buffer
    void set_value(midas::ConstBuffer buffer, uint64_t rcvt, int64_t id) {
        set_value(buffer.data(), buffer.size(), rcvt, id);
    }

    midas::MutableBuffer mutableBuffer() { return m_mutableBuffer; }

    void set_has_more_data(bool hasMoreData_) { hasMoreData = hasMoreData_; }

    bool has_more_data() { return hasMoreData; }

    int64_t id = 0;
    uint64_t rcvt = 0;

protected:
    midas::MutableBuffer m_mutableBuffer;

private:
    long sequenceValue{-1};
    bool isValid{false};

    // this variable can be used to indicate that there are more data to be consumed, allowing the
    // consumer of the data to make use of a more efficient mode of handling this payload
    bool hasMoreData{false};

    allocator_type m_allocator;
};

/**
 * suitable broadcast dispatch policy
 */
class ConstBroadcastPayload {
public:
    typedef std::true_type BroadcastSupported;      // Payload can be used in broadcast or single dispatch mode
    typedef std::false_type BatchConsumeSupported;  // Payload cannot be used in batch consume mode

    ConstBroadcastPayload(const uint32_t bufferSize = 512) {}

    ConstBroadcastPayload(const void* data, size_t size, uint64_t rcvt_, int64_t id_)
        : id(id_), rcvt(rcvt_), m_constBuffer(data, size), isValid(true) {}

    ConstBroadcastPayload(midas::ConstBuffer buffer, uint64_t rcvt_, int64_t id_)
        : id(id_), rcvt(rcvt_), m_constBuffer(buffer), isValid(true) {}

    ConstBroadcastPayload(midas::MutableBuffer buffer, uint64_t rcvt_, int64_t id_)
        : id(id_), rcvt(rcvt_), m_constBuffer(buffer), isValid(true) {}

    ConstBroadcastPayload(const ConstBroadcastPayload& other)
        : id(other.id),
          rcvt(other.rcvt),
          m_constBuffer(other.m_constBuffer),
          sequenceValue(other.sequenceValue),
          isValid(other.isValid) {}

    virtual ~ConstBroadcastPayload() {}

    ConstBroadcastPayload& operator=(const ConstBroadcastPayload& other) {
        if (this == &other) return *this;

        sequenceValue = other.sequenceValue;
        rcvt = other.rcvt;
        id = other.id;
        isValid = other.isValid;
        reset();
        m_constBuffer = other.buffer();

        return *this;
    }

    void set_sequence_value(const long value, long cursorWhenAdded) {
        sequenceValue = value;
        (void)cursorWhenAdded;
    }

    long get_sequence_value() const { return sequenceValue; }

    // this function is called to load this message into the disruptor ring
    bool set_value(ConstBroadcastPayload& other) {
        if (midas::is_unlikely_hint(!other.is_valid())) {
            MIDAS_LOG_WARNING("Incoming payload is not valid, **DROPPED MESSAGE**");
            isValid = false;
            return false;
        }

        m_constBuffer = other.m_constBuffer;  // copy ref
        rcvt = other.rcvt;
        id = other.id;

        isValid = true;
        return true;
    }

    bool is_valid() const { return isValid; }

    void set_is_valid(bool isValid_) { isValid = isValid_; }

    const char* data() const { return m_constBuffer.data(); }
    size_t get_size() const { return m_constBuffer.size(); }

    void reset() {
        isValid = false;
        hasMoreData = false;
    }

    void swapBuffer(ConstBroadcastPayload& other) { m_constBuffer.swap(other.m_constBuffer); }

    int64_t get_id() const { return id; }

    uint64_t get_rcvt() const { return rcvt; }

    midas::ConstBuffer buffer() { return m_constBuffer; }

    midas::ConstBuffer buffer() const { return m_constBuffer; }

    void set_has_more_data(bool hasMoreData_) { hasMoreData = hasMoreData_; }

    bool has_more_data() { return hasMoreData; }

    int64_t id = 0;
    uint64_t rcvt = 0;

protected:
    midas::ConstBuffer m_constBuffer;

private:
    long sequenceValue{-1};
    bool isValid{false};
    bool hasMoreData{false};
};
}

#endif
