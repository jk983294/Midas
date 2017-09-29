#include "catch.hpp"
#include "utils/ConvertHelper.h"

using namespace midas;

TEST_CASE("convert int to string", "[convert]") {
    REQUIRE(int2str(42) == std::string("42"));
    REQUIRE(int2str(-42) == std::string("-42"));
    REQUIRE(int2str(static_cast<uint8_t>(42)) == std::string("42"));
    REQUIRE(int2str(123456789) == std::string("123456789"));
    REQUIRE(int2str(-123456789) == std::string("-123456789"));
}

TEST_CASE("convert fixed-point to string", "[fixed-point]") {
    REQUIRE(fixed_point_string_size(0, 0) == 1);
    REQUIRE(fixed_point_string_size(0, 1) == 3);
    REQUIRE(fixed_point_string_size(0, -1) == 1);
    REQUIRE(fixed_point_string_size(0, 20) == 22);     // 0.0{20}
    REQUIRE(fixed_point_string_size(0, 21) == 22);     // max places
    REQUIRE(fixed_point_string_size(123, 20) == 22);   // 0.0{17}123
    REQUIRE(fixed_point_string_size(-123, 20) == 23);  // -0.0{17}123

    REQUIRE(fixed_point_2_string(42, 6) == std::string("0.000042"));
    REQUIRE(fixed_point_2_string(-42, 6) == std::string("-0.000042"));
    REQUIRE(fixed_point_2_string(4200, 6) == std::string("0.004200"));
    REQUIRE(fixed_point_2_string(-4200, 6) == std::string("-0.004200"));

    REQUIRE(fixed_point_2_string_trim(42, 6) == std::string("0.000042"));
    REQUIRE(fixed_point_2_string_trim(-42, 6) == std::string("-0.000042"));
    REQUIRE(fixed_point_2_string_trim(4200, 6) == std::string("0.0042"));
    REQUIRE(fixed_point_2_string_trim(-4200, 6) == std::string("-0.0042"));
}

TEST_CASE("convert float to string", "[float2str]") {
    REQUIRE(float_2_string(0.499999, 7) == std::string("0.4999990"));
    REQUIRE(float_2_string(0.500001, 7) == std::string("0.5000010"));
    REQUIRE(float_2_string(0.499999, 5) == std::string("0.50000"));
    REQUIRE(float_2_string(0.500001, 5) == std::string("0.50000"));

    REQUIRE(float_2_string(-0.499999, 7) == std::string("-0.4999990"));
    REQUIRE(float_2_string(-0.500001, 7) == std::string("-0.5000010"));
    REQUIRE(float_2_string(-0.499999, 5) == std::string("-0.50000"));
    REQUIRE(float_2_string(-0.500001, 5) == std::string("-0.50000"));

    REQUIRE(float_2_string(0.499999, 6) == std::string("0.499999"));
    REQUIRE(float_2_string(0.5, 6) == std::string("0.500000"));
    REQUIRE(float_2_string(0.500001, 6) == std::string("0.500001"));
    REQUIRE(float_2_string(1.5, 6) == std::string("1.500000"));

    REQUIRE(float_2_string(-0.499999, 6) == std::string("-0.499999"));
    REQUIRE(float_2_string(-0.5, 6) == std::string("-0.500000"));
    REQUIRE(float_2_string(-0.500001, 6) == std::string("-0.500001"));
    REQUIRE(float_2_string(-1.5, 6) == std::string("-1.500000"));

    REQUIRE(float_2_string(INFINITY, 6) == std::string("inf"));
    REQUIRE(float_2_string(NAN, 6) == std::string("nan"));
}

TEST_CASE("convert float to string with trim", "[float2str_trim]") {
    REQUIRE(float_2_string_trim(0.499999, 7) == std::string("0.499999"));
    REQUIRE(float_2_string_trim(0.500001, 7) == std::string("0.500001"));
    REQUIRE(float_2_string_trim(0.499999, 5) == std::string("0.5"));
    REQUIRE(float_2_string_trim(0.500001, 5) == std::string("0.5"));

    REQUIRE(float_2_string_trim(-0.499999, 7) == std::string("-0.499999"));
    REQUIRE(float_2_string_trim(-0.500001, 7) == std::string("-0.500001"));
    REQUIRE(float_2_string_trim(-0.499999, 5) == std::string("-0.5"));
    REQUIRE(float_2_string_trim(-0.500001, 5) == std::string("-0.5"));

    REQUIRE(float_2_string_trim(0.499999, 6) == std::string("0.499999"));
    REQUIRE(float_2_string_trim(0.5, 6) == std::string("0.5"));
    REQUIRE(float_2_string_trim(0.500001, 6) == std::string("0.500001"));
    REQUIRE(float_2_string_trim(1.5, 6) == std::string("1.5"));

    REQUIRE(float_2_string_trim(-0.499999, 6) == std::string("-0.499999"));
    REQUIRE(float_2_string_trim(-0.5, 6) == std::string("-0.5"));
    REQUIRE(float_2_string_trim(-0.500001, 6) == std::string("-0.500001"));
    REQUIRE(float_2_string_trim(-1.5, 6) == std::string("-1.5"));

    REQUIRE(float_2_string_trim(INFINITY, 6) == std::string("inf"));
    REQUIRE(float_2_string_trim(NAN, 6) == std::string("nan"));
}

TEST_CASE("str2int", "[str2int]") {
    REQUIRE(str2int64(string("12345.6789"), -8) == 0LL);
    REQUIRE(str2int64(string("12345.6789"), -7) == 0LL);
    REQUIRE(str2int64(string("12345.6789"), -6) == 0LL);
    REQUIRE(str2int64(string("12345.6789"), -5) == 0LL);
    REQUIRE(str2int64(string("12345.6789"), -4) == 1LL);
    REQUIRE(str2int64(string("12345.6789"), -3) == 12LL);
    REQUIRE(str2int64(string("12345.6789"), -2) == 123LL);
    REQUIRE(str2int64(string("12345.6789"), -1) == 1234LL);
    REQUIRE(str2int64(string("12345.6789"), 0) == 12345LL);
    REQUIRE(str2int64(string("12345.6789"), 1) == 123456LL);
    REQUIRE(str2int64(string("12345.6789"), 2) == 1234567LL);
    REQUIRE(str2int64(string("12345.6789"), 3) == 12345678LL);
    REQUIRE(str2int64(string("12345.6789"), 4) == 123456789LL);
    REQUIRE(str2int64(string("12345.6789"), 5) == 1234567890LL);

    REQUIRE(str2int64(string(" 12345.6789, "), -8) == 0LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -7) == 0LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -6) == 0LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -5) == 0LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -4) == 1LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -3) == 12LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -2) == 123LL);
    REQUIRE(str2int64(string(" 12345.6789, "), -1) == 1234LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 0) == 12345LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 1) == 123456LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 2) == 1234567LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 3) == 12345678LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 4) == 123456789LL);
    REQUIRE(str2int64(string(" 12345.6789, "), 5) == 1234567890LL);

    REQUIRE(str2int64(string(" +12345.6789, "), -5) == 0LL);
    REQUIRE(str2int64(string(" +12345.6789, "), -4) == 1LL);
    REQUIRE(str2int64(string(" +12345.6789, "), -3) == 12LL);
    REQUIRE(str2int64(string(" +12345.6789, "), -2) == 123LL);
    REQUIRE(str2int64(string(" +12345.6789, "), -1) == 1234LL);

    REQUIRE(str2int64(string(" -12345.6789, "), -5) == 0LL);
    REQUIRE(str2int64(string(" -12345.6789, "), -4) == -1LL);
    REQUIRE(str2int64(string(" -12345.6789, "), -3) == -12LL);
    REQUIRE(str2int64(string(" -12345.6789, "), -2) == -123LL);
    REQUIRE(str2int64(string(" -12345.6789, "), -1) == -1234LL);

    REQUIRE(str2int64(string(""), 0) == 0LL);
    REQUIRE(str2int64(string("abc"), 0) == 0LL);
    REQUIRE(str2int64(string(" abc "), 0) == 0LL);
}

TEST_CASE("str2uint", "[str2uint]") {
    REQUIRE(str2uint64(string("12345.6789"), -8) == 0ULL);
    REQUIRE(str2uint64(string("12345.6789"), -7) == 0ULL);
    REQUIRE(str2uint64(string("12345.6789"), -6) == 0ULL);
    REQUIRE(str2uint64(string("12345.6789"), -5) == 0ULL);
    REQUIRE(str2uint64(string("12345.6789"), -4) == 1ULL);
    REQUIRE(str2uint64(string("12345.6789"), -3) == 12ULL);
    REQUIRE(str2uint64(string("12345.6789"), -2) == 123ULL);
    REQUIRE(str2uint64(string("12345.6789"), -1) == 1234ULL);
    REQUIRE(str2uint64(string("12345.6789"), 0) == 12345ULL);
    REQUIRE(str2uint64(string("12345.6789"), 1) == 123456ULL);
    REQUIRE(str2uint64(string("12345.6789"), 2) == 1234567ULL);
    REQUIRE(str2uint64(string("12345.6789"), 3) == 12345678ULL);
    REQUIRE(str2uint64(string("12345.6789"), 4) == 123456789ULL);
    REQUIRE(str2uint64(string("12345.6789"), 5) == 1234567890ULL);

    REQUIRE(str2uint64(string(" 12345.6789, "), -8) == 0ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -7) == 0ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -6) == 0ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -5) == 0ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -4) == 1ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -3) == 12ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -2) == 123ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), -1) == 1234ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 0) == 12345ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 1) == 123456ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 2) == 1234567ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 3) == 12345678ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 4) == 123456789ULL);
    REQUIRE(str2uint64(string(" 12345.6789, "), 5) == 1234567890ULL);

    REQUIRE(str2uint64(string(" +12345.6789, "), -5) == 0ULL);
    REQUIRE(str2uint64(string(" +12345.6789, "), -4) == 1ULL);
    REQUIRE(str2uint64(string(" +12345.6789, "), -3) == 12ULL);
    REQUIRE(str2uint64(string(" +12345.6789, "), -2) == 123ULL);
    REQUIRE(str2uint64(string(" +12345.6789, "), -1) == 1234ULL);

    REQUIRE(str2uint64(string(""), 0) == 0ULL);
    REQUIRE(str2uint64(string("abc"), 0) == 0ULL);
    REQUIRE(str2uint64(string(" abc "), 0) == 0ULL);
}

TEST_CASE("expand_scientific_number", "[expand_scientific_number]") {
    REQUIRE(expand_scientific_number("1.23e0") == "1.23");
    REQUIRE(expand_scientific_number("1.23e+0") == "1.23");
    REQUIRE(expand_scientific_number("1.23e-0") == "1.23");
    REQUIRE(expand_scientific_number("1.23e1") == "12.3");
    REQUIRE(expand_scientific_number("1.23e2") == "123");
    REQUIRE(expand_scientific_number("1.23e3") == "1230");
    REQUIRE(expand_scientific_number("1.23e+1") == "12.3");
    REQUIRE(expand_scientific_number("1.23e+2") == "123");
    REQUIRE(expand_scientific_number("1.23e-1") == "0.123");
    REQUIRE(expand_scientific_number("1.23e-2") == "0.0123");

    REQUIRE(expand_scientific_number("0.23e0") == "0.23");
    REQUIRE(expand_scientific_number("0.23e+0") == "0.23");
    REQUIRE(expand_scientific_number("0.23e-0") == "0.23");
    REQUIRE(expand_scientific_number("0.23e1") == "2.3");
    REQUIRE(expand_scientific_number("0.23e2") == "23");
    REQUIRE(expand_scientific_number("0.23e3") == "230");
    REQUIRE(expand_scientific_number("0.23e+1") == "2.3");
    REQUIRE(expand_scientific_number("0.23e+2") == "23");
    REQUIRE(expand_scientific_number("0.23e-1") == "0.023");
    REQUIRE(expand_scientific_number("0.23e-2") == "0.0023");

    REQUIRE(expand_scientific_number(".23e0") == ".23");
    REQUIRE(expand_scientific_number(".23e+0") == ".23");
    REQUIRE(expand_scientific_number(".23e-0") == ".23");
    REQUIRE(expand_scientific_number(".23e1") == "2.3");
    REQUIRE(expand_scientific_number(".23e2") == "23");
    REQUIRE(expand_scientific_number(".23e3") == "230");
    REQUIRE(expand_scientific_number(".23e+1") == "2.3");
    REQUIRE(expand_scientific_number(".23e+2") == "23");
    REQUIRE(expand_scientific_number(".23e-1") == "0.023");
    REQUIRE(expand_scientific_number(".23e-2") == "0.0023");

    REQUIRE(expand_scientific_number("23e0") == "23");
    REQUIRE(expand_scientific_number("23e+0") == "23");
    REQUIRE(expand_scientific_number("23e-0") == "23");
    REQUIRE(expand_scientific_number("23e1") == "230");
    REQUIRE(expand_scientific_number("23e2") == "2300");
    REQUIRE(expand_scientific_number("23e3") == "23000");
    REQUIRE(expand_scientific_number("23e+1") == "230");
    REQUIRE(expand_scientific_number("23e+2") == "2300");
    REQUIRE(expand_scientific_number("23e-1") == "2.3");
    REQUIRE(expand_scientific_number("23e-2") == "0.23");
}
