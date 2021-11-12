#include "doctest.h"

#include "ir.h"

TEST_CASE("LiveInterval operator==") {
    CHECK(IR::LiveInterval{} == IR::LiveInterval{});    
}

TEST_CASE("LiveInterval::split_at") {
    IR::LiveInterval interval;
    interval.push_range({10, 15});
    interval.push_range({0, 5});

    SUBCASE("split in first interval") {
        auto split_off = interval.split_at(2);
        REQUIRE(interval.ranges.size() == 1);
        REQUIRE(split_off.ranges.size() == 2);
        CHECK(interval.ranges.back() == std::pair<size_t, size_t>{0, 1});
        CHECK(split_off.ranges[1] == std::pair<size_t, size_t>{2, 5});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{10, 15});
    }
    SUBCASE("split in last interval") {
        auto split_off = interval.split_at(12);
        REQUIRE(interval.ranges.size() == 2);
        REQUIRE(split_off.ranges.size() == 1);
        CHECK(interval.ranges[1] == std::pair<size_t, size_t>{0, 5});
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{10, 11});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{12, 15});
    }
    SUBCASE("split between intervals") {
        auto split_off = interval.split_at(8);
        REQUIRE(interval.ranges.size() == 1);
        REQUIRE(split_off.ranges.size() == 1);
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{0, 5});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{10, 15});
    }
    SUBCASE("split in middle interval") {
        interval.ranges.insert(interval.ranges.begin(), {20, 25});    
        auto split_off = interval.split_at(12);
        REQUIRE(interval.ranges.size() == 2);
        REQUIRE(split_off.ranges.size() == 2);
        CHECK(interval.ranges[1] == std::pair<size_t, size_t>{0, 5});
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{10, 11});
        CHECK(split_off.ranges[1] == std::pair<size_t, size_t>{12, 15});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{20, 25});
    }
}