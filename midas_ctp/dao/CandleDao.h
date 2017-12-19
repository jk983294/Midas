#ifndef MIDAS_CANDLE_DAO_H
#define MIDAS_CANDLE_DAO_H

#include <mysql_driver.h>
#include <unordered_map>
#include "MysqlCommon.h"
#include "model/CandleData.h"

using namespace std;

class CandleDao {
public:
    sql::mysql::MySQL_Driver& driver;
    const MysqlAccount& account;

public:
    CandleDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account);

    void get_all_candles(std::unordered_map<std::string, std::vector<CandleData>>& data);

    int save_candles(const std::string& instrument, const std::vector<CandleData>& data, size_t pos);
};

#endif
