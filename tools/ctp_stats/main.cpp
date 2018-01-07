#include <model/CtpData.h>
#include <trade/TradeStatusManager.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "InstrumentStats.h"
#include "Utils.h"

using namespace std;
using namespace midas;
namespace po = boost::program_options;

enum StatType { TradingHour, CandleData };

int main(int argc, char** argv) {
    int typeNumber{0};
    string tradeSessionFile{""};
    string inputFile{""};
    po::options_description desc("Program options");
    desc.add_options()("help,h", "print_trading_hour help")
            ("type,t", po::value<int>(&typeNumber)->default_value(0),
             "stats type\n\t0) trading hour\n\t1) candle data")
            ("session_file,s", po::value<string>(&tradeSessionFile)->default_value(""), "trade session file")
            ("input_file,f", po::value<string>(&inputFile)->default_value(""), "input tick file");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    StatType type = StatType(typeNumber);

    if (type == StatType::CandleData && tradeSessionFile.empty()) {
        cout << "must provide trade session file!" << '\n';
        return 0;
    }

    ifstream ifs(inputFile, ifstream::in);
    istream* is = &ifs;
    if (vm.count("input_file") == 0) {
        is = &cin;
    } else if (!ifs.good()) {
        cout << "tick file error!" << '\n';
        return 0;
    }

    CtpData data;
    data.tradeStatusManager.load_trade_session(tradeSessionFile);
    map<string, InstrumentStats> stats;
    string line;
    MidasTick tick;
    MktDataPayload payload;
    while (getline(*is, line)) {
        if (line.empty() || *(line.end() - 1) != ';') continue;
        tick.parse(line.c_str(), line.size());
        CThostFtdcDepthMarketDataField field = convert(tick);
        payload.data_ = field;
        payload.rcvt = tick["rcvt"];
        string instrumentId{field.InstrumentID};

        auto itr = stats.find(instrumentId);
        if (itr != stats.end()) {
            itr->second.update(field);
        } else {
            stats.insert({instrumentId, instrumentId});
            stats[instrumentId].update(field);
        }

        auto itr1 = data.instruments.find(instrumentId);
        if (itr1 != data.instruments.end()) {
            itr1->second->update_tick(payload);
        } else {
            const TradeSessions& pts = data.tradeStatusManager.get_session(instrumentId);
            std::shared_ptr<CtpInstrument> instrument = make_shared<CtpInstrument>(instrumentId, pts);
            instrument->update_tick(payload);
            data.instruments.insert({instrumentId, instrument});
        }
    }

    if (type == StatType::TradingHour) {
        for (auto it = stats.begin(); it != stats.end(); ++it) {
            it->second.print_trading_hour();
        }
    } else if (type == StatType::CandleData) {
        // cout << data.tradeStatusManager << '\n';
        for (auto it = data.instruments.begin(); it != data.instruments.end(); ++it) {
            cout << *(it->second) << '\n';
        }
    }

    return 0;
}
