#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include "InstrumentStats.h"
#include "midas/MidasTick.h"

using namespace std;
using namespace midas;
namespace po = boost::program_options;

int main(int argc, char** argv) {
    int type{0};
    po::options_description desc("Program options");
    desc.add_options()("help,h", "print help")("type,t", po::value<int>(&type)->default_value(0),
                                               "stats type\n\t0) trading hour");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return 0;
    }

    map<string, InstrumentStats> stats;
    string line;
    MidasTick tick;
    while (getline(cin, line)) {
        if (line.empty() || *(line.end() - 1) != ';') continue;
        tick.parse(line.c_str(), line.size());
        string instrumentId = tick["id"].value_string();
        auto itr = stats.find(instrumentId);
        if (itr != stats.end()) {
            itr->second.update(tick);
        } else {
            stats.insert({instrumentId, instrumentId});
            stats[instrumentId].update(tick);
        }
    }

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        it->second.print();
    }
    return 0;
}
