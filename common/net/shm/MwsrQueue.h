#ifndef MIDAS_MPSC_QUEUE_H
#define MIDAS_MPSC_QUEUE_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace midas {
/**
 * multiple writer single reader
 */
template <typename T, typename Alloc = std::allocator<T>>
class MwsrQueue {
    struct Node {
        std::atomic<Node *> next{nullptr};
        T value;

        Node() = default;
        explicit Node(const T &value_) : value{value_} {}
        template <typename... Args>
        Node(Args &&... args_) : value{std::forward<Args>(args_)...} {}
    };

    Node stub __attribute__((aligned(64)));            // created at construction time
    std::atomic<Node *> tail;                          // modified by push - multiple threads
    volatile Node *head __attribute__((aligned(64)));  // modified by pop - single thread
    using RealAlloc = typename Alloc::template rebind<Node>::other;
    RealAlloc nodeAlloc;

public:
    using value_type = T;
    MwsrQueue() : head{&stub} { tail.store(&stub); }

    ~MwsrQueue() {
        T elem;
        while (get(elem))
            ;
    }

    template <typename... Args>
    void put(Args &&... args_) {
        auto deleter = [this](Node *np_) { std::allocator_traits<RealAlloc>::deallocate(nodeAlloc, np_, 1); };
        std::unique_ptr<Node, std::function<void(Node *)>> up{std::allocator_traits<RealAlloc>::allocate(nodeAlloc, 1),
                                                              deleter};
        std::allocator_traits<RealAlloc>::construct(nodeAlloc, up.get(), std::forward<Args>(args_)...);
        Node *pNode = up.release();
        Node *prev = tail.exchange(pNode, std::memory_order_acq_rel);
        prev->next.store(pNode, std::memory_order_release);
    }

    // get operates in chunk of elements and re-inserts stub after each chunk
    bool get(T &elem_) {
        if (head == &stub) {                  // current chunk empty
            if (tail == &stub) return false;  // queue is empty
            // wait for producer in put()
            while (stub.next.load(std::memory_order_relaxed) == nullptr) {
                asm volatile("pause" ::: "memory");
            }
            head = stub.next;  // remove stub
            stub.next.store(nullptr, std::memory_order_relaxed);
            Node *prev = tail.exchange(&stub, std::memory_order_acquire);
            prev->next.store(&stub, std::memory_order_relaxed);
        }
        // wait for producer in put()
        while (head->next.load(std::memory_order_relaxed) == nullptr) {
            asm volatile("pause" ::: "memory");
        }
        // retrieve and return first element
        Node *pNode = const_cast<Node *>(head);
        head = head->next;
        elem_ = std::move(pNode->value);
        std::allocator_traits<RealAlloc>::destroy(nodeAlloc, pNode);
        std::allocator_traits<RealAlloc>::deallocate(nodeAlloc, pNode, 1);
        return true;
    }
};
}

#endif
