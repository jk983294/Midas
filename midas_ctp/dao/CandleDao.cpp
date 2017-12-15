#include "CandleDao.h"

CandleDao::CandleDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account)
    : driver(_driver), account(_account) {}
