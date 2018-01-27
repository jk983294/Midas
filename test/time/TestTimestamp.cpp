#include "catch.hpp"
#include "time/Timestamp.h"

using namespace midas;

TEST_CASE("test Timestamp", "[Timestamp]") {
    Timestamp timestamp(20170103, 194700);
    REQUIRE(timestamp.time2string() == "19:47:00");
    REQUIRE(timestamp.cob2string() == "2017-01-03");
    REQUIRE(timestamp.to_string() == "2017-01-03 19:47:00");
}
