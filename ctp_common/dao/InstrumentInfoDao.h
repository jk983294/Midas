#ifndef MIDAS_INSTRUMENT_INFO_DAO_H
#define MIDAS_INSTRUMENT_INFO_DAO_H

#include <cppconn/exception.h>
#include <ctp/ThostFtdcUserApiStruct.h>
#include <mysql_driver.h>
#include <vector>
#include "MysqlCommon.h"

using namespace std;

class InstrumentInfoDao {
public:
    sql::mysql::MySQL_Driver& driver;
    const MysqlAccount& account;

public:
    InstrumentInfoDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account);

    std::vector<string> get_all_instruments();

    int save_instruments(const std::vector<CThostFtdcInstrumentField>& instruments);
};

#endif
