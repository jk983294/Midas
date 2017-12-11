#ifndef NODE3_PROCESS_H
#define NODE3_PROCESS_H

#include "midas/MidasTick.h"
#include "net/channel/Channel.h"
#include "net/common/PacketDeFrame.h"
#include "net/tcp/TcpPublisherAsync.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class Node3Process : public MidasProcessBase {
public:
    CChannel channelIn;
    ConstBufferSequence bufferList;

public:
    Node3Process() = delete;
    Node3Process(int argc, char** argv);
    ~Node3Process();

protected:
    void app_start() override;
    void app_stop() override;

private:
    struct TcpInput {};
    typedef midas::TcpReceiver<TcpInput> TTcpReceiver;

    size_t data_notify(const ConstBuffer& data, MemberPtr member);
    void split_messages(const std::string& s, std::vector<std::string>& ret);

    bool configure();
    void init_admin();

    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;
};

#endif
