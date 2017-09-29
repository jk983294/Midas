#include <net/common/TimerMember.h>
#include <net/tcp/TcpReceiver.h>
#include <utils/log/Log.h>

using namespace midas;

struct MyReceiverTag {};
typedef TcpReceiver<MyReceiverTag> TReceiver;
typedef TimerMember<MyReceiverTag> TTimer;

void timer_expire(TReceiver::SharedPtr connection, TTimer::SharedPtr timer, boost::posix_time::time_duration d) {
    uint64_t now = ntime();
    connection->deliver(ConstBuffer(&now, sizeof(now)));
    timer->start(d, boost::bind(timer_expire, connection, _1, d));
}

bool connect_callback(TReceiver::SharedPtr connection, TTimer::SharedPtr timer, boost::posix_time::time_duration d) {
    timer->start(d, boost::bind(timer_expire, connection, _1, d));
    return true;
}

bool disconnect_callback(TTimer::SharedPtr timer) {
    timer->stop();
    return true;
}

size_t data_callback(const ConstBuffer& data) {
    if (data.size() < sizeof(uint64_t)) return 0;
    const uint64_t& t = *reinterpret_cast<const uint64_t*>(data.data());
    MIDAS_LOG_INFO("time: " << ntime() - t << " ns");
    return sizeof(uint64_t);
}

int main() {
    string host{"localhost"}, port{"8023"};
    CChannel channel("echo_client");
    TTimer::SharedPtr timer = TTimer::new_instance(channel, "echo");

    TReceiver::connect_callback(boost::bind(connect_callback, _1, timer, boost::posix_time::seconds(1)));
    TReceiver::disconnect_callback(boost::bind(disconnect_callback, timer));
    TReceiver::data_callback(boost::bind(data_callback, _1));

    TReceiver::new_instance(host, port, channel);
    channel.run();
    return 0;
}
