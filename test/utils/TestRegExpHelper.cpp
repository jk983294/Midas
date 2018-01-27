#include "catch.hpp"
#include "utils/RegExpHelper.h"

using namespace midas;

TEST_CASE("RegExpHelper", "[RegExpHelper]") {
    REQUIRE(!is_int("\t hello\t \nworld \t\n"));
    REQUIRE(!is_int("3.1415"));
    REQUIRE(is_int("479"));
    REQUIRE(!is_int("-3.12"));
    REQUIRE(is_int("-567"));
    REQUIRE(!is_int("-0.2364"));
    REQUIRE(!is_int("+0.2364"));
    REQUIRE(!is_int("+4."));
    REQUIRE(!is_int("-1.2364e12"));
    REQUIRE(!is_int("-1.2364e-12"));
    REQUIRE(!is_int("-1,234,345,234.5435"));
    REQUIRE(!is_int("-1,234,5,234.5435"));
    REQUIRE(!is_int("-1.2364e"));
    REQUIRE(!is_int(""));

    REQUIRE(!is_double("\t hello\t \nworld \t\n"));
    REQUIRE(is_double("3.1415"));
    REQUIRE(!is_double("479"));
    REQUIRE(is_double("-3.12"));
    REQUIRE(!is_double("-567"));
    REQUIRE(is_double("-0.2364"));
    REQUIRE(is_double("+0.2364"));
    REQUIRE(is_double("+4.1"));
    REQUIRE(!is_double(""));
}
