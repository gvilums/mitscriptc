#include <vector>
#include <array>
#include <queue>

#include "ir.h"

namespace IR {

void allocate_registers(Function& func);

void rewrite_instructions(
    Function& func, 
    std::vector<IntervalGroup> groups, 
    std::vector<std::pair<size_t, std::pair<Operand, Operand>>> splits
);

auto compute_live_intervals(
    const Function& func, 
    const std::vector<std::pair<size_t, size_t>> block_ranges
) -> std::vector<LiveInterval>;

auto compute_machine_assignments(
    const Function& func
) -> std::vector<LiveInterval>;

auto mapping_to_instructions(
    const std::vector<std::pair<Operand, Operand>>& mapping
) -> std::vector<Instruction>;

auto try_alloc_reg(
    LiveInterval& current,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>& unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    const std::vector<LiveInterval>& machine_reg_uses
) -> bool;

void alloc_blocked_reg(
    LiveInterval& current,
    const size_t& position,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>& unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    size_t& stack_slot,
    const std::vector<LiveInterval>& machine_reg_uses
);

};
