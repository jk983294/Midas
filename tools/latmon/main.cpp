#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include "LatencyStatistics.h"
#include "utils/convert/TimeHelper.h"

using namespace std;
using namespace midas;

string get_field(const string& input, const string& fieldName) {
    string toFind = fieldName + ' ';
    auto itr1 = input.find(toFind);
    if (itr1 == string::npos)
        return "";
    else
        itr1 += (fieldName.size() + 1);
    auto itr2 = input.find(',', itr1) - 1;
    return input.substr(itr1, itr2 - itr1);
}

int main(int argc, char** argv) {
    vector<LatencyStatistics> lsv{60, 60 * 10, 60 * 60, 0};
    string line;
    while (getline(cin, line)) {
        if (line.empty() || *(line.end() - 1) != ';') continue;

        string rcvtStr = get_field(line, "rcvt");
        string updateTime = get_field(line, "ut");
        string updateMillisecond = get_field(line, "um");
        string actionDate = get_field(line, "ad");

        if (!rcvtStr.empty() && !updateTime.empty()) {
            double rcvt = std::atof(rcvtStr.c_str()) / 1e9;
            double time =
                midas::time_string2double(actionDate + " " + updateTime) + std::atof(updateMillisecond.c_str()) * 1e-3;

            for (auto& ls : lsv) {
                ls.add(rcvt, time);
            }
        }
    }

    for (auto& ls : lsv) {
        ls.print_result();
    }

    return 0;
}
