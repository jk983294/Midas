#ifndef MIDAS_NET_DATA_H
#define MIDAS_NET_DATA_H

#include <cstdint>
#include <string>

using namespace std;

namespace midas {

enum NetState { Created = 0, Pending, Connected, Disconnected, Closed };
enum NetProtocol { p_any = 0, p_tcp_primary, p_tcp_secondary, p_udp, p_timer, p_file, p_local };
enum TcpAddressType { PrimaryAddress = 0, SecondaryAddress, AddressNumber };

struct IoStatData {
    uint64_t bytesSent{0};
    uint64_t bytesRecv{0};
    uint64_t msgsSent{0};
    uint64_t msgsRecv{0};
    string connectionId;
    NetState state{NetState::Created};
    time_t connectTime{0};

    IoStatData() {}
};

// const string DefaultChannelCfgPath = "midas.io.channel";

// traits
struct CustomCallbackTag {};
struct DefaultCallbackTag {};
struct StopRunTag {};
struct DefaultPoolTag {};
struct DefaultTimerTag {};
struct DefaultTcpReceiverTag {};
struct DefaultTcpPublisherTag {};
struct DefaultUdpReceiverTag {};
struct DefaultUdpPublisherTag {};
}

#endif
