#ifndef MIDAS_MEMBER_H
#define MIDAS_MEMBER_H

#include <tbb/atomic.h>
#include <tbb/concurrent_unordered_map.h>
#include <boost/intrusive_ptr.hpp>
#include "NetData.h"
#include "net/buffer/ConstBuffer.h"
#include "utils/ConvertHelper.h"

namespace midas {

template <typename DeliverType>
class Channel;

class Member {
public:
    typedef boost::intrusive_ptr<Member> SharedPtr;
    typedef tbb::concurrent_unordered_map<size_t, ssize_t> SlotMap;  // slot -> slotIndex
    static const size_t MaxNameNumber = 8;

    IoStatData statsCache;
    tbb::atomic<bool> statsCacheUpdating;
    tbb::atomic<bool> statsCacheReading;
    volatile uint64_t bytesSent{0}, bytesRecv{0}, msgsSent{0}, msgsRecv{0};
    volatile NetProtocol netProtocol{NetProtocol::p_any};
    volatile time_t connectTime{0};

    string name[MaxNameNumber];
    string description[MaxNameNumber];
    size_t nameIndex{0}, descIndex{0};

    string configPath;
    NetState state{NetState::Created};
    volatile bool enabled{true};
    ssize_t id{-1};
    SlotMap slotMap;

    bool displayStat{true};  // indicate if member will be part of meters
    uint64_t userData{0};

public:
    Member() {
        references = 0;
        statsCacheUpdating = false;
        statsCacheReading = false;
    }
    virtual ~Member() {}

    const string& get_name() const { return name[nameIndex]; }
    void set_name(const string& value, size_t idx = 0) { name[idx] = value; }
    void use_name(size_t idx) { nameIndex = idx; }
    const string& get_description() const { return description[descIndex]; }
    void set_description(const string& value, size_t idx = 0) { description[idx] = value; }
    void use_description(size_t idx) { descIndex = idx; }
    void set_user_data(uint64_t value) { userData = value; }
    const char* get_state_as_string() {
        static const char stateStrings[][64] = {"Created", "Pending", "Connected", "Disconnected", "Closed"};
        if (!enabled) return "disabled";
        if ((size_t)state >= sizeof((stateStrings)) / sizeof((stateStrings)[0]))
            return "N/A";
        else
            return stateStrings[state];
    }
    const char* get_protocol_as_string() {
        static const char protocolStrings[][64] = {"N/A", "tcp", "tcp_secondary", "udp", "timer", "file", "local"};
        if ((size_t)netProtocol >= sizeof((protocolStrings)) / sizeof((protocolStrings)[0]))
            return "N/A";
        else
            return protocolStrings[netProtocol];
    }
    void set_state(const NetState& st, time_t ct = 0) {
        state = st;
        connectTime = (ct != 0) ? ct : ((state == NetState::Connected) ? time(NULL) : 0);
    }
    // record how many bytes and msgs been processed
    void add_sent_msg_2_stat(const uint64_t& bytes) {
        bytesSent += bytes;
        ++msgsSent;
    }
    void reset_sent_stats() {
        bytesSent = 0;
        msgsSent = 0;
    }
    // record how many bytes and msgs been processed
    void add_recv_msg_2_stat(const uint64_t& bytes) {
        bytesRecv += bytes;
        ++msgsRecv;
    }
    void reset_recv_stats() {
        bytesRecv = 0;
        msgsRecv = 0;
    }

    virtual void enable() { enabled = true; }
    virtual void disable() { enabled = false; }
    virtual void stats(ostream& os) {
        os << "name             = " << get_name() << '\n'
           << "state            = " << get_state_as_string() << '\n'
           << "description      = " << get_description() << '\n'
           << "protocol         = " << get_protocol_as_string() << '\n'
           << "enabled          = " << get_bool_string(enabled) << '\n'
           << "config           = " << configPath << '\n'
           << "connection time  = " << time_t2string(connectTime) << '\n'
           << "joined slot#     = " << slotMap.size() << '\n'
           << "bytes recv       = " << bytesRecv << '\n'
           << "msgs recv        = " << msgsRecv << '\n'
           << "bytes sent       = " << bytesSent << '\n'
           << "msgs recv        = " << msgsSent << '\n';
    }

    // return true if receiver was shutdown
    bool is_closed() const { return state == NetState::Closed; }

    // let member join the same slots as caller
    template <typename ChannelType, typename MemberType>
    void share_slots(ChannelType& channel, typename MemberType::SharedPtr member) {
        for (auto itr1 = slotMap.begin(), itr2 = slotMap.end(); itr1 != itr2; ++itr1) {
            if (itr1->second != -1) {
                channel.template join_slot<MemberType>(member, itr1->first);
            }
        }
    }

    // static cast to derived member pointer
    template <typename MemberType>
    static typename MemberType::SharedPtr static_pointer_cast(SharedPtr member) {
        return boost::static_pointer_cast<MemberType>(member);
    }

    // dynamic cast to derived member pointer
    template <typename MemberType>
    static typename MemberType::SharedPtr dynamic_pointer_cast(SharedPtr member) {
        return boost::dynamic_pointer_cast<MemberType>(member);
    }

private:
    tbb::atomic<long> references;

    long add_ref() { return ++references; }
    long remove_ref() { return --references; }

    bool add_slot(size_t slot, ssize_t slotIndex) {
        std::pair<SlotMap::iterator, bool> ret = slotMap.insert(make_pair(slot, slotIndex));
        if (ret.second)
            return true;
        else if (ret.first->second == -1) {
            ret.first->second = slotIndex;
            return true;
        }
        return false;
    }
    ssize_t remove_slot(size_t slot) {
        ssize_t slotIndex = -1;
        auto itr = slotMap.find(slot);
        if (itr != slotMap.end()) {
            slotIndex = itr->second;
            itr->second = -1;
        }
        return slotIndex;
    }
    bool remove_next_slot(std::pair<size_t, ssize_t>& value) {
        for (auto itr1 = slotMap.begin(), itr2 = slotMap.end(); itr1 != itr2; ++itr1) {
            if (itr1->second != -1) {
                value = *itr1;
                itr1->second = -1;
                return true;
            }
        }
        return false;
    }

    friend void intrusive_ptr_add_ref(Member* p);
    friend void intrusive_ptr_release(Member* p);
    template <typename DT>
    friend class Channel;
};

typedef Member::SharedPtr MemberPtr;

inline void intrusive_ptr_add_ref(Member* p) { p->add_ref(); }
inline void intrusive_ptr_release(Member* p) {
    if (p->remove_ref() == 0) {
        delete p;
    }
}
}

#endif
