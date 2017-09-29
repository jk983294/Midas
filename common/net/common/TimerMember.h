#ifndef MIDAS_TIMER_MEMBER_H
#define MIDAS_TIMER_MEMBER_H

#include <boost/asio/deadline_timer.hpp>
#include <string>
#include "net/channel/Channel.h"

using namespace std;

namespace midas {

// boost::asio::timer wrapper, join channel as member
template <typename Tag = DefaultTimerTag, bool UseStrand = true>
class TimerMember : public Member {
public:
    typedef typename boost::intrusive_ptr<TimerMember> SharedPtr;
    typedef boost::function<void(SharedPtr)> TExpireCallback;

public:
    CChannel& channel;
    boost::asio::deadline_timer timer;

public:
    static SharedPtr new_instance(CChannel& tc, const string& name) {
        SharedPtr newTimer(new TimerMember(tc, name));
        tc.join<TimerMember>(newTimer);
        return newTimer;
    }
    ~TimerMember() {}
    void start(int expireSec, const TExpireCallback callback = expireCallback()) {
        start(boost::posix_time::seconds(expireSec), callback);
    }

    void start(const boost::posix_time::time_duration& expires, const TExpireCallback callback = expireCallback()) {
        timer.expires_from_now(expires);
        set_state(Connected, ptime2time_t(timer.expires_at()));

        // use byte recv as seq number
        add_recv_msg_2_stat(1);

        if (UseStrand)
            timer.async_wait(
                channel.mainStrand.wrap(boost::bind(&TimerMember::time_notify, SharedPtr(this), _1, callback)));
        else
            timer.async_wait(boost::bind(&TimerMember::time_notify, SharedPtr(this), _1, callback));
    }

    void stop() {
        timer.cancel();
        set_state(Closed);
        channel.leave<TimerMember>(this);
    }

    // deliver nothing
    void deliver(const ConstBuffer&) {}

    // register callback
    static void expireCallback(const TExpireCallback& callback) { expireCallback() = callback; }

private:
    TimerMember(CChannel& tc, const string& name) : channel(tc), timer(tc.iosvc) {
        set_name(name);
        netProtocol = p_timer;
        set_state(Pending);
    }

    static TExpireCallback& expireCallback() {
        static TExpireCallback callback_;
        return callback_;
    }

    // timer expire notify callback
    void time_notify(const boost::system::error_code& e, TExpireCallback callback) {
        if (!e) {
            set_state(Disconnected);
            if (callback) {
                add_sent_msg_2_stat(1);
                callback(SharedPtr(this));
            } else {
                set_state(Closed);
            }
        }
    }
};
}

#endif
