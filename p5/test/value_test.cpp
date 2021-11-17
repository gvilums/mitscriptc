#include <iostream>

#include "doctest.h"

#include "value.h"

TEST_CASE("use as integer") {
    using namespace runtime;
    Value val = from_int32(20);
    Value val2 = from_int32(30);
    Value val3 = value_sub(val, val2);
    CHECK(value_get_int32(val3) == -10);
    
    Value val4 = from_int32(5);
    Value val5 = value_mul(val3, val4);
    CHECK(value_get_int32(val5) == -50);
}