#ifndef MIDAS_CONST_BUFFER_LIST_H
#define MIDAS_CONST_BUFFER_LIST_H

#include <iterator>
#include <memory>
#include "ConstBuffer.h"

using namespace std;

namespace midas {

class ConstBufferList {
public:
    BufferData* first{nullptr};
    BufferData** next;
    volatile size_t size{0};
    volatile size_t byteSize{0};

public:
    ConstBufferList() : next(&first) {}

    bool empty() const { return !first; }

    void push_back(const ConstBuffer& buffer) {
        BufferData* data = buffer_cast_helper(buffer).get();
        assert(data != nullptr);
        data->add_ref();
        data->prefix().next = nullptr;
        *next = data;
        next = &data->prefix().next;
        byteSize += data->prefix().size;
        ++size;
    }

    ConstBuffer pop_front() {
        assert(!empty());
        BufferData* data = first;
        first = data->prefix().next;
        if (!first) next = &first;
        byteSize -= data->prefix().size;
        --size;
        ConstBuffer result(data);
        data->remove_ref();
        return result;
    }

    void clear() {
        BufferData* i = first;
        while (i) {
            BufferData* next_ = i->prefix().next;
            i->remove_ref();
            i = next_;
        }
        first = nullptr;
        next = &first;
        size = byteSize = 0;
    }

    void merge() {
        static const size_t MergeBufferSize = 51200;
        BufferData* mergeBuffer = new (pool_allocator<MergeBufferSize, ConstBufferList>(MergeBufferSize)) BufferData;
        BufferData* f = mergeBuffer;
        size_t offset = 0;
        BufferData* i = first;
        size = 0;
        while (i) {
            if (i->prefix().size + offset > MergeBufferSize) {
                // close current buffer
                mergeBuffer->prefix().size = offset;
                mergeBuffer->add_ref();
                ++size;

                // open new buffer
                mergeBuffer->prefix().next =
                    new (pool_allocator<MergeBufferSize, ConstBufferList>(MergeBufferSize)) BufferData;
                mergeBuffer = mergeBuffer->prefix().next;
                offset = 0;
            }
            memcpy(mergeBuffer->data + offset, i->data, i->prefix().size);
            offset += i->prefix().size;

            BufferData* n = i->prefix().next;
            i->remove_ref();
            i = n;
        }

        // close buffer
        mergeBuffer->prefix().size = offset;
        mergeBuffer->add_ref();
        ++size;

        // add merged buffer as first
        first = f;
        next = &mergeBuffer->prefix().next;
    }

    void swap(ConstBufferList& right) {
        std::swap(first, right.first);
        std::swap(next, right.next);
        if (!first) {
            next = &first;
        }
        if (!right.first) {
            right.next = &right.first;
        }
        std::swap(byteSize, right.byteSize);
        std::swap(size, right.size);
    }

    class const_iterator : public std::iterator<std::forward_iterator_tag, ConstBuffer, int, ConstBuffer, ConstBuffer> {
    public:
        BufferData* buffer;

    public:
        const_iterator(BufferData* p = nullptr) : buffer(p) {}
        bool operator==(const const_iterator& rhs) const { return buffer == rhs.buffer; }
        bool operator!=(const const_iterator& rhs) const { return buffer != rhs.buffer; }
        reference operator*() const { return value_type(buffer); }
        const_iterator& operator++() {
            buffer = buffer->prefix().next;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            buffer = buffer->prefix().next;
            return tmp;
        }
    };

    typedef const_iterator::value_type value_type;

    const_iterator begin() const { return const_iterator(first); }
    const_iterator end() const { return const_iterator(); }
};
}

#endif
