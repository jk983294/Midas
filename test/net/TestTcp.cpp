#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include "catch.hpp"
#include "net/channel/Channel.h"
#include "net/tcp/TcpAcceptor.h"
#include "net/tcp/TcpPublisherAsync.h"
#include "net/tcp/TcpPublisherDirect.h"
#include "net/tcp/TcpReceiver.h"
#include "net/tcp/TcpRecvGroup.h"

using namespace midas;

struct TestIn {};
struct TestOut {};
struct SlowOut {};

static const char testPort1[] = "8023";
static const char testPort2[] = "8024";
static const char testPort3[] = "8025";

static const size_t slowOutThreshold = 1024;

typedef TcpPublisherAsync<TestOut> TAsyncPublisher;
typedef TcpPublisherDirect<TestOut> TDirectPublisher;
typedef TcpPublisherAsync<SlowOut, true, slowOutThreshold> TSlowPublisher;
typedef TcpReceiver<TestIn, 1024, HeapAllocator> TReceiver;
typedef TcpAcceptor<TAsyncPublisher> TAsyncAcceptor;
typedef TcpAcceptor<TDirectPublisher> TDirectAcceptor;
typedef TcpAcceptor<TSlowPublisher> TSlowAcceptor;

static ConstBuffer msgBuffer;
static size_t data_callback(const ConstBuffer& data, MemberPtr m) {
    msgBuffer = data;
    return data.size();
}

static bool connectCallbackCalled = false;
static bool on_connect_callback(MemberPtr member) {
    connectCallbackCalled = true;
    return true;
}

static bool disconnectCallbackCalled = false;
static bool on_disconnect_callback(MemberPtr member) {
    disconnectCallbackCalled = true;
    return true;
}

static bool retryCallbackCalled = false;
static bool retryCallbackFail = false;
static bool on_retry_callback(MemberPtr member) {
    retryCallbackCalled = true;
    return !retryCallbackFail;
}

static TDirectPublisher::SharedPtr directPublisherPtr;
static bool on_direct_connect_callback(TDirectPublisher::SharedPtr member) {
    directPublisherPtr = member;
    return true;
}

static bool on_direct_disconnect_callback(TDirectPublisher::SharedPtr member) {
    directPublisherPtr.reset();
    return true;
}

static size_t slow_callback(const ConstBuffer& data, MemberPtr member) {
    sleep(1);
    return data.size();
}

static TSlowPublisher::SharedPtr slowPublisherPtr;
static bool on_slow_connect_callback(TSlowPublisher::SharedPtr member) {
    slowPublisherPtr = member;
    return true;
}

static bool on_slow_disconnect_callback(MemberPtr member) {
    slowPublisherPtr.reset();
    return true;
}

TEST_CASE("TcpDirect", "[TcpDirect]") {
    CChannel outChannel("out_channel");
    CChannel inChannel("in_channel");
    msgBuffer = ConstBuffer();
    connectCallbackCalled = false;
    disconnectCallbackCalled = false;
    retryCallbackCalled = false;

    directPublisherPtr.reset();

    // register callback
    TReceiver::data_callback(data_callback);
    TReceiver::connect_callback(on_connect_callback);
    TReceiver::disconnect_callback(on_disconnect_callback);
    TReceiver::retry_callback(on_retry_callback);
    TDirectPublisher::connect_callback(on_direct_connect_callback);
    TDirectPublisher::disconnect_callback(on_direct_disconnect_callback);

    outChannel.work();
    inChannel.work();

    TDirectAcceptor::new_instance(testPort1, outChannel);
    TReceiver::SharedPtr receiver(TReceiver::new_instance("localhost", testPort1, inChannel));

    sleep(1);
    REQUIRE(connectCallbackCalled);
    REQUIRE((bool)directPublisherPtr);

    static const char TestMsg[] = "hello world";
    outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
    sleep(1);

    REQUIRE(msgBuffer.size() == sizeof(TestMsg));
    REQUIRE(memcmp(msgBuffer.data(), TestMsg, sizeof(TestMsg)) == 0);

    connectCallbackCalled = false;
    // kill output
    directPublisherPtr->skt.close();
    directPublisherPtr.reset();

    sleep(1);
    REQUIRE(disconnectCallbackCalled);

    sleep(2);
    REQUIRE(retryCallbackCalled);
    REQUIRE(connectCallbackCalled);
    REQUIRE((bool)directPublisherPtr);

    connectCallbackCalled = false;
    disconnectCallbackCalled = false;
    retryCallbackCalled = false;
    retryCallbackFail = true;

    // kill output
    directPublisherPtr->skt.close();
    directPublisherPtr.reset();

    sleep(1);
    REQUIRE(disconnectCallbackCalled);
    sleep(2);
    REQUIRE(retryCallbackCalled);
    REQUIRE(!connectCallbackCalled);

    retryCallbackFail = false;
    directPublisherPtr.reset();

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}

TEST_CASE("TcpAsync", "[TcpAsync]") {
    CChannel outChannel("out_channel1");
    CChannel inChannel("in_channel1");
    msgBuffer = ConstBuffer();
    connectCallbackCalled = false;
    disconnectCallbackCalled = false;

    // register callback
    TReceiver::data_callback(data_callback);
    TReceiver::connect_callback(on_connect_callback);
    TReceiver::disconnect_callback(on_disconnect_callback);

    outChannel.work();
    inChannel.work();

    TAsyncAcceptor::new_instance(testPort2, outChannel);
    TReceiver::SharedPtr receiver(TReceiver::new_instance("localhost", testPort2, inChannel));
    sleep(1);

    static const char TestMsg[] = "hello world";
    outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
    sleep(1);

    REQUIRE(msgBuffer.size() == sizeof(TestMsg));
    REQUIRE(memcmp(msgBuffer.data(), TestMsg, sizeof(TestMsg)) == 0);

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}

TEST_CASE("TcpSlow", "[TcpSlow]") {
    CChannel outChannel("out_channel");
    CChannel inChannel("in_channel");
    msgBuffer = ConstBuffer();
    connectCallbackCalled = false;
    disconnectCallbackCalled = false;
    slowPublisherPtr.reset();

    // register callback
    TReceiver::data_callback(slow_callback);
    TReceiver::connect_callback(on_connect_callback);
    TReceiver::disconnect_callback(on_disconnect_callback);
    TSlowPublisher::connect_callback(on_slow_connect_callback);
    TSlowPublisher::disconnect_callback(on_slow_disconnect_callback);

    outChannel.work();
    inChannel.work();

    TSlowAcceptor::new_instance(testPort3, outChannel);
    TReceiver::SharedPtr receiver(TReceiver::new_instance("localhost", testPort3, inChannel));

    sleep(1);
    REQUIRE(connectCallbackCalled);
    REQUIRE((bool)slowPublisherPtr);

    static const char TestMsg[] = "hello world";
    boost::asio::socket_base::send_buffer_size sendBufferSize;
    slowPublisherPtr->skt.get_option(sendBufferSize);
    boost::asio::socket_base::receive_buffer_size receiveBufferSize;
    slowPublisherPtr->skt.get_option(receiveBufferSize);
    size_t i = 0;
    size_t threshold = 2 * slowOutThreshold + sendBufferSize.value() + receiveBufferSize.value();

    while (i < threshold) {
        outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
        i += sizeof(TestMsg);
    }
    sleep(1);
    // check if sender side disconnect as receiver might busy
    REQUIRE(!(bool)slowPublisherPtr);

    slowPublisherPtr.reset();

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}
/*
namespace {
    boost::interprocess::interprocess_semaphore serverAcceptSema(0);
    boost::interprocess::interprocess_semaphore serverConnectedSema(0);
    boost::interprocess::interprocess_semaphore serverDisconnectedSema(0);
    boost::interprocess::interprocess_semaphore cliConnectedSema(0);
    boost::interprocess::interprocess_semaphore cliDisconnectedSema(0);

    const string testPorts[] = {
            "8026", "8027", "8028", "8029", "8030"
    };
    const string testMsgs[] = {
            "test msg 1", "test msg 2", "test msg 3", "test msg 4", "test msg 5"
    };

    ConstBuffer data;

    bool server_accept(TcpRecvGroup::TcpRecv& t){
        serverAcceptSema.post();
        return true;
    }

    bool server_connect(TcpRecvGroup::TcpRecv& t){
        serverConnectedSema.post();
        return true;
    }

    bool server_disconnect(TcpRecvGroup::TcpRecv& t){
        serverDisconnectedSema.post();
        return true;
    }

    size_t server_data_callback(const ConstBuffer& cb, TcpRecvGroup::TcpRecv& t){
        data = cb;
        return data.size();
    }

    bool client_connect(TcpRecvGroup::TcpRecv& t);
    bool client_disconnect(TcpRecvGroup::TcpRecv& t);
    size_t client_data_callback(const ConstBuffer& cb, TcpRecvGroup::TcpRecv& t);
    bool client_retry(TcpRecvGroup::TcpRecv& t){
        return true;
    }

    bool client_connect(TcpRecvGroup::TcpRecv& t){
        t.disconnect_callback(boost::bind(&client_disconnect, _1), CustomCallbackTag());
        t.data_callback(boost::bind(&client_data_callback, _1, _2), CustomCallbackTag());
        cliConnectedSema.post();
        return true;
    }

    bool client_disconnect(TcpRecvGroup::TcpRecv& t){
        cliDisconnectedSema.post();
        return false;
    }

    size_t client_data_callback(const ConstBuffer& cb, TcpRecvGroup::TcpRecv& t){
        return cb.size();
    }


    TEST_CASE("TcpRecvGroup", "[TcpRecvGroup]") {
        data = ConstBuffer();

        // register callback
        TcpRecvGroup::accept_callback(boost::bind(&server_accept, _1));
        TcpRecvGroup::TcpRecv::connect_callback(boost::bind(&server_connect, _1));
        TcpRecvGroup::TcpRecv::disconnect_callback(boost::bind(&server_disconnect, _1));
        TcpRecvGroup::TcpRecv::data_callback(boost::bind(&server_data_callback, _1, _2));
        CChannel inChannel("in_channel");
        inChannel.work();

        TcpRecvGroup::SharedPtr inTcpRecvGroup = TcpRecvGroup::new_instance(string(), inChannel);
        inTcpRecvGroup->start(testPorts[4]);

        CChannel outChannel("out_channel");
        outChannel.work();

        TcpRecvGroup::SharedPtr outTcpRecvGroup = TcpRecvGroup::new_instance(string(), outChannel);
        outTcpRecvGroup->start();

        vector<TcpAddress> addrVec;
        for (int j = 0; j < 4; ++j) {
            addrVec.push_back(TcpAddress("localhost", testPorts[j]));
        }

        TcpRecvGroup::TcpRecv* recv1 = outTcpRecvGroup->new_tcp_recv(addrVec,
                                                                     boost::bind(&client_connect, _1),
                                                                     boost::bind(&client_retry, _1));
        REQUIRE(recv1 != NULL);
        serverAcceptSema.wait();
        serverConnectedSema.wait();
        cliConnectedSema.wait();

        for (int j = 0; j < 4; ++j) {
            outChannel.deliver(ConstBuffer(testMsgs[j]));
            sleep(1);
            REQUIRE(data.size() == testMsgs[j].size());
            REQUIRE(memcmp(data.data(), testMsgs[j].data(), testMsgs[j].size()) == 0);
        }

        recv1->disconnect();
        cliDisconnectedSema.wait();
        serverDisconnectedSema.wait();

        inChannel.stop();
        outChannel.stop();
        sleep(1);
    }
}
*/
