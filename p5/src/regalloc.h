#include <vector>
#include <array>

#include "ir.h"

namespace IR {

class RegisterAllocator {
    std::vector<std::pair<size_t, size_t>> block_ranges;
    std::array<LiveInterval, MACHINE_REG_COUNT> machine_reg_intervals{};

public:
    RegisterAllocator(const Program& program);
    void transform(Program& program);
};

};
