#ifndef MIDAS_CHANNEL_H
#define MIDAS_CHANNEL_H

#include <tbb/atomic.h>
#include <tbb/concurrent_vector.h>
#include <tbb/tbb_allocator.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/thread.hpp>
#include <memory>
#include <ostream>
#include "MemberEntry.h"
#include "SlotEntry.h"
#include "midas/Lock.h"
#include "midas/MidasConstants.h"
#include "net/buffer/ConstBuffer.h"
#include "process/admin/MidasAdminBase.h"
#include "utils/ConvertHelper.h"
#include "utils/MidasUtils.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

/**
 * a channel is a group of Member objects having common io interest
 * Member can join/leave channel
 * MemberEntry is wrapper of MemberPtr to make thread safe access
 * SlotEntry is wrapper of list members which joined the same slot
 */

template <typename DeliverType>
struct MemberEntry;

template <typename DeliverType>
struct SlotEntry;

template <typename DeliverType>
class Channel {
public:
    typedef boost::function<void(const DeliverType&, Channel&, ssize_t slot)> DeliverFunction;
    typedef MemberEntry<DeliverType> TMemberEntry;
    typedef SlotEntry<DeliverType> TSlotEntry;
    typedef tbb::concurrent_vector<TMemberEntry, tbb::zero_allocator<TMemberEntry>> TMembers;
    typedef tbb::concurrent_vector<TSlotEntry, tbb::zero_allocator<TSlotEntry>> TSlots;
    typedef boost::function<bool(MemberPtr, Channel&)> TJoinCallback;
    typedef boost::function<void(MemberPtr, Channel&)> TLeaveCallback;

    string label;       // optional thread label
    string configPath;  // config path used to init member
    uint64_t userData{0};
    string affinity;
    int workerLimit{0};
    volatile bool enabled;
    tbb::atomic<long> memberCount;
    TSlots slots;
    tbb::atomic<long> slotCount;
    size_t broadcastSlotCount;
    boost::asio::io_service iosvc;
    TMembers members;
    boost::asio::strand mainStrand;
    tbb::atomic<int> workerCount;                       // active thread count
    shared_ptr<boost::asio::io_service::work> work2do;  // inform io_service when it has work to do

public:
    Channel(const string& label_, const string& cfg) : Channel(label_, cfg, std::thread::hardware_concurrency()) {}

    Channel(const string& label_)
        : Channel(label_, constants::DefaultChannelCfgPath, std::thread::hardware_concurrency()) {}

    Channel(const string& label_, const string& cfg, int threadMax)
        : label(label_),
          configPath(cfg),
          affinity(Config::instance().get<string>(cfg + ".cpu_affinity")),
          workerLimit(Config::instance().get<int>(cfg + ".thread_count", threadMax)),
          enabled(Config::instance().get<bool>(cfg + ".enabled", true)),
          mainStrand(iosvc) {
        memberCount = 0;
        slotCount = 0;
        workerCount = 0;
        broadcastSlotCount = get_slot();
    }

    /**
     * start output channel's iosvc on the same thread, it will block if any pending work
     * @param enableThrow
     * @param workerNumber
     */
    void run(bool enableThrow = true, int workers = -1) {
        string localLabel{label};
        MIDAS_LOG_INFO("thread " << localLabel << " running... ");
        if (!affinity.empty()) {
            MIDAS_LOG_INFO("set cpu affinity for channel " << localLabel << " to " << affinity);
            set_cpu_affinity(get_tid(), affinity);
        }
        if (workers == -1) workers = ++workerCount;
        if (workers == 1) iosvc.reset();
        for (;;) {
            try {
                iosvc.run();

                // naturally run out of task, then break
                break;
            } catch (const StopRunTag&) {
                break;
            } catch (const std::exception& e) {
                try {
                    MIDAS_LOG_ERROR("exception caught in thread " << localLabel << " : " << e.what());
                    if (enableThrow) Log::instance().get_writer()->flush_log();
                } catch (...) {
                }

                if (enableThrow) throw;
            } catch (...) {
                try {
                    MIDAS_LOG_ERROR("unknown exception caught in thread "
                                    << localLabel << " : " << boost::current_exception_diagnostic_information());
                    if (enableThrow) Log::instance().get_writer()->flush_log();
                } catch (...) {
                }

                if (enableThrow) throw;
            }
        }
        --workerCount;
        MIDAS_LOG_INFO("thread " << localLabel << " terminate...");
    }

    // start new thread to run iosvc, this won't block
    bool start(int limit = 0, bool enableThrow = true) {
        if (!limit) limit = workerLimit;
        int workers = workerCount;
        while (workers < limit && workerCount.compare_and_swap(workers + 1, workers) != workers) {
            workers = workerCount;
        }

        if (workers < limit) {
            boost::thread worker(boost::bind(&Channel<DeliverType>::run, this, enableThrow, workers + 1));
            return true;
        }
        return false;
    }

    int start_n(int n, int limit = 0, bool enableThrow = true) {
        int startCount = 0;
        for (int i = 0; i < n; ++i) {
            if (start(limit, enableThrow))
                ++startCount;
            else
                break;
        }
        return startCount;
    }

    // create work to not stop when run out of work
    bool work(int limit = 0, bool enableThrow = true) {
        if (!work2do) work2do.reset(new boost::asio::io_service::work(iosvc));
        return start(limit, enableThrow);
    }

    int work_n(int n, int limit = 0, bool enableThrow = true) {
        if (!work2do) work2do.reset(new boost::asio::io_service::work(iosvc));
        return start_n(n, limit, enableThrow);
    }

    void stop() {
        work2do.reset();
        iosvc.stop();
        while (workerCount)
            ;  // spin wait all threads exit
        members.clear();
        slots.clear();
        memberCount = 0;
        slotCount = 0;
    }

    void enable_all() {
        for (auto m : members) {
            MemberPtr memberPtr = const_cast<TMemberEntry&>(m).get();
            if (memberPtr) memberPtr->enable();
        }
        enabled = true;
    }

    void enable(const string& m) {
        MemberPtr memberPtr = get_member(m);
        if (memberPtr) {
            memberPtr->enable();
            return true;
        }
        return false;
    }

    void disable_all() {
        for (auto m : members) {
            MemberPtr memberPtr = const_cast<TMemberEntry&>(m).get();
            if (memberPtr) memberPtr->disable();
        }
        enabled = false;
    }

    void disable(const string& m) {
        MemberPtr memberPtr = get_member(m);
        if (memberPtr) {
            memberPtr->disable();
            return true;
        }
        return false;
    }

    /**
     * set member's deliver function for deliverFunction,
     * once msg come, call this to deliver msg to member
     */
    template <typename MemberType>
    void join(typename MemberType::SharedPtr member) {
        auto itr = members.begin();
        for (; itr != members.end(); ++itr) {
            if (itr->template set_member<MemberType>(member)) break;
        }

        if (itr == members.end()) {  // add new member
            itr = members.push_back(TMemberEntry());
            while (!itr->template set_member<MemberType>(member)) {
                itr = members.push_back(TMemberEntry());
            }
        }
        ssize_t idx = itr - members.begin();
        member->id = idx;

        bool doDisable = false;
        if (!enabled && member->enabled) {
            member->disable();
            doDisable = true;
        }

        bool doJoin = true;
        if (joinCallback<MemberType>()) doJoin = joinCallback<MemberType>()(member, *this);
        if (joinCallback<DefaultCallbackTag>()) doJoin = joinCallback<DefaultCallbackTag>()(member, *this);
        if (!doJoin) {  // undo join
            members[idx].template clear<MemberType>(member);
            member->id = -1;
            if (doDisable) member->enable();
        } else {
            ++memberCount;
        }
    }

    template <typename MemberType>
    bool leave(typename MemberType::SharedPtr member) {
        if (member->id == -1 || static_cast<size_t>(member->id) >= members.size()) return false;
        members[member->id].template clear<MemberType>(member);
        leave_all_slots<MemberType>(member);

        if (leaveCallback<DefaultCallbackTag>()) leaveCallback<DefaultCallbackTag>()(member, *this);
        if (leaveCallback<MemberType>()) leaveCallback<MemberType>()(member, *this);

        --memberCount;
        return true;
    }

    void deliver(const DeliverType& msg) {
        for (auto& m : members) {
            // call MemberEntry void deliver(const DeliverType& msg, TChannel& channel, ssize_t slot)
            // slot id is -1 means broadcast to all member, usually acceptor is not a broadcast candidate
            m.deliver(msg, *this, -1);
        }
    }

    void channel_callback(const DeliverType& msg) { deliver(msg); }

    template <typename MemberType>
    void deliver(const DeliverType& msg, typename MemberType::SharedPtr member) {
        member->deliver(msg);
    }

    void reserve_slots(size_t n) { slots.grow_to_at_least(n); }

    // allocate next available slot and return its id
    size_t get_slot() {
        auto itr = slots.begin();
        if (slotCount < (long)slots.size()) itr += slotCount;
        for (; itr != slots.end(); ++itr) {
            if (itr->reserve()) break;
        }
        if (itr == slots.end()) {
            itr = slots.push_back(TSlotEntry());
            while (!itr->reserve()) itr = slots.push_back(TSlotEntry());
        }
        size_t num = itr - slots.begin();
        ++slotCount;
        return num;
    }

    void clear_slot(size_t idx) {
        if (idx < slots.size()) {
            for_all_members(boost::bind(&Member::remove_slot, _1, idx));
            slots[idx].clear();
            --slotCount;
        }
    }

    template <typename MemberType>
    void join_slot(typename MemberType::SharedPtr member, size_t slotIdx) {
        // call ssize_t add(typename MemberType::SharedPtr m)
        ssize_t idx = slots[slotIdx].template add<MemberType>(member);
        if (idx != -1) {
            if (!member->add_slot(slotIdx, idx)) {
                // failed to add, probably already joined
                slots[slotIdx].template clear<MemberType>((size_t)idx, member);
            }
        } else {
            MIDAS_LOG_ERROR("failed to join slot: " << slotIdx);
        }
    }

    template <typename MemberType>
    bool leave_slot(typename MemberType::SharedPtr member, size_t slot) {
        ssize_t slotIdx = member->remove_slot(slot);
        if (slotIdx != -1) {
            slots[slot].template clear<MemberType>((size_t)slotIdx, member);
            return true;
        }
        return false;
    }

    template <typename MemberType>
    void leave_all_slots(typename MemberType::SharedPtr member) {
        pair<size_t, ssize_t> slot;
        while (member->remove_next_slot(slot)) {
            slots[slot.first].template clear<MemberType>(slot.second, member);
        }
    }

    void deliver_slot(const DeliverType& msg, size_t slot) { slots[slot].deliver(msg, *this, slot); }

    MemberPtr get_member(const string& name) const {
        for (auto& m : members) {
            MemberPtr member = const_cast<TMemberEntry&>(m).get();
            if (member && (member->get_name() == name || member->get_description() == name)) return member;
        }
        return MemberPtr();
    }

    MemberPtr get_member(ssize_t id) const {
        if (id == -1 || (size_t)id >= members.size()) return MemberPtr();
        return const_cast<TMemberEntry&>(members[(size_t)id]).get();
    }

    template <typename Functor>
    void for_all_members(const Functor& f) const {
        for (auto& m : members) {
            MemberPtr member = const_cast<TMemberEntry&>(m).get();
            if (member) f(member);
        }
    }

    template <typename Functor>
    void for_all_slot_members(size_t slot, const Functor& f) const {
        const_cast<TSlotEntry&>(slots[slot]).for_all_members(f);
    }

    void enable(ostream& os, const TAdminCallbackArgs& args) {
        string memberStr = (args.empty() ? string() : args[0]);
        if (memberStr.empty() || memberStr == label) {
            os << "enable all members on channel: " << label << '\n';
            enable();
        } else if (!memberStr.empty() && enable(memberStr)) {
            os << "member : " << memberStr << " enabled." << '\n';
        }
    }

    void disable(ostream& os, const TAdminCallbackArgs& args) {
        string memberStr = (args.empty() ? string() : args[0]);
        if (memberStr.empty() || memberStr == label) {
            os << "disable all members on channel: " << label << '\n';
            disable();
        } else if (!memberStr.empty() && disable(memberStr)) {
            os << "member : " << memberStr << " disable." << '\n';
        }
    }

    // print stats for given member
    void meters(ostream& os, const TAdminCallbackArgs& args, bool isFirst = false) const {
        fixed_meters(os, args, 0, isFirst);
    }

    void meters(ostream& os, bool isFirst = false) const { fixed_meters(os, 0, isFirst); }

    void fixed_meters(ostream& os, const TAdminCallbackArgs& args, size_t width, bool isFirst = false) const {
        string memberStr = (args.empty() ? string() : args[0]);
        if (memberStr.empty())
            fixed_meters(os, width, isFirst);
        else {
            if (memberStr == label)
                stats(os);
            else {
                MemberPtr m = get_member(memberStr);
                if (m) m->stats(os);
            }
        }
    }

    void fixed_meters(ostream& os, size_t width, bool isFirst = false) const {
        static const size_t maxNameWidth = 80, maxDescWidth = 80;
        size_t nameWidth = (width ? width : 20), descWidth = 20;

        std::map<string, MemberPtr> name2member;
        for (auto& m : members) {
            MemberPtr member = const_cast<TMemberEntry&>(m).get();
            if (!member || member->get_name().empty() || !member->displayStat) continue;
            if (!member->get_description().empty())
                name2member[member->get_description()] = member;
            else
                name2member[member->get_name()] = member;

            if (!width) {
                if (member->get_name().length() > nameWidth) nameWidth = member->get_name().length();
            }
            if (member->get_description().length() > descWidth) descWidth = member->get_description().length();
        }

        if (!width) nameWidth += 5;
        descWidth += 5;

        nameWidth = std::min(nameWidth, maxNameWidth);
        descWidth = std::min(descWidth, maxDescWidth);

        if (isFirst) {
            os << std::left << std::setw(nameWidth) << "Connection" << setw(10) << "Protocol" << setw(10) << "Channel"
               << setw(20) << "Connect Time" << setw(15) << std::right << "Msgs Sent" << setw(15) << "Bytes Sent"
               << setw(15) << "Msgs Recv" << setw(15) << "Bytes Recv"
               << " " << std::left << "Desc" << '\n';

            char delimiter[256];
            memset(delimiter, 0, sizeof(delimiter));
            memset(delimiter, '_', nameWidth + descWidth + 96);
            os << delimiter << '\n';
        }

        for (auto itr = name2member.begin(); itr != name2member.end(); ++itr) {
            MemberPtr member = itr->second;
            string timeLocal{"N/A"};
            if (member->enabled && member->state == NetState::Connected) {
                timeLocal = time_t2string(member->connectTime);
            } else {
                timeLocal = member->get_state_as_string();
            }

            os << std::left << std::setw(nameWidth) << member->get_name().substr(0, nameWidth - 1) << setw(10)
               << member->get_protocol_as_string() << setw(10) << label << setw(20) << timeLocal << setw(15)
               << std::right << member->msgsSent << setw(15) << member->bytesSent << setw(15) << member->msgsRecv
               << setw(15) << member->bytesRecv << " " << std::left
               << member->get_description().substr(0, descWidth - 1) << '\n';
        }
    }

    void stats(ostream& os) const {
        os << "channel          = " << label << '\n'
           << "config           = " << configPath << '\n'
           << "enabled          = " << get_bool_string(enabled) << '\n'
           << "worker limit     = " << workerLimit << '\n'
           << "worker number    = " << workerCount << '\n'
           << "affinity         = " << affinity << '\n'
           << "member number    = " << memberCount << '\n'
           << "slot number      = " << slotCount << '\n'
           << "exit when done   = " << get_bool_string(!work2do) << '\n';
    }

    bool stats(ostream& os, const string& member) const {
        MemberPtr m = get_member(member);
        if (m) {
            m->stats(os);
            return true;
        }
        return false;
    }

    static void register_join_callback(const TJoinCallback& callback) { joinCallback<DefaultCallbackTag>() = callback; }

    template <typename Tag>
    static void register_join_callback(const TJoinCallback& callback) {
        joinCallback<Tag>() = callback;
    }

    static void register_leave_callback(const TLeaveCallback& callback) {
        leaveCallback<DefaultCallbackTag>() = callback;
    }

    template <typename Tag>
    static void register_leave_callback(const TLeaveCallback& callback) {
        leaveCallback<Tag>() = callback;
    }

private:
    template <typename Tag>
    static TJoinCallback& joinCallback() {
        static TJoinCallback callback;
        return callback;
    }

    template <typename Tag>
    static TLeaveCallback& leaveCallback() {
        static TLeaveCallback callback;
        return callback;
    }
};

typedef Channel<ConstBuffer> CChannel;
}

#endif
