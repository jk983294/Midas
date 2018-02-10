#ifndef MIDAS_HEAP_MEMORY_H
#define MIDAS_HEAP_MEMORY_H

#include <cstdint>

namespace midas {

class HeapMemory {
public:
    uint8_t *buffer{nullptr};
    uint32_t size{0};
    /**
     * true means object will manage the life time of the allocated space
     * false means donot manage life time
     */
    bool isAdopt{false};

public:
    HeapMemory(uint32_t size_) : size{size_}, isAdopt{false} { buffer = new uint8_t[size_]; }

    HeapMemory(uint8_t *buffer_, uint32_t size_) : buffer{buffer_}, size{size_}, isAdopt{true} {}

    ~HeapMemory() {
        if (!isAdopt && buffer) delete[] buffer;
    }

    HeapMemory(HeapMemory &&other_) : buffer{other_.buffer}, size{other_.size}, isAdopt{other_.isAdopt} {
        other_.buffer = nullptr;
    }

    HeapMemory(const HeapMemory &) = delete;
    HeapMemory &operator=(const HeapMemory &) = delete;

    HeapMemory &operator=(HeapMemory &&rhs_) {
        buffer = rhs_.buffer;
        size = rhs_.size;
        isAdopt = rhs_.isAdopt;
        rhs_.buffer = nullptr;
        return *this;
    }

    uint32_t capacity() const { return size; }
};
}

#endif
