#include <utils/FileUtils.h>
#include "CtpBackTester.h"

void CtpBackTester::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&CtpBackTester::admin_meters, this, _1, _2),
                                      "display statistical information", "meters");
    admin_handler().register_callback("dump", boost::bind(&CtpBackTester::admin_dump, this, _1, _2), "dump (candle)",
                                      "dump data");
}

string CtpBackTester::admin_dump(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];

    string dumpFile;
    if (param1 == "candle") {
        auto itr = data->instruments.find(param2);
        if (itr != data->instruments.end()) {
            dumpFile = dump2file<CtpInstrument>(itr->second, resultDirectory + "/" + param2 + ".dump");
        } else {
            return "instrument cannot found.";
        }
    }
    return dumpFile + " dump finished";
}

string CtpBackTester::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << "instruments:\n";
    for (const auto& item : data->instruments) {
        oss << item.first << ", ";
    }
    oss << "\n";
    return oss.str();
}
