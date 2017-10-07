#ifndef MIDAS_CTP_ORDER_BOOK_H
#define MIDAS_CTP_ORDER_BOOK_H

#include <string>

using namespace std;

class CtpOrderBook {
public:
    string instrument;
    double bap[10];
    double bbp[10];
    int bas[10];
    int bbs[10];

public:
};

#endif
