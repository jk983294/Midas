#ifndef MIDAS_CANDLE_DAO_H
#define MIDAS_CANDLE_DAO_H

#include <cppconn/exception.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include "MysqlCommon.h"
#include "model/CandleData.h"

class CandleDao {
public:
    sql::mysql::MySQL_Driver& driver;
    const MysqlAccount& account;

public:
    CandleDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account);
};

#endif
