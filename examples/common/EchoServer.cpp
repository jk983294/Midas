#include <net/tcp/TcpAcceptor.h>
#include <net/tcp/TcpPublisherDirect.h>
#include <net/tcp/TcpReceiver.h>
#include <utils/log/Log.h>

using namespace midas;

struct MyPublisherTag {};
typedef TcpPublisherDirect<MyPublisherTag> TPublisher;
typedef TcpAcceptor<TPublisher> TAcceptor;

size_t data_callback(const ConstBuffer& data, TPublisher::SharedPtr connection) {
    connection->deliver(data);
    return data.size();
}

int main() {
    string port{"8023"};
    CChannel channel("echo_server");
    TAcceptor::new_instance(port, channel);
    TPublisher::data_callback(boost::bind(data_callback, _1, _2));
    channel.run();
    return 0;
}
