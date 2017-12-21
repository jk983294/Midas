#ifndef MIDAS_BACKTEST_DATA_LOADER_H
#define MIDAS_BACKTEST_DATA_LOADER_H

#include <map>
#include <string>
#include <vector>
#include "model/CandleData.h"

using namespace std;

enum RawDataFormat {
    Unknown,
    Type1,  // "Date","Time","Open","High","Low","Close","TotalVolume"
    Type2   // "DateTime","Open","High","Low","Close","TotalVolume"
};

class BacktestDataLoader {
public:
    map<string, vector<CandleData>> instrument2candle;
    string currentFileName;
    string instrumentName;
    vector<string> rawData;
    bool hasHeader{true};
    RawDataFormat format{RawDataFormat::Unknown};

public:
    void load(const string& pathName);

private:
    void load_single_file(const string& pathName);
    void process_data();
    void load_format_type1();
    void load_format_type2();
    void analysis_header(const string& content);
    void analysis_format(const string& content);
};

#endif
