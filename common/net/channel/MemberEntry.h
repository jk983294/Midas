#ifndef MIDAS_MEMBER_ENTRY_H
#define MIDAS_MEMBER_ENTRY_H

#include "net/common/Member.h"
#include "utils/MidasUtils.h"

namespace midas {

// wrapper of MemberPtr to make thread safe
template <typename DeliverType>
struct MemberEntry {
    typedef Channel<DeliverType> TChannel;
    typedef boost::function<void(const DeliverType&, TChannel&, ssize_t slot)> DeliverFunction;

    tbb::atomic<long> references;  // -1 free, 0 initializing, >1 valid
    MemberPtr member;
    DeliverFunction deliverFunction;

    MemberEntry() { references = -1; }

    template <typename MemberType>
    bool set_member(typename MemberType::SharedPtr value) {
        if (references != -1) return false;  // this entry already occupied by other member
        // get ownership of entry
        if (references.fetch_and_increment() == -1) {  // references 0
            member = value;
            deliverFunction = getDeliverFunction<MemberType>(&MemberType::deliver, value);
            ++references;  // mark available by references 1
            return true;
        }
        --references;
        return false;
    }

    MemberPtr get() {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        MemberPtr safeMember;
        if (refCount > 0) {
            safeMember = member;
            clear();
        }
        return safeMember;
    }

    void deliver(const DeliverType& msg, TChannel& channel, ssize_t slot) {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        if (refCount > 0) {
            if (is_likely_hint(member->enabled)) {
                deliverFunction(msg, channel, slot);
            }
            clear();
        }
    }

    void clear() {
        if (references.fetch_and_decrement() == 1) {  // references 0
            member.reset();
            --references;  // mark free by references -1
        }
    }

    // clear only when member's value matches
    template <typename MemberType>
    void clear(typename MemberType::SharedPtr value) {
        if (references.fetch_and_decrement() == 1) {  // references 0
            if (member == value) {
                member.reset();
                deliverFunction = DeliverFunction();
                --references;  // mark free by references -1
            } else {
                ++references;
            }
        }
    }

    /**
     *  several overload for Member's deliver function
     *  possible format:
     *  void deliver(const ConstBuffer& m)
     *  void deliver(const ConstBuffer& m, TChannel&)
     *  void deliver(const ConstBuffer& m, ssize_t)
     *  void deliver(const ConstBuffer& m, TChannel&, ssize_t)
     */
    template <typename MemberType>
    DeliverFunction getDeliverFunction(void (MemberType::*)(const DeliverType&), typename MemberType::SharedPtr m) {
        return boost::bind(&MemberType::deliver, m, _1);
    }

    template <typename MemberType>
    DeliverFunction getDeliverFunction(void (MemberType::*)(const DeliverType&, TChannel&),
                                       typename MemberType::SharedPtr m) {
        return boost::bind(&MemberType::deliver, m, _1, _2);
    }

    template <typename MemberType>
    DeliverFunction getDeliverFunction(void (MemberType::*)(const DeliverType&, ssize_t),
                                       typename MemberType::SharedPtr m) {
        return boost::bind(&MemberType::deliver, m, _1, _3);
    }

    template <typename MemberType>
    DeliverFunction getDeliverFunction(void (MemberType::*)(const DeliverType&, TChannel&, ssize_t),
                                       typename MemberType::SharedPtr m) {
        return boost::bind(&MemberType::deliver, m, _1, _2, _3);
    }
};
}

#endif
