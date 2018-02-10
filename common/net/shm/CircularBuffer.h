#ifndef MIDAS_CIRCULAR_BUFFER_H
#define MIDAS_CIRCULAR_BUFFER_H

#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include "HeapMemory.h"
#include "midas/MidasException.h"

namespace midas {

struct Buffer {
    const uint8_t *address;
    uint32_t length;
};

template <typename T>
struct NullSerializer {
    static std::pair<bool, uint32_t> deserialize(const uint8_t *buffer_, uint32_t length_, T &t_) {
        memcpy(&t_, buffer_, sizeof(T));
        return {true, sizeof(T)};
    }
    static std::pair<T, uint32_t> deserialize(const uint8_t *buffer_, uint32_t length_) {
        std::pair<T, uint32_t> ret{T(), sizeof(T)};
        memcpy(&ret.first, buffer_, sizeof(T));
        return ret;
    }

    Buffer serialize(const T &v_, uint8_t *address_ = nullptr) {
        return {reinterpret_cast<const uint8_t *>(&v_), sizeof(T)};
    }

    static uint32_t recordSize() { return sizeof(T); }
};

// Default Traits used by CircularBuffer when the Element type is flat in memory layout.
template <typename T, typename S = NullSerializer<T>>
struct FlatStructTraits {
    const static bool isFlat = true;
    using Serializer = S;
    using Type = T;
};

template <typename T, typename S>
struct NonFlatStructTraits {
    const static bool isFlat = false;
    using Serializer = S;
    using Type = T;  // Element Type given to CircularBuffer must be uint8_t
};

template <typename MemorySpace = HeapMemory, typename Element = uint8_t, typename Traits = FlatStructTraits<Element>>
class CircularBuffer {
private:
    static constexpr uint32_t CbMagic = 0x00108023;
    struct alignas(64) Meta {
        uint32_t magic{CbMagic};
        uint32_t metaSize{sizeof(Meta)};
        uint32_t capacity{0};  // in bytes
        uint32_t elementSize{sizeof(Element)};
        uint32_t dataOffset{0};
        uint32_t recordSize{0};               // 0 indicates variable length
        std::atomic<uint32_t> isInitialized;  // set the value to 1 after all other fields set.
        // writer acquires it before writing to the buffer, reader releases it after committing the read.
        alignas(64) std::atomic<uint32_t> readerPos;
        // reader acquires it before reading the buffer, writer releases it after committing the write.
        alignas(64) std::atomic<uint32_t> writerPos;
        /**
         * when writer starts over from the begin, wrap keeps where the writer stops.
         * writer only updates wrap when writerPos > readerPos
         * reader only reads wrap when readerPos > writerPos
         * there is no simultaneous access.
         */
        uint32_t wrap = 0;
    };

    MemorySpace space;
    Meta *meta;

public:
    using ElementType = Element;
    using Serializer = typename Traits::Serializer;

    CircularBuffer(MemorySpace &&space_, bool init_ = false) : space{std::move(space_)} {
        if (space.capacity() <= offset()) THROW_MIDAS_EXCEPTION("CircularBuffer: Insufficient space.");

        uint32_t cap = space.capacity() - offset();
        if (init_) {
            meta = new (space.buffer) Meta;
            meta->capacity = cap;
            meta->dataOffset = offset();
            meta->recordSize = Serializer::recordSize();
            meta->readerPos.store(0, std::memory_order_release);
            meta->writerPos.store(0, std::memory_order_release);
            meta->isInitialized.store(1, std::memory_order_release);
        } else {
            meta = reinterpret_cast<Meta *>(space.buffer);
            if (meta->isInitialized.load(std::memory_order_acquire) != 1)
                THROW_MIDAS_EXCEPTION("CircularBuffer: Peer initialization not finished.");
            if (meta->magic != CbMagic) THROW_MIDAS_EXCEPTION("CircularBuffer: Magic number mismatch.");
            if (meta->elementSize != sizeof(ElementType))
                THROW_MIDAS_EXCEPTION("CircularBuffer: sizeof ElementType mismatch.");
        }
    }

    /**
     * Request read from the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the reading position
     * @param size_ how many bytes to read
     * @return      number of available bytes for reading
     */
    uint32_t get_read_pointer(uint8_t *&buf_, uint32_t size_) {
        const uint32_t wPos = writer_pos(std::memory_order_acquire);
        const uint32_t rPos = reader_pos(std::memory_order_relaxed);
        uint8_t *src = get_payload();
        if (wPos == rPos) {
            return 0;
        } else if (wPos > rPos) {  // |   RXXXXXW    |
            if (size_ <= wPos - rPos) {
                buf_ = (src + rPos);
                return size_;
            } else {
                return 0;  // no space
            }
        } else {  // |XXXW     RXXXX|
            if (rPos == meta->wrap) {
                reader_pos(0, std::memory_order_relaxed);
                if (size_ <= wPos) {
                    buf_ = src;
                    return size_;
                } else {
                    return 0;  // no space
                }
            } else {
                if (size_ <= meta->wrap - rPos) {
                    buf_ = (src + rPos);
                    return size_;
                } else {
                    return 0;  // no space
                }
            }
        }
    }

    /**
     * Request maximum continuous available space for reading from the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the reading position
     * @return      number of available bytes for reading
     */
    uint32_t get_read_pointer(uint8_t *&buf_) {
        const uint32_t wPos = writer_pos(std::memory_order_acquire);
        const uint32_t rPos = reader_pos(std::memory_order_relaxed);
        uint8_t *src = get_payload();
        if (wPos == rPos) {
            return 0;
        } else if (wPos > rPos) {  // |   RXXXXXW    |
            buf_ = (src + rPos);
            return wPos - rPos;
        } else {  // |XXXW     RXXXX|
            if (rPos == meta->wrap) {
                reader_pos(0, std::memory_order_relaxed);
                buf_ = src;
                return wPos;
            } else {
                buf_ = (src + rPos);
                return meta->wrap - rPos;
            }
        }
    }

    /**
     * Commit read
     * @param size_ how many bytes to commit
     */
    void commit_read(uint32_t size_) { meta->readerPos.fetch_add(size_, std::memory_order_release); }

    /**
     * Request read from the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the reading position
     * @param size_ how many Element to read
     * @return      number of available Element for reading
     */
    uint32_t get_element_read_pointer(ElementType *&buf_, uint32_t size_) {
        return get_read_pointer(reinterpret_cast<uint8_t *&>(buf_), size_ * sizeof(ElementType)) / sizeof(ElementType);
    }
    /**
     * Commit read
     * @param size_ how many Element to commit
     */
    void commit_element_read(uint32_t size_) { commit_read(size_ * sizeof(ElementType)); }

    /**
     * Request write to the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the writing position
     * @param size_ how many bytes to read
     * @return      number of available bytes for writing
     */
    uint32_t get_write_pointer(uint8_t *&buf_, uint32_t size_) {
        const uint32_t wPos = writer_pos(std::memory_order_relaxed);
        const uint32_t rPos = reader_pos(std::memory_order_acquire);
        uint8_t *dst = get_payload();
        if (wPos < rPos) {  // |XXXW     RXXXX|
            if (wPos + size_ < rPos) {
                buf_ = (dst + wPos);
                return size_;
            } else {
                return 0;  // no space
            }
        } else {  // |   RXXXXXW    |
            if (size_ <= meta->capacity - wPos) {
                buf_ = (dst + wPos);
                return size_;
            } else if (size_ < rPos) {
                buf_ = dst;
                meta->wrap = wPos;
                writer_pos(0, std::memory_order_relaxed);
                return size_;
            } else {
                return 0;  // no space
            }
        }
    }

    /**
     * Request maximum continuous available space for writing to the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the writing position
     * @return      number of available bytes for writing
     */
    uint32_t get_write_pointer(uint8_t *&buf_) {
        const uint32_t wPos = writer_pos(std::memory_order_relaxed);
        const uint32_t rPos = reader_pos(std::memory_order_acquire);
        uint8_t *dst = get_payload();
        if (wPos < rPos) {  // |XXXW     RXXXX|
            buf_ = (dst + wPos);
            return rPos - wPos - 1;
        } else {  // |   RXXXXXW    |
            if (rPos == 0) {
                buf_ = (dst + wPos);
                return meta->capacity - wPos - 1;
            } else {
                if (wPos == meta->capacity) {
                    buf_ = dst;
                    meta->wrap = wPos;
                    writer_pos(0, std::memory_order_relaxed);
                    return rPos - 1;
                }
                buf_ = (dst + wPos);
                return meta->capacity - wPos;
            }
        }
    }

    /**
     * Commit write
     * @param size_ how many bytes to commit
     */
    void commit_write(uint32_t size_) { meta->writerPos.fetch_add(size_, std::memory_order_release); }

    /**
     * Request write to the CircularBuffer
     * @param buf_  address of the pointer, which will be set to point to the writing position
     * @param size_ how many Element to write
     * @return      number of available Element for writing
     */
    uint32_t get_element_write_pointer(ElementType *&buf_, uint32_t size_) {
        return get_write_pointer(reinterpret_cast<uint8_t *&>(buf_), size_ * sizeof(ElementType)) / sizeof(ElementType);
    }
    /**
     * Commit write (type-ed)
     * @param size_ how many Element to commit
     */
    void commit_element_write(uint32_t size_) { commit_write(size_ * sizeof(ElementType)); }

public:
    /**
     * Write Elem to the CircularBuffer, serialized data is copied into CircularBuffer.
     */
    template <typename Elem>
    bool write(const Elem &value_) {
        Serializer s;
        auto p = s.serialize(value_);
        uint8_t *dst = nullptr;
        if (get_write_pointer(dst, p.length) != p.length) {
            return false;
        } else {
            memcpy(dst, p.address, p.length);
            commit_write(p.length);
        }
        return true;
    }

    /**
     * Write Elem to the CircularBuffer, with length_ if we know the output size ahead.
     * Request space from CircularBuffer before serialize.
     * Serializer then could serialize in place, to avoid one copy.
     */
    template <typename Elem>
    bool write(const Elem &value_, uint32_t length_) {
        uint8_t *dst = nullptr;
        if (get_write_pointer(dst, length_) != length_) {
            return false;
        } else {
            Serializer s;
            auto p = s.serialize(value_, dst);
            commit_write(length_);
        }
        return true;
    }

    template <typename Elem>
    bool read(Elem &e_) {
        uint8_t *dst = nullptr;
        uint32_t length = get_read_pointer(dst);
        if (length == 0) {
            return false;
        }
        auto p = Serializer::deserialize(dst, length, e_);
        commit_read(p.second);
        return p.first;
    }

    /**
     * Read n Elements from the CircularBuffer.
     */
    template <typename Elem>
    uint32_t read(std::vector<Elem> &vec_, size_t n_) {
        uint8_t *dst = nullptr;
        uint32_t length = get_read_pointer(dst);
        if (length == 0) {
            return 0;
        }
        uint32_t pos = 0;
        uint32_t cnt = 0;
        while (pos < length && cnt < n_) {
            auto p = Serializer::deserialize(dst + pos, length - pos);
            if (p.second == 0) return cnt;
            commit_read(p.second);
            vec_.push_back(std::move(p.first));
            pos += p.second;
            cnt++;
        }
        return cnt;
    }

private:
    constexpr uint32_t offset() const { return std::max<uint32_t>(sizeof(Meta), sizeof(Element)); }
    uint8_t *get_payload() const { return reinterpret_cast<uint8_t *>(meta) + offset(); }
    uint32_t writer_pos(std::memory_order order_) const { return meta->writerPos.load(order_); }
    uint32_t reader_pos(std::memory_order order_) const { return meta->readerPos.load(order_); }
    void writer_pos(uint32_t v_, std::memory_order order_) { meta->writerPos.store(v_, order_); }
    void reader_pos(uint32_t v_, std::memory_order order_) { meta->readerPos.store(v_, order_); }
};
}

#endif
