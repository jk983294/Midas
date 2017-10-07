#ifndef MIDAS_FILE_UTILS_H
#define MIDAS_FILE_UTILS_H

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include "utils/convert/TimeHelper.h"

using namespace std;

namespace midas {

template <typename T1, typename T2>
void dump2file(const map<T1, T2>& m, string filePrefix) {
    ofstream ofs(filePrefix + "." + now_string(), ofstream::out | ofstream::trunc);

    if (!ofs) {
        cerr << "open file failed" << endl;
    } else {
        for (auto it = m.begin(); it != m.end(); ++it) ofs << it->second << endl;
        ofs.close();
    }
}

template <typename T>
void dump2file(const vector<T>& v, string filePrefix) {
    ofstream ofs(filePrefix + now_string(), ofstream::out | ofstream::trunc);

    if (!ofs) {
        cerr << "open file failed" << endl;
    } else {
        for (auto it = v.begin(); it != v.end(); ++it) ofs << *it << endl;
        ofs.close();
    }
}

/**
 * if no key provide, then dump all map data
 * if key provided, then only dump map[key]
 */
template <typename T1, typename T2>
void dump2stream(ostream& oss, const map<T1, T2>& m, string key) {
    if (key == "" || key == "all") {
        for (auto it = m.begin(); it != m.end(); ++it) oss << it->second << endl;
    } else if (m.find(key) != m.end()) {
        auto it = m.find(key);
        oss << it->second << endl;
    } else {
        oss << "can not find entry for " << key;
    }
}
}

#endif
