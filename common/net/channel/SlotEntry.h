#ifndef MIDAS_SLOTENTRY_H
#define MIDAS_SLOTENTRY_H

namespace midas {

// wrapper of list members which joined the same slot
template <typename DeliverType>
struct SlotEntry {
    typedef MemberEntry<DeliverType> TMemberEntry;
    typedef Channel<DeliverType> TChannel;
    typedef tbb::concurrent_vector<TMemberEntry, tbb::zero_allocator<TMemberEntry>> TMembers;

    tbb::atomic<long> references;  // -1 free, 0 initializing, >1 valid
    TMembers members;

    SlotEntry() { references = -1; }

    bool reserve() {
        if (references != -1) return false;  // not free
        if (references.fetch_and_increment() == -1) {
            ++references;
            return true;
        }
        --references;
        return false;
    }

    template <typename MemberType>
    ssize_t add(typename MemberType::SharedPtr m) {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        ssize_t idx = -1;
        if (refCount > 0) {
            auto itr = members.begin();
            for (; itr != members.end(); ++itr) {
                if (itr->template set_member<MemberType>(m)) break;
            }

            if (itr == members.end()) {  // add new member
                itr = members.push_back(TMemberEntry());
                while (!itr->template set_member<MemberType>(m)) {
                    itr = members.push_back(TMemberEntry());
                }
            }
            idx = itr - members.begin();
            clear();
        }
        return idx;
    }

    void deliver(const DeliverType& msg, TChannel& channel, ssize_t slot) {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        if (refCount > 0) {
            for (auto itr = members.begin(); itr != members.end(); ++itr) {
                itr->deliver(msg, channel, slot);
            }
            clear();
        }
    }

    template <typename Functor>
    void for_all_members(const Functor& f) {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        if (refCount > 0) {
            for (auto itr = members.begin(); itr != members.end(); ++itr) {
                MemberPtr m = const_cast<TMemberEntry&>((*itr)).get();
                if (m) f(m);
            }
            clear();
        }
    }

    void clear() {
        if (references.fetch_and_decrement() == 1) {  // references 0
            for (auto itr = members.begin(); itr != members.end(); ++itr) {
                itr->clear();
            }
            --references;  // mark free by references -1
        }
    }

    template <typename MemberType>
    void clear(size_t idx, typename MemberType::SharedPtr value) {
        long refCount = references;
        // spin to set references++
        while (refCount > 0 && references.compare_and_swap(refCount + 1, refCount) != refCount) {
            refCount = references;
        }

        if (refCount > 0) {
            if (idx < members.size()) members[idx].template clear<MemberType>(value);
            clear();
        }
    }
};
}

#endif
