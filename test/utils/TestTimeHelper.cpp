#include "catch.hpp"
#include "utils/convert/TimeHelper.h"

using namespace midas;

TEST_CASE("convert time to string", "[time]") {
    struct timespec ts;
    ts.tv_sec = 1496325586;
    ts.tv_nsec = 999061732;

    double dts = timespec2double(ts);
    REQUIRE(dts == 1496325586.9990611076);

    struct timespec ts1;
    double2timespec(dts, ts1);
    REQUIRE(ts1.tv_sec == ts.tv_sec);

    uint64_t nano = timespec2nanos(ts);
    REQUIRE(nano == 1496325586999061732);

    struct timespec ts2;
    nanos2timespec(nano, ts2);
    REQUIRE(ts2.tv_sec == ts.tv_sec);
    REQUIRE(ts2.tv_sec == ts.tv_sec);

    REQUIRE(nano2date(nano) == 20170601);

    struct timeval tv;
    tv.tv_sec = 1496325586;
    tv.tv_usec = 999061;
    REQUIRE(timeval2string(tv) == std::string("2017-06-01 21:59:46.999"));

    REQUIRE(ntime_from_double(20171101.233625) == 1509550584000000000);

    REQUIRE(intraday_time_HMS("21:59:46") == 215946);

    REQUIRE(cob("20171112") == 20171112);

    REQUIRE(intraday_time_HM("21:59") == 2159);

    REQUIRE(time_string2double("20171112 23:59:59") == 1510502399.0);
}

TEST_CASE("intraday", "[intraday]") {
    unsigned int seconds = 0;
    REQUIRE(string_2_second_intraday("101010", "HHMMSS", seconds));
    REQUIRE(seconds == (10 * 60 * 60 + 10 * 60 + 10));

    string str;
    REQUIRE(second_2_string_intraday(seconds, "HHMMSS", str));
    REQUIRE(str == "101010");

    unsigned int microseconds = 0;
    REQUIRE(string_2_microsecond_intraday("101010.101", "HHMMSS.MMM", microseconds));
    REQUIRE(microseconds == ((10 * 60 * 60 + 10 * 60 + 10) * 1000 + 101));

    string str1;
    REQUIRE(microsecond_2_string_intraday(microseconds, "HHMMSS.MMM", str1));
    REQUIRE(str1 == "101010.101");
}
