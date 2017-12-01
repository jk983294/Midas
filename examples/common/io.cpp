#include <net/tcp/TcpAcceptor.h>
#include <net/tcp/TcpPublisherAsync.h>
#include <net/tcp/TcpPublisherDirect.h>
#include <net/tcp/TcpReceiver.h>
#include <net/udp/UdpReceiver.h>
#include <process/admin/AdminHandler.h>
#include <process/admin/TcpAdmin.h>
#include <utils/log/Log.h>

using namespace midas;

struct MyOutputTag {};
struct MyPublisherTag {};
struct MyTcpReceiverTag {};
struct MyUdpReceiverTag {};

void deadline(boost::shared_ptr<boost::asio::deadline_timer> t) { MIDAS_LOG_INFO("deadline"); }

bool connect_callback(MemberPtr member) {
    MIDAS_LOG_INFO("connected: " << member->get_name());
    return true;
}
bool disconnect_callback(MemberPtr member) {
    MIDAS_LOG_INFO("disconnected: " << member->get_name());
    return true;
}
bool join_notify_any(MemberPtr member, CChannel&) {
    MIDAS_LOG_INFO("new any joiner: " << member->get_name());
    return true;
}
void leave_notify_any(MemberPtr member, CChannel&) { MIDAS_LOG_INFO("leave any: " << member->get_name()); }
bool join_notify_pub(MemberPtr member, CChannel&) {
    MIDAS_LOG_INFO("new pub joiner: " << member->get_name());
    return true;
}
void leave_notify_pub(MemberPtr member, CChannel&) { MIDAS_LOG_INFO("leave pub: " << member->get_name()); }

string admin_test(const string& cmd, const TAdminCallbackArgs& args) {
    string response = string("recv cmd: ") + cmd + " args: ";
    for (const string& s : args) {
        response += s + ", ";
    }
    return response;
}

string admin_meters(const string& cmd, const TAdminCallbackArgs& args, CChannel* c1, CChannel* c2) {
    ostringstream oss;
    oss << "server: " << '\n';
    c1->meters(oss, true);
    oss << '\n' << "clients: " << '\n';
    c2->meters(oss);
    return oss.str();
}

string admin_stats(const string& cmd, const TAdminCallbackArgs& args, CChannel* c1, CChannel* c2) {
    ostringstream oss;
    c1->stats(oss, args.empty() ? string() : args[0]);
    c2->stats(oss, args.empty() ? string() : args[0]);
    return oss.str();
}

size_t data_notify_tcp(const ConstBuffer& data, MemberPtr member, CChannel& c) {
    MutableBuffer buffer(1024, PoolAllocator<1024>());
    size_t s = data.size();
    buffer.store(data.data(), s);
    c.deliver(buffer);
    return s;
}

size_t data_notify_udp(const ConstBuffer& data, MemberPtr member, CChannel& c) {
    MutableBuffer buffer(1024, PoolAllocator<1024>());
    size_t s = data.size();
    buffer.store(data.data(), s);
    c.deliver(buffer);
    return s;
}

int main() {
    string adminPort{"8023"}, dataDirectPort{"8024"}, dataAsyncPort{"8025"};
    CChannel out("out");
    CChannel in("in");
    AdminHandler adminHandler;

    CChannel::register_join_callback(join_notify_any);
    CChannel::register_leave_callback(leave_notify_any);
    CChannel::register_join_callback<DefaultTcpPublisherTag>(join_notify_pub);
    CChannel::register_leave_callback<DefaultTcpPublisherTag>(leave_notify_pub);

    TcpReceiver<MyTcpReceiverTag>::connect_callback(connect_callback);
    TcpReceiver<MyTcpReceiverTag>::disconnect_callback(disconnect_callback);

    TcpReceiver<MyTcpReceiverTag>::data_callback(boost::bind(data_notify_tcp, _1, _2, boost::ref(out)));
    TcpReceiver<MyUdpReceiverTag>::data_callback(boost::bind(data_notify_udp, _1, _2, boost::ref(out)));

    adminHandler.register_callback("test", boost::bind(admin_test, _1, _2), in, "test admin", "test arg1 arg2");
    adminHandler.register_callback("meters", boost::bind(admin_meters, _1, _2, &in, &out), in, "meters", "");
    adminHandler.register_callback("stats", boost::bind(admin_stats, _1, _2, &in, &out), in, "stats", "");

    TcpAcceptor<TcpAdmin>::new_instance(adminPort, adminHandler.get_channel());
    TcpAcceptor<TcpPublisherDirect<>>::new_instance(dataDirectPort, out);
    TcpAcceptor<TcpPublisherAsync<>>::new_instance(dataAsyncPort, out);

    // create input connection
    TcpReceiver<MyTcpReceiverTag>::new_instance("localhost", "10000", in);

    TcpReceiver<MyTcpReceiverTag>::register_admin(in, adminHandler);
    TcpReceiver<MyUdpReceiverTag>::register_admin(in, adminHandler);

    adminHandler.start();  // non-block
    out.run();             // block
    return 0;
}
