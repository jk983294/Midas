#include <string>
#include "catch.hpp"
#include "net/channel/Channel.h"
#include "net/common/TimerMember.h"

using namespace midas;

static bool testMemberCreated = false;
static bool joinCallbackCalled = true;
static bool leaveCallbackCalled = true;

template <typename Derived>
class TestMemberBase : public Member, public std::enable_shared_from_this<Derived> {
public:
    typedef boost::intrusive_ptr<Derived> SharedPtr;
    CChannel& channel;
    ConstBuffer msg;

public:
    TestMemberBase(CChannel& channel_, const string& memberName_) : channel(channel_) {
        testMemberCreated = true;
        set_name(memberName_);
    }

    virtual ~TestMemberBase() { testMemberCreated = false; }
    static SharedPtr new_instance(CChannel& c, const string& n) { return SharedPtr(new Derived(c, n)); }
    void start() { channel.join<Derived>(static_cast<Derived*>(this)); }
    void stop() { channel.leave<Derived>(static_cast<Derived*>(this)); }
    void clear() { msg = ConstBuffer(); }
};

class TestMember1 : public TestMemberBase<TestMember1> {
public:
    TestMember1(CChannel& channel_, const string& memberName_) : TestMemberBase<TestMember1>(channel_, memberName_) {}
    void deliver(const ConstBuffer& m) { msg = m; }
    friend class TestMemberBase<TestMember1>;
};

bool join_callback_true(MemberPtr m, CChannel&) {
    joinCallbackCalled = true;
    return true;
}
bool join_callback_false(MemberPtr m, CChannel&) {
    joinCallbackCalled = true;
    return false;
}

void leave_callback(MemberPtr m, CChannel&) { leaveCallbackCalled = true; }

TEST_CASE("Channel join leave", "[Channel]") {
    CChannel channel("test channel");
    REQUIRE(channel.label == "test channel");

    // register callback
    CChannel::register_join_callback<TestMember1>(join_callback_true);
    CChannel::register_leave_callback<TestMember1>(leave_callback);

    {
        TestMember1::SharedPtr m1(TestMember1::new_instance(channel, "member1"));
        REQUIRE(testMemberCreated);
        m1->start();
        REQUIRE(joinCallbackCalled);
        REQUIRE(channel.get_member("member1"));

        m1->stop();
        REQUIRE(leaveCallbackCalled);
        REQUIRE(!(bool)channel.get_member("member1"));

        joinCallbackCalled = false;
        leaveCallbackCalled = false;
    }

    REQUIRE(!testMemberCreated);
    CChannel::register_join_callback<TestMember1>(join_callback_false);
    {
        TestMember1::SharedPtr m1(TestMember1::new_instance(channel, "member2"));
        REQUIRE(testMemberCreated);
        m1->start();
        REQUIRE(joinCallbackCalled);
        REQUIRE(!leaveCallbackCalled);
        REQUIRE(!(bool)channel.get_member("member2"));

        joinCallbackCalled = false;
        leaveCallbackCalled = false;
    }
    REQUIRE(!testMemberCreated);

    // register back
    CChannel::register_join_callback<TestMember1>(CChannel::TJoinCallback());
    CChannel::register_leave_callback<TestMember1>(CChannel::TLeaveCallback());
}

TEST_CASE("deliver", "[Channel]") {
    CChannel channel("test channel");
    typename TestMember1::SharedPtr member(TestMember1::new_instance(channel, "member"));
    static const char testMsg[] = "hello test";
    channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
    REQUIRE(member->msg.size() == 0);  // have not enable

    member->start();
    channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
    REQUIRE(member->msg.size() == sizeof(testMsg));  //  enable
    REQUIRE(memcmp(member->msg.data(), testMsg, sizeof(testMsg)) == 0);

    member->clear();
    REQUIRE(member->msg.size() == 0);

    channel.disable_all();
    channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
    REQUIRE(member->msg.size() == 0);

    channel.enable_all();
    channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
    REQUIRE(member->msg.size() == sizeof(testMsg));
}

TEST_CASE("slot", "[Channel]") {
    CChannel channel("test channel");

    // create slot
    size_t slotAll = channel.get_slot();
    size_t slot1 = channel.get_slot();
    size_t slot2 = channel.get_slot();

    // create slot
    typename TestMember1::SharedPtr member1(TestMember1::new_instance(channel, "member1"));
    typename TestMember1::SharedPtr member2(TestMember1::new_instance(channel, "member2"));

    // spread members between slot
    channel.join_slot<TestMember1>(member1, slot1);
    channel.join_slot<TestMember1>(member1, slotAll);
    channel.join_slot<TestMember1>(member2, slot2);
    channel.join_slot<TestMember1>(member2, slotAll);

    static const char testMsg[] = "hello test";

    {
        channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
        // should not arrive
        REQUIRE(member1->msg.size() == 0);
        REQUIRE(member2->msg.size() == 0);
    }

    for (int i = 0; i < 2; ++i) {
        // i == 0 test for without members join channel (slot only)
        if (i == 1) {
            member1->start();
            member2->start();
        }

        // send to slot1
        {
            channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slot1);
            REQUIRE(member1->msg.size() == sizeof(testMsg));
            REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
            REQUIRE(member2->msg.size() == 0);
            member1->clear();
            member2->clear();
        }

        // send to slot2
        {
            channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slot2);
            REQUIRE(member2->msg.size() == sizeof(testMsg));
            REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
            REQUIRE(member1->msg.size() == 0);
            member1->clear();
            member2->clear();
        }

        // send to all
        {
            channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
            REQUIRE(member1->msg.size() == sizeof(testMsg));
            REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
            REQUIRE(member2->msg.size() == sizeof(testMsg));
            REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
            member1->clear();
            member2->clear();
        }
    }

    // now all member join channel, msg should arrive
    {
        channel.deliver(ConstBuffer(testMsg, sizeof(testMsg)));
        REQUIRE(member1->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
        REQUIRE(member2->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();
        member2->clear();
    }

    // explicit leave slot
    {
        typename TestMember1::SharedPtr member3(TestMember1::new_instance(channel, "member3"));
        member3->start();
        channel.join_slot<TestMember1>(member3, slotAll);
        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
        REQUIRE(member3->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member3->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member3->clear();

        channel.leave_slot<TestMember1>(member3, slotAll);
        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
        REQUIRE(member3->msg.size() == 0);
        REQUIRE(member1->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
        REQUIRE(member2->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();
        member2->clear();
    }

    // implicit leave slot by leave channel
    {
        typename TestMember1::SharedPtr member3(TestMember1::new_instance(channel, "member3"));
        member3->start();
        channel.join_slot<TestMember1>(member3, slotAll);
        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
        REQUIRE(member3->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member3->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member3->clear();

        member3->stop();
        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
        REQUIRE(member3->msg.size() == 0);
        REQUIRE(member1->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
        REQUIRE(member2->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();
        member2->clear();
    }

    // leave multiple slot
    {
        typename TestMember1::SharedPtr member3(TestMember1::new_instance(channel, "member3"));
        member3->start();
        channel.join_slot<TestMember1>(member3, slot1);
        channel.join_slot<TestMember1>(member3, slot2);
        channel.join_slot<TestMember1>(member3, slotAll);

        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slot1);
        REQUIRE(member3->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member3->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();
        member2->clear();
        member3->clear();

        channel.leave_all_slots<TestMember1>(member3);

        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slot1);
        REQUIRE(member3->msg.size() == 0);
        REQUIRE(member1->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();

        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slot2);
        REQUIRE(member3->msg.size() == 0);
        REQUIRE(member2->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member2->clear();

        channel.deliver_slot(ConstBuffer(testMsg, sizeof(testMsg)), slotAll);
        REQUIRE(member3->msg.size() == 0);
        REQUIRE(member1->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member1->msg.data(), testMsg, sizeof(testMsg)) == 0);
        REQUIRE(member2->msg.size() == sizeof(testMsg));
        REQUIRE(memcmp(member2->msg.data(), testMsg, sizeof(testMsg)) == 0);
        member1->clear();
        member2->clear();
    }
}

/*
// comment timer test for speed
struct t1{};
struct t2{};
bool expire1Called = false;
void expire_callback1(MemberPtr m){ expire1Called = true; }
bool expire2Called = false;
void expire_callback2(MemberPtr m){ expire2Called = true; }
int expireSequence = 0;
void expire_callback_seq(TimerMember<t1>::SharedPtr tm){
    if(expireSequence < 3){
        ++expireSequence;
        tm->start(1); // recur every 1 second
    }
}

TEST_CASE("TimerMember", "[Channel]") {
    CChannel channel("test channel");
    TimerMember<t1>::SharedPtr timer1(TimerMember<t1>::new_instance(channel, "timer1"));
    TimerMember<t2>::SharedPtr timer2(TimerMember<t2>::new_instance(channel, "timer2"));

    REQUIRE((bool) channel.get_member("timer1"));
    REQUIRE((bool) channel.get_member("timer2"));

    TimerMember<t1>::expireCallback(expire_callback1);
    TimerMember<t2>::expireCallback(expire_callback2);

    channel.work();

    REQUIRE(!expire1Called);
    REQUIRE(!expire2Called);

    timer1->start(1);
    timer2->start(1);

    sleep(2);
    REQUIRE(expire1Called);
    REQUIRE(expire2Called);

    channel.stop();
}

TEST_CASE("TimerMember seq", "[Channel]") {
    CChannel channel("test channel");
    TimerMember<t1>::SharedPtr timer1(TimerMember<t1>::new_instance(channel, "timer1"));
    TimerMember<t1>::expireCallback(expire_callback_seq);
    channel.work();
    timer1->start(1);
    sleep(1);
    REQUIRE(expireSequence < 3);
    sleep(3);
    REQUIRE(expireSequence == 3);
    channel.stop();
}
*/
