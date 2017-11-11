#ifndef MIDAS_CANDLE_DATA_H
#define MIDAS_CANDLE_DATA_H

enum CandleScale { Minute1, Minute5, Minute15, Minute30, Hour1, Day1 };

class CandleData {
public:
    CandleScale scale;
    int date;
    int time;
    double open;
    double close;
    double high;
    double low;

public:
    CandleData() {}
    CandleData(int date, int time, double open, double close, double high, double low);
};

#endif
