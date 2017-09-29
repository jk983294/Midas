#include <cstring>
#include "catch.hpp"
#include "net/channel/Channel.h"
#include "net/udp/UdpPublisherAsync.h"
#include "net/udp/UdpPublisherDirect.h"
#include "net/udp/UdpReceiver.h"

using namespace midas;

struct UdpTestIn {};
struct UdpTestOut {};

static const char testMulticastIp[] = "239.255.80.23";
static const char testPort1[] = "18023";
static const char testPort2[] = "18024";
static const string cfg{"test_udp"};

typedef UdpPublisherAsync<UdpTestOut> TAsyncPublisher;
typedef UdpPublisherDirect<UdpTestOut> TDirectPublisher;
typedef UdpReceiver<UdpTestIn, 1024, HeapAllocator> TReceiver;

static ConstBuffer testMsg;
static size_t data_callback(const ConstBuffer& data, MemberPtr m) {
    testMsg = data;
    return data.size();
}

static ConstBuffer requestMsg;
static void* requestHandle = nullptr;
static size_t request_callback(const ConstBuffer& data, void* handle, MemberPtr member) {
    requestMsg = data;
    requestHandle = handle;
    return data.size();
}

void init() {
    Config::instance().set(cfg + ".tll", 0);
    Config::instance().set(cfg + ".enable_loopback", true);
    Config::instance().set(cfg + ".bind_address", "lo");
    testMsg = ConstBuffer();
}

TEST_CASE("UdpDirectMulticast", "[UdpDirectMulticast]") {
    CChannel outChannel("out_channel");
    CChannel inChannel("in_channel");
    init();

    // register callback
    TReceiver::data_callback(data_callback);

    outChannel.work();
    inChannel.work();

    TDirectPublisher::new_instance(testMulticastIp, testPort1, outChannel, cfg);
    TReceiver::SharedPtr receiver(TReceiver::new_instance(testMulticastIp, testPort1, "", cfg, inChannel));

    sleep(1);

    static const char TestMsg[] = "hello world UdpDirectMulticast";
    outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
    sleep(1);

    REQUIRE(testMsg.size() == sizeof(TestMsg));
    REQUIRE(memcmp(testMsg.data(), TestMsg, sizeof(TestMsg)) == 0);

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}

TEST_CASE("UdpAsyncMulticast", "[UdpAsyncMulticast]") {
    CChannel outChannel("out_channel");
    CChannel inChannel("in_channel");
    init();

    // register callback
    TReceiver::data_callback(data_callback);

    outChannel.work();
    inChannel.work();

    TAsyncPublisher::new_instance(testMulticastIp, testPort1, outChannel, cfg);
    TReceiver::SharedPtr receiver(TReceiver::new_instance(testMulticastIp, testPort1, "", cfg, inChannel));

    sleep(1);

    static const char TestMsg[] = "hello world UdpAsyncMulticast";
    outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
    sleep(1);

    REQUIRE(testMsg.size() == sizeof(TestMsg));
    REQUIRE(memcmp(testMsg.data(), TestMsg, sizeof(TestMsg)) == 0);

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}

/*
TEST_CASE("UdpDirectUnicast", "[UdpDirectUnicast]") {
    CChannel outChannel("out_channel");
    CChannel inChannel("in_channel");

    init();

    // register callback
    TReceiver::data_callback(data_callback);
    TDirectPublisher::request_callback(request_callback);

    outChannel.work();
    inChannel.work();

    TDirectPublisher::SharedPtr publisher = TDirectPublisher::new_instance("", testPort1, outChannel, cfg);
    TReceiver::SharedPtr receiver(TReceiver::new_instance("localhost", testPort1, "", cfg, inChannel));

    sleep(1);

    static const char TestMsg[] = "hello world UdpDirectUnicast";
    outChannel.deliver(ConstBuffer(TestMsg, sizeof(TestMsg)));
    sleep(1);

    REQUIRE(testMsg.size() == sizeof(TestMsg));
    REQUIRE(memcmp(testMsg.data(), TestMsg, sizeof(TestMsg)) == 0);

    REQUIRE(requestHandle);
    if(requestHandle){
        static const char ResponseMsg[] = "hello world response";
        publisher->respond(requestHandle, ConstBuffer(ResponseMsg, sizeof(ResponseMsg)));

        sleep(1);

        REQUIRE(testMsg.size() == sizeof(ResponseMsg));
        REQUIRE(memcmp(testMsg.data(), ResponseMsg, sizeof(ResponseMsg)) == 0);
    }

    outChannel.stop();
    inChannel.stop();
    sleep(1);
}
*/
