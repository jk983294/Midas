#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include "LatencyStatistics.h"

using namespace std;
using namespace midas;

/**
 * given two files contains market data, both file line contains receive time from exchange
 * however message sequence may not exactly the same for two file
 * so need to use match algorithm to get the same line represent the same record
 * choose (id, exchange, volumeSize) as unique id, use longest common string dynamic programming to match
 */

struct Record {
    string id;
    string exchange;
    int size;
    double rcvt;
};

string get_field(const string& input, const string& fieldName);
void parse_file(string fileName, map<string, map<string, vector<Record>>>& exch2id2record);
std::vector<std::pair<int, int>> lcs(vector<Record>& a, vector<Record>& b);

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "latcmp file1 file2" << endl;
        return -1;
    }
    map<string, map<string, vector<Record>>> exch2id2record1;
    map<string, map<string, vector<Record>>> exch2id2record2;
    parse_file(argv[1], exch2id2record1);
    parse_file(argv[2], exch2id2record2);

    for (auto& item : exch2id2record1) {
        string exchange = item.first;
        if (exch2id2record2.find(exchange) == exch2id2record2.end()) {
            cout << "no exchange " << exchange << " in second file" << endl;
            continue;
        }

        vector<LatencyStatistics> lsv{60, 60 * 10, 60 * 60, 0};
        map<string, vector<Record>>& id2record1 = exch2id2record1[exchange];
        map<string, vector<Record>>& id2record2 = exch2id2record2[exchange];
        for (auto& sr : id2record1) {
            string id = sr.first;
            if (id2record2.find(id) == id2record2.end()) {
                continue;
            } else {
                std::vector<std::pair<int, int>> match = lcs(sr.second, id2record2[id]);

                for (auto& itr : match) {
                    for (auto& ls : lsv) {
                        ls.add(id2record1[id][itr.first].rcvt, id2record2[id][itr.first].rcvt);
                    }
                }
            }
        }

        cout << "stats for exchange: " << exchange << endl;
        for (auto& ls : lsv) {
            ls.print_result();
        }
    }
    return 0;
}

std::vector<std::pair<int, int>> lcs(vector<Record>& a, vector<Record>& b) {
    size_t lenA = a.size() + 1, lenB = b.size() + 1;
    std::vector<std::vector<int>> v;
    v.resize(lenA);
    for (size_t i = 0; i < lenA; i++) {
        v[i].resize(lenB, 0);
    }
    for (size_t i = 1; i < lenA; i++) {
        for (size_t j = 1; j < lenB; j++) {
            if (a[i - 1].size == b[j - 1].size) {
                v[i][j] = v[i - 1][j - 1] + 1;
            } else {
                v[i][j] = std::max(v[i - 1][j], v[i][j - 1]);
            }
        }
    }

    // based on lcs matrix, find a possible match sequence
    std::vector<std::pair<int, int>> match;
    size_t i = a.size(), j = b.size();
    while (i > 0 && j > 0) {
        if (v[i][j] == (v[i - 1][j - 1] + 1) && a[i - 1].size == b[j - 1].size) {
            match.push_back({i - 1, j - 1});
            --i;
            --j;
        } else if (v[i][j] == v[i - 1][j]) {
            --i;
        } else {
            --j;
        }
    }
    std::reverse(match.begin(), match.end());
    return match;
}

void parse_file(string fileName, map<string, map<string, vector<Record>>>& exch2id2record) {
    ifstream ifs(fileName, ifstream::in);
    string line;
    while (getline(ifs, line)) {
        if (line.empty() || *(line.end() - 1) != ';') continue;

        string rcvtStr = get_field(line, "rcvt");
        string id = get_field(line, "id");
        string exchange = get_field(line, "e");
        string size = get_field(line, "ts");

        if (!rcvtStr.empty() && !id.empty() && !size.empty()) {
            if (exchange.empty()) exchange = "fake";

            double rcvt = std::atof(rcvtStr.c_str()) / 1e9;
            int ts = std::atoi(size.c_str());

            Record r{id, exchange, ts, rcvt};
            exch2id2record[exchange][id].push_back(r);
        }
    }
}

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
