#ifndef NODE2_PROCESS_H
#define NODE2_PROCESS_H

#include "midas/MidasTick.h"
#include "net/channel/Channel.h"
#include "net/common/PacketDeFrame.h"
#include "net/tcp/TcpPublisherAsync.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class Node2Process : public MidasProcessBase {
public:
    CChannel channelIn;
    CChannel channelOut;
    ConstBufferSequence bufferList;
    MidasTick tick;

public:
    Node2Process() = delete;
    Node2Process(int argc, char** argv);
    ~Node2Process();

protected:
    void app_start() override;
    void app_stop() override;

private:
    struct TcpInput {};
    struct TcpOutput {};
    typedef midas::TcpReceiver<TcpInput> TTcpReceiver;
    typedef TcpAcceptor<TcpPublisherAsync<TcpOutput>> TTcpOutputAcceptor;

    size_t data_notify(const ConstBuffer& data, MemberPtr member);
    void split_messages(const std::string& s, std::vector<std::string>& ret);

    bool configure();
    void init_admin();

    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;
};

#endif
