#ifndef MIDAS_MYSQL_Account_H
#define MIDAS_MYSQL_Account_H

#include <cppconn/exception.h>
#include <string>

using namespace std;

class MysqlAccount {
public:
    string ip;
    string userName;
    string password;

public:
    MysqlAccount() {}

    MysqlAccount(const string& _ip, const string& _userName, const string& _password)
        : ip(_ip), userName(_userName), password(_password) {}
};

inline ostream& operator<<(ostream& os, const sql::SQLException& e) {
    os << "MySQL error: " << e.what() << " ( code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )";
    return os;
}

#endif
