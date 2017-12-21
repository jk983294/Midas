#ifndef MIDAS_FILE_UTILS_H
#define MIDAS_FILE_UTILS_H

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include "utils/convert/TimeHelper.h"

using namespace std;

namespace midas {

inline bool check_file_exists(const char* path) {
    fstream ifs(path, ios::in);
    if (ifs) {
        ifs.close();
        return true;
    }
    return false;
}

template <typename T1, typename T2>
string dump2file(const map<T1, T2>& m, string filePrefix) {
    string filePath = filePrefix + "." + now_string();
    ofstream ofs(filePath, ofstream::out | ofstream::trunc);

    if (!ofs) {
        cerr << "open file failed" << '\n';
    } else {
        for (auto it = m.begin(); it != m.end(); ++it) ofs << it->second << '\n';
        ofs.close();
    }
    return filePath;
}

template <typename T>
string dump2file(const vector<T>& v, string filePrefix) {
    string filePath = filePrefix + "." + now_string();
    ofstream ofs(filePath, ofstream::out | ofstream::trunc);

    if (!ofs) {
        cerr << "open file failed" << '\n';
    } else {
        for (auto it = v.begin(); it != v.end(); ++it) ofs << *it << '\n';
        ofs.close();
    }
    return filePath;
}

template <typename T>
string dump2file(const T& v, string filePrefix) {
    string filePath = filePrefix + "." + now_string();
    ofstream ofs(filePath, ofstream::out | ofstream::trunc);

    if (!ofs) {
        cerr << "open file failed" << '\n';
    } else {
        ofs << v << '\n';
        ofs.close();
    }
    return filePath;
}

/**
 * if no key provide, then dump all map data
 * if key provided, then only dump map[key]
 */
template <typename T1, typename T2>
void dump2stream(ostream& oss, const map<T1, T2>& m, string key) {
    if (key == "" || key == "all") {
        for (auto it = m.begin(); it != m.end(); ++it) oss << it->second << '\n';
    } else if (m.find(key) != m.end()) {
        auto it = m.find(key);
        oss << it->second << '\n';
    } else {
        oss << "can not find entry for " << key;
    }
}
}

#endif
