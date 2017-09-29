#ifndef MIDAS_VISUAL_HELPER_H
#define MIDAS_VISUAL_HELPER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <iconv.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

using namespace std;

inline void codeConvert(char* from, char* to, char* src, char* dst, size_t dstSize) {
    size_t sl, dl;
    sl = strlen(src);
    dl = dstSize;
    char* pIn = src;
    char* pOut = dst;
    memset(dst, '\0', dstSize);
    iconv_t conv = iconv_open(to, from);
    iconv(conv, &pIn, &sl, &pOut, &dl);
    iconv_close(conv);
}

inline void gbk2utf8(char* src, char* dst, size_t dstSize) {
    char* gbk = (char*)"GBK";
    char* utf8 = (char*)"UTF-8";
    codeConvert(gbk, utf8, src, dst, dstSize);
}

inline string gbk2utf8(char* src) {
    char dest[256];
    gbk2utf8(src, dest, sizeof(dest));
    return string(dest);
}

template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const set<Type>& _set) {
    for (auto itr = _set.begin(); itr != _set.end(); itr++) os << *itr << " ";
    return os;
}

template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const vector<Type>& v) {
    for (auto itr = v.begin(); itr != v.end(); itr++) os << *itr << " ";
    return os;
}
template <typename Type>
inline std::ostream& operator<<(std::ostream& os, const vector<vector<Type> >& v) {
    for (auto itr = v.begin(); itr != v.end(); itr++) os << *itr << endl;
    return os;
}

#endif
