#include "doctest.h"

#include "ir.h"
#include "regalloc.h"

TEST_CASE("LiveInterval operator==") {
    CHECK(IR::LiveInterval{} == IR::LiveInterval{});    
}

TEST_CASE("LiveInterval::split_at") {
    IR::IntervalBuilder builder;
    builder.push_range({10, 15});
    builder.push_range({0, 5});

    SUBCASE("split in first interval") {
        auto interval = builder.finish();
        auto split_off = interval.split_at(2);
        REQUIRE(interval.ranges.size() == 1);
        REQUIRE(split_off.ranges.size() == 2);
        std::cout << interval.ranges[0].first << " " << interval.ranges[0].second << std::endl;
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{0, 1});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{2, 5});
        CHECK(split_off.ranges[1] == std::pair<size_t, size_t>{10, 15});
    }
    SUBCASE("split in last interval") {
        auto interval = builder.finish();
        auto split_off = interval.split_at(12);
        REQUIRE(interval.ranges.size() == 2);
        REQUIRE(split_off.ranges.size() == 1);
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{0, 5});
        CHECK(interval.ranges[1] == std::pair<size_t, size_t>{10, 11});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{12, 15});
    }
    SUBCASE("split between intervals") {
        auto interval = builder.finish();
        auto split_off = interval.split_at(8);
        REQUIRE(interval.ranges.size() == 1);
        REQUIRE(split_off.ranges.size() == 1);
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{0, 5});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{10, 15});
    }
    SUBCASE("split in middle interval") {
        builder.push_range({20, 25});
        auto interval = builder.finish();
        auto split_off = interval.split_at(12);
        REQUIRE(interval.ranges.size() == 2);
        REQUIRE(split_off.ranges.size() == 2);
        CHECK(interval.ranges[0] == std::pair<size_t, size_t>{0, 5});
        CHECK(interval.ranges[1] == std::pair<size_t, size_t>{10, 11});
        CHECK(split_off.ranges[0] == std::pair<size_t, size_t>{12, 15});
        CHECK(split_off.ranges[1] == std::pair<size_t, size_t>{20, 25});
    }
}

bool check_mapping(std::vector<std::pair<IR::Operand, IR::Operand>>& mapping, std::vector<IR::Instruction>& instrs) {
    return false;
}

TEST_CASE("move and swap ordering") {
    using namespace IR;
    SUBCASE("mapping without intersections") {
        std::vector<std::pair<Operand, Operand>> mapping{
            {{Operand::MACHINE_REG, 0}, {Operand::MACHINE_REG, 1}},
            {{Operand::MACHINE_REG, 2}, {Operand::MACHINE_REG, 3}},
        };
        auto instructions = mapping_to_instructions(mapping);
        
    }
}