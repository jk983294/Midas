#ifndef NODE1_PROCESS_H
#define NODE1_PROCESS_H

#include "TimerThread.h"
#include "net/channel/Channel.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class Node1Process : public MidasProcessBase {
public:
    CChannel channelIn;
    CChannel channelOut;

    TimerThread timer;
    bool cancelAdmin{false};
    std::mutex mutexAdmin;
    std::condition_variable cvAdmin;
    std::map<uint64_t, string> outAdmin;

public:
    Node1Process() = delete;
    Node1Process(int argc, char** argv);
    ~Node1Process();

protected:
    void app_start() override;
    void app_stop() override;

private:
    struct TcpInput {};
    typedef midas::TcpReceiver<TcpInput> TTcpReceiver;

    void set_log_level(int argc, char** argv) const;
    bool configure();
    void init_admin();
    uint32_t configure_heart_beats(bool lock = true);

    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;

    void heart_beat_timer();
};

#endif
