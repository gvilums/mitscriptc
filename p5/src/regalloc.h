#pragma once

#include <vector>
#include <array>
#include <queue>

#include "ir.h"

namespace IR {
    
const size_t MACHINE_REG_COUNT = 14;

enum class MachineReg : size_t {
    RAX,
    RCX,
    RDX,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    RBX,
};

struct LiveInterval {
    std::vector<std::pair<size_t, size_t>> ranges;
    std::vector<size_t> use_locations;

    size_t reg_id{0};

    Operand op{};
    bool split_off{false};
    
    size_t end_pos() const;
    size_t start_pos() const;

    bool covers(size_t position) const;
    auto next_intersection(const LiveInterval& other) const -> std::optional<size_t>;
    LiveInterval split_at(size_t pos);
    size_t next_alive_after(size_t pos) const;
    size_t next_use_after(size_t pos) const;
    size_t first_use() const;
    bool empty() const;

    friend bool operator<(const LiveInterval& lhs, const LiveInterval& rhs);
    friend bool operator>(const LiveInterval& lhs, const LiveInterval& rhs);
    friend bool operator==(const LiveInterval& lhs, const LiveInterval& rhs);
    // friend std::ostream& operator<<(std::ostream& os, const LiveInterval& interval);
    friend ::std::hash<LiveInterval>;
};

struct IntervalBuilder {
    std::vector<std::pair<size_t, size_t>> ranges;
    std::vector<size_t> use_locations;
    size_t reg_id{0};
    
    void push_range(std::pair<size_t, size_t> range);
    void push_use(size_t pos);
    void shorten(size_t new_begin);
    
    auto finish() -> LiveInterval;
};

struct IntervalGroup {
    std::vector<LiveInterval> intervals;

    bool begins_at(size_t pos) const;
    auto assignment_at(size_t pos) const -> std::optional<Operand>;
};



void allocate_registers(Function& func);

void rewrite_instructions(
    Function& func, 
    const std::vector<IntervalGroup>& groups, 
    std::vector<std::pair<size_t, std::pair<Operand, Operand>>> resolves
);

auto compute_live_intervals(
    const Function& func, 
    const std::vector<std::pair<size_t, size_t>>& block_ranges
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
