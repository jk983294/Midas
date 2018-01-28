#ifndef MIDAS_CTP_BACK_TESTER_H
#define MIDAS_CTP_BACK_TESTER_H

#include <memory>
#include "model/CtpData.h"
#include "net/channel/Channel.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"
#include "strategy/StrategyFactory.h"
#include "train/Simulator.h"

using namespace std;
using namespace midas;

class CtpBackTester : public MidasProcessBase {
public:
    string dataDirectory;
    string resultDirectory{"/tmp"};
    CandleScale candleScale{CandleScale::Minute1};

    std::shared_ptr<CtpData> data;

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

    void load_test_data(const string& dataPath);

    BacktestResult calculate(StrategyType type);

private:
    // admin section
    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_dump(const string& cmd, const TAdminCallbackArgs& args);
    string admin_csv_dump(const string& cmd, const TAdminCallbackArgs& args);
    string admin_load_file(const string& cmd, const TAdminCallbackArgs& args);
    string admin_train(const string& cmd, const TAdminCallbackArgs& args);
    string admin_calculate(const string& cmd, const TAdminCallbackArgs& args);
};

#endif
