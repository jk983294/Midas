#ifndef MIDAS_CTP_BACK_TESTER_H
#define MIDAS_CTP_BACK_TESTER_H

#include <memory>
#include "model/CtpData.h"
#include "net/channel/Channel.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class CtpBackTester : public MidasProcessBase {
public:
    string dataDirectory;
    int fileType{-1};

public:
    CtpBackTester() = delete;
    CtpBackTester(int argc, char** argv);
    ~CtpBackTester();

protected:
    void app_start() override;
    void app_stop() override;

private:
    bool configure();
    void init_admin();

    void load_data();

private:
    // admin section
    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;
};

#endif
