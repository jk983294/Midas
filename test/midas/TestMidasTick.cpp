#include <iostream>
#include "catch.hpp"
#include "midas/MidasTick.h"

using namespace midas;

TEST_CASE("empty field", "[field]") {
    TickField<> field;
    REQUIRE(!field.is_valid());
    REQUIRE(field.size() == -1);

    TickField<string> field1;
    REQUIRE(!field1.is_valid());
    REQUIRE(field1.size() == -1);
}

TEST_CASE("iterator range field", "[field]") {
    const char id[] = "id";
    const char kun[] = "kun";
    const char hello[] = "hello";
    const char world[] = "world";
    const char damn[] = "damn";
    const string shit{"shit"};
    TickField<> field(id, kun);
    REQUIRE(!std::strcmp(field.get_key().begin(), id));
    REQUIRE(field.key_string() == id);
    REQUIRE(!std::strcmp(field.get_value().begin(), kun));
    REQUIRE(field.value_string() == kun);
    field.set_value(hello);
    REQUIRE(!std::strcmp(field.get_value().begin(), hello));
    REQUIRE(field.value_string() == hello);
    field.set_value(world, 5);
    REQUIRE(!std::strcmp(field.get_value().begin(), world));
    REQUIRE(field.value_string() == world);
    REQUIRE(field.is_valid());
    REQUIRE(field.size() == 5);
    field = damn;
    REQUIRE((field == damn));
    REQUIRE((field != world));
    field = shit.c_str();
    REQUIRE((field == shit));
}

TEST_CASE("string field", "[field]") {
    const char id[] = "id";
    const char kun[] = "kun";
    const char hello[] = "hello";
    const char world[] = "world";
    const char damn[] = "damn";
    const string shit{"shit"};
    TickField<string> field(id, kun);
    REQUIRE((field.get_key() == id));
    REQUIRE(field.key_string() == id);
    REQUIRE((field.get_value() == kun));
    REQUIRE(field.value_string() == kun);
    field.set_value(hello);
    REQUIRE((field.get_value() == hello));
    REQUIRE(field.value_string() == hello);
    field.set_value(world, 5);
    REQUIRE((field.get_value() == world));
    REQUIRE(field.value_string() == world);
    REQUIRE(field.is_valid());
    REQUIRE(field.size() == 5);
    field = damn;
    REQUIRE((field == damn));
    REQUIRE((field != world));
    field = shit.c_str();
    REQUIRE((field == shit));
}

TEST_CASE("iterator range field integer", "[field]") {
    const char id[] = "id";
    const char v1[] = "1234";
    const char v2[] = "-1234";
    TickField<> field(id, v1);
    REQUIRE(field.key_string() == id);
    REQUIRE((field == (int)1234));
    REQUIRE((field == (unsigned int)1234));
    REQUIRE((field == (long)1234));
    REQUIRE((field == (unsigned long)1234));

    field.set_value(v2);
    REQUIRE((field == (int)-1234));
    REQUIRE((field == (long)-1234));
}

TEST_CASE("string field integer", "[field]") {
    const char id[] = "id";
    const char v1[] = "1234";
    const char v2[] = "-1234";
    TickField<string> field(id, v1);
    REQUIRE(field.key_string() == id);
    REQUIRE((field == (int)1234));
    REQUIRE((field == (unsigned int)1234));
    REQUIRE((field == (long)1234));
    REQUIRE((field == (unsigned long)1234));

    field.set_value(v2);
    REQUIRE((field == (int)-1234));
    REQUIRE((field == (long)-1234));
}

TEST_CASE("iterator range field float", "[field]") {
    const char id[] = "id";
    const char v1[] = "1.234";
    const char v2[] = "-1.234";
    TickField<> field(id, v1);
    REQUIRE(field.key_string() == id);
    REQUIRE((field == (double)1.234));
    REQUIRE((field == (float)1.234));

    field.set_value(v2);
    REQUIRE((field == (double)-1.234));
    REQUIRE((field == (float)-1.234));
}

TEST_CASE("string field float", "[field]") {
    const char id[] = "id";
    const char v1[] = "1.234";
    const char v2[] = "-1.234";
    TickField<string> field(id, v1);
    REQUIRE(field.key_string() == id);
    REQUIRE((field == (double)1.234));
    REQUIRE((field == (float)1.234));

    field.set_value(v2);
    REQUIRE((field == (double)-1.234));
    REQUIRE((field == (float)-1.234));
}

static const char msg1[] = "id 1 ,a 42 ,b jk ,c 1.345 ,d 21.1 ,;";
static const string msg2{"id 1 ,a 42 ,b jk ,c 1.345 ,d 21.1 ,;"};

TEST_CASE("iterator range tick", "[tick]") {
    MidasTick tick;
    REQUIRE(tick.size() == 0);
    REQUIRE(tick.empty());

    tick.add("id");
    REQUIRE(tick.size() == 1);
    REQUIRE(!tick.empty());

    tick.set("id", "1");
    tick.add("a", true);
    tick.set("a", "42");
    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE(tick.at(0).key_string() == "a");
    REQUIRE(tick.at(1).key_string() == "id");

    tick.clear();
    REQUIRE(tick.size() == 0);
    REQUIRE(tick.empty());

    MidasTick tick1(msg1, strlen(msg1));
    MidasTick tick2(msg2);
    MidasTick tick3, tick4, tick6;
    tick3.parse(msg1);
    tick4.parse(msg1, strlen(msg1));
    MidasTick tick5(tick4);
    tick6 = tick5;

    REQUIRE(tick1.size() == 5);
    REQUIRE(tick2.size() == 5);
    REQUIRE(tick3.size() == 5);
    REQUIRE(tick4.size() == 5);
    REQUIRE(tick5.size() == 5);
    REQUIRE(tick6.size() == 5);

    REQUIRE(tick1.get("a").value_string() == "42");
    REQUIRE(tick2.get("a").value_string() == "42");
    REQUIRE(tick3.get("a").value_string() == "42");
    REQUIRE(tick4.get("a").value_string() == "42");
    REQUIRE(tick5.get("a").value_string() == "42");
    REQUIRE(tick6.get("a").value_string() == "42");

    tick1.set("a", "43");
    tick2.set("a", "43");
    tick3.set("a", "44");
    tick4.set("a", "45");
    tick5.set("a", "46");
    tick6.set("a", "47");

    REQUIRE(tick1.get("a").value_string() == "43");
    REQUIRE(tick2.get("a").value_string() == "43");
    REQUIRE(tick3.get("a").value_string() == "44");
    REQUIRE(tick4.get("a").value_string() == "45");
    REQUIRE(tick5.get("a").value_string() == "46");
    REQUIRE(tick6.get("a").value_string() == "47");

    REQUIRE(tick1.size() == 5);
    REQUIRE(tick2.size() == 5);
    REQUIRE(tick3.size() == 5);
    REQUIRE(tick4.size() == 5);
    REQUIRE(tick5.size() == 5);
    REQUIRE(tick6.size() == 5);

    tick6 = tick1;
    REQUIRE(tick6.get("a").value_string() == "43");

    ostringstream oss;
    oss << tick1.at(1).get_value() << ", value " << (int)tick1.at(1) << ", " << (int)tick1.get("a");
    REQUIRE(oss.str() == "43, value 43, 43");
}

TEST_CASE("string tick", "[tick]") {
    TMidasTick<string> tick;
    REQUIRE(tick.size() == 0);
    REQUIRE(tick.empty());

    tick.add("id");
    REQUIRE(tick.size() == 1);
    REQUIRE(!tick.empty());

    tick.set("id", "1");
    tick.add("a", true);
    tick.set("a", "42");
    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE(tick.at(0).key_string() == "a");
    REQUIRE(tick.at(1).key_string() == "id");

    tick.clear();
    REQUIRE(tick.size() == 0);
    REQUIRE(tick.empty());

    TMidasTick<string> tick1(msg1, strlen(msg1));
    TMidasTick<string> tick2(msg2);
    TMidasTick<string> tick3, tick4, tick6;
    tick3.parse(msg1);
    tick4.parse(msg1, strlen(msg1));
    TMidasTick<string> tick5(tick4);
    tick6 = tick5;

    REQUIRE(tick1.size() == 5);
    REQUIRE(tick2.size() == 5);
    REQUIRE(tick3.size() == 5);
    REQUIRE(tick4.size() == 5);
    REQUIRE(tick5.size() == 5);
    REQUIRE(tick6.size() == 5);

    REQUIRE(tick1.get("a").value_string() == "42");
    REQUIRE(tick2.get("a").value_string() == "42");
    REQUIRE(tick3.get("a").value_string() == "42");
    REQUIRE(tick4.get("a").value_string() == "42");
    REQUIRE(tick5.get("a").value_string() == "42");
    REQUIRE(tick6.get("a").value_string() == "42");

    tick1.set("a", "43");
    tick2.set("a", "43");
    tick3.set("a", "44");
    tick4.set("a", "45");
    tick5.set("a", "46");
    tick6.set("a", "47");

    REQUIRE(tick1.get("a").value_string() == "43");
    REQUIRE(tick2.get("a").value_string() == "43");
    REQUIRE(tick3.get("a").value_string() == "44");
    REQUIRE(tick4.get("a").value_string() == "45");
    REQUIRE(tick5.get("a").value_string() == "46");
    REQUIRE(tick6.get("a").value_string() == "47");

    REQUIRE(tick1.size() == 5);
    REQUIRE(tick2.size() == 5);
    REQUIRE(tick3.size() == 5);
    REQUIRE(tick4.size() == 5);
    REQUIRE(tick5.size() == 5);
    REQUIRE(tick6.size() == 5);

    tick6 = tick1;
    REQUIRE(tick6.get("a").value_string() == "43");

    ostringstream oss;
    oss << tick1.at(1).get_value() << ", value " << (int)tick1.at(1) << ", " << (int)tick1.get("a");
    REQUIRE(oss.str() == "43, value 43, 43");
}

TEST_CASE("iterator range tick parse light", "[tick]") {
    MidasTick tick;

    tick.add("id");
    tick.add("a");
    tick.add("b");
    tick.add("c");

    tick.parse_light(msg1);
    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE((tick["b"] == "jk"));
    REQUIRE((tick["c"] == 1.345));
    REQUIRE(!tick["d"].is_valid());
}

TEST_CASE("string tick parse light", "[tick]") {
    TMidasTick<string> tick;

    tick.add("id");
    tick.add("a");
    tick.add("b");
    tick.add("c");

    tick.parse_light(msg1);
    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE((tick["b"] == "jk"));
    REQUIRE((tick["c"] == 1.345));
    REQUIRE(!tick["d"].is_valid());
}

template <typename T = boost::iterator_range<const char*>>
struct TestComplete {
    bool operator()(const TickField<T>& field) const { return (field.key_string() == "c"); }
};

TEST_CASE("iterator range tick parse complete", "[tick]") {
    MidasTick tick;
    tick.parse(msg1, strlen(msg1), TestComplete<>());

    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE((tick["b"] == "jk"));
    REQUIRE((tick["c"] == 1.345));
    REQUIRE(!tick["d"].is_valid());
}

TEST_CASE("string tick parse complete", "[tick]") {
    TMidasTick<string> tick;
    tick.parse(msg1, strlen(msg1), TestComplete<string>());

    REQUIRE((tick["id"] == "1"));
    REQUIRE((tick["a"] == "42"));
    REQUIRE((tick["b"] == "jk"));
    REQUIRE((tick["c"] == 1.345));
    REQUIRE(!tick["d"].is_valid());
}

TEST_CASE("cross type", "[tick]") {
    MidasTick tick1;
    TMidasTick<string> tick2;
    tick1.set("a", "1.23");
    tick2 = tick1;

    REQUIRE((tick1["a"] == "1.23"));
    REQUIRE((tick2["a"] == 1.23));

    // tick1 = tick2; // fail because boost::iterator_range<const char*> cannot be initialized with string
}