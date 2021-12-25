#include <iostream>

#include "doctest.h"

#include "value.h"

TEST_CASE("use as integer") {
    using namespace runtime;
    Value val = to_value(20);
    Value val2 = to_value(30);
    Value val3 = value_sub(val, val2);
    CHECK(value_get_int32(val3) == -10);
    
    Value val4 = to_value(5);
    Value val5 = value_mul(val3, val4);
    CHECK(value_get_int32(val5) == -50);
}

TEST_CASE("string") {
    using namespace runtime;
    SUBCASE("inline") {
        auto val = to_value(std::string{"hello"});
        std::string str = value_get_std_string(val);
        CHECK(str == "hello");
    }
    SUBCASE("heap allocated") {
        auto val = to_value(std::string{"hello world"});
        std::string str = value_get_std_string(val);
        CHECK(str == "hello world");
    }
}

TEST_CASE("addition") {
    using namespace runtime;
    Value int0 = to_value(0);
    Value int1 = to_value(1);
    Value int2 = to_value(2);
    Value string_empty = to_value(std::string{""});
    Value string_space = to_value(std::string{" "});
    Value string_hello = to_value(std::string{"hello"});
    Value string_long = to_value(std::string{"this is a long string"});

    SUBCASE("integers") {
        CHECK(value_get_int32(value_add(int1, int2)) == 3);
        CHECK(value_get_int32(value_add(int2, int0)) == 2);
    }
    
    SUBCASE("short strings") {
        CHECK(value_get_std_string(value_add(string_empty, string_space)) == " ");
        CHECK(value_get_std_string(value_add(string_hello, string_space)) == "hello ");
    }
    
    SUBCASE("long strings") {
        CHECK(value_get_std_string(value_add(string_long, string_long)) 
                == "this is a long stringthis is a long string");
    }
    
    SUBCASE("short and long strings") {
        CHECK(value_get_std_string(value_add(string_hello, string_long)) == "hellothis is a long string");
        CHECK(value_get_std_string(value_add(string_hello, value_add(string_space, string_hello))) == "hello hello");
    }
    
    SUBCASE("stringify in print") {
        CHECK(value_get_std_string(value_add(string_empty, int0)) == "0");
        CHECK(value_get_std_string(value_add(int1, string_hello)) == "1hello");
        CHECK(value_get_std_string(value_add(string_long, int2)) == "this is a long string2");
    }
}