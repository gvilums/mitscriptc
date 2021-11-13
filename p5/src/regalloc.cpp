#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <queue>
#include <system_error>
#include <unordered_set>
#include <vector>
// #include <compare>

#include "ir.h"

namespace std {
template <>
struct hash<::IR::LiveInterval> {
    size_t operator()(const ::IR::LiveInterval& interval) {
        const size_t prime = 1000000007;
        size_t cprime = prime;
        size_t value = 0;
        for (auto [x, y] : interval.ranges) {
            value += x * cprime;
            cprime *= prime;
            value += y * cprime;
            cprime *= prime;
        }
        for (size_t x : interval.use_locations) {
            value += x * cprime;
            cprime *= prime;
        }
        return value;
    }
};
};  // namespace std

namespace IR {

using std::vector;
using std::pair;

std::string opr_str[] = {
    "NONE",
    "VIRT_REG",
    "IMMEDIATE",
    "LOGICAL",
    "MACHINE_REG",
    "STACK_SLOT"};

std::ostream& operator<<(std::ostream& os, const IR::Operand& opr) {
    os << "[" << opr_str[(int)opr.type] << " " << opr.index << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const IR::LiveInterval& interval) {
    os << "interval vreg " << interval.vreg_id << " {" << std::endl;
    os << "ranges: ";
    for (auto range = interval.ranges.rbegin(); range != interval.ranges.rend(); range++) {
        os << range->first << " " << range->second << ", ";
    }
    os << std::endl;
    os << "use locations: ";
    for (size_t loc : interval.use_locations) {
        os << loc << ", ";
    }
    os << std::endl;
    switch (interval.op.type) {
        case Operand::MACHINE_REG:
            os << "MACHINE_REG ";
            break;
        case Operand::STACK_SLOT:
            os << "STACK_SLOT ";
            break;
        default:
            os << "UNINIT ";
            break;
    }
    os << interval.op.index << std::endl;
    return os;
}

auto LiveInterval::split_at(size_t pos) -> LiveInterval {
    LiveInterval result;
    result.vreg_id = this->vreg_id;
    vector<pair<size_t, size_t>> remaining_ranges;
    vector<size_t> remaining_use_loc;

    for (size_t rev_i = 0; rev_i < this->ranges.size(); ++rev_i) {
        size_t i = this->ranges.size() - rev_i - 1;
        // split lands between ranges
        if (this->ranges[i].first >= pos) {
            // remaining ranges should move to result interval;
            for (size_t j = 0; j <= i; ++j) {
                result.ranges.push_back(this->ranges[j]);
            }
            break;
        }
        // inside range
        if (this->ranges[i].first < pos && this->ranges[i].second >= pos) {
            remaining_ranges.push_back({this->ranges[i].first, pos - 1});
            for (size_t j = 0; j < i; ++j) {
                result.ranges.push_back(this->ranges[j]);
            }
            result.ranges.push_back({pos, this->ranges[i].second});
            break;
        }
        remaining_ranges.push_back(this->ranges[i]);
    }

    for (size_t use_loc : this->use_locations) {
        if (use_loc < pos) {
            remaining_use_loc.push_back(use_loc);
        } else {
            result.use_locations.push_back(use_loc);
        }
    }

    std::reverse(remaining_ranges.begin(), remaining_ranges.end());
    this->ranges = std::move(remaining_ranges);
    this->use_locations = std::move(remaining_use_loc);
    return result;
}

void LiveInterval::push_range(std::pair<size_t, size_t> range) {
    auto [begin, end] = range;
    if (this->ranges.empty() || end + 1 < this->ranges.back().first) {
        this->ranges.push_back(range);
    }
    if (this->ranges.back().first > begin) {
        this->ranges.back().first = begin;
    }
}

void LiveInterval::push_loop_range(std::pair<size_t, size_t> range) {
    auto [begin, end] = range;
    if (this->ranges.empty()) {
        this->ranges.push_back(range);
    } else {
        while (!this->ranges.empty() && this->ranges.back().first <= end + 1) {
            if (this->ranges.back().second > end) {
                end = this->ranges.back().second;
            }
            this->ranges.pop_back();
        }
        this->ranges.push_back({begin, end});
    }
}

auto LiveInterval::next_intersection(const LiveInterval& other) const -> std::optional<size_t> {
    auto this_current = this->ranges.rbegin();
    auto other_current = other.ranges.rbegin();

    while (this_current != this->ranges.rend() && other_current != other.ranges.rend()) {
        if (this_current->second < other_current->first) {
            this_current++;
        } else if (other_current->second < this_current->first) {
            other_current++;
        } else if (this_current->first < other_current->first) {
            return other_current->first;
        } else if (other_current->first <= this_current->first) {
            return this_current->first;
        } else {
            assert(false && "invalid control flow at next_intersection");
        }
    }
    return std::nullopt;
}

bool LiveInterval::covers(size_t position) const {
    for (auto rg = this->ranges.rbegin(); rg != this->ranges.rend(); rg++) {
        if (rg->first <= position && rg->second >= position) {
            return true;
        }
    }
    return false;
}

size_t LiveInterval::next_use_after(size_t pos) const {
    for (size_t use_pos : this->use_locations) {
        // TODO check if >= or >
        if (use_pos >= pos) {
            return use_pos;
        }
    }
    assert(false && "invalid interval state, active after end");
}

size_t LiveInterval::next_alive_after(size_t pos) const {
    for (auto current = this->ranges.rbegin(); current != this->ranges.rend(); current++) {
        if (current->first >= pos) {
            return current->first;
        }
    }
    assert(false && "invalid interval state, inactive after end");
}

bool operator<(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.ranges.empty()) {
        return false;
    }
    if (rhs.ranges.empty()) {
        return true;
    }
    return lhs.ranges.back().first < rhs.ranges.back().first;
}

bool operator>(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.ranges.empty()) {
        return false;
    }
    if (rhs.ranges.empty()) {
        return true;
    }
    return lhs.ranges.back().first > rhs.ranges.back().first;
}

bool operator==(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.ranges.size() != rhs.ranges.size() || lhs.use_locations.size() != lhs.use_locations.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.ranges.size(); ++i) {
        if (lhs.ranges[i] != rhs.ranges[i]) {
            return false;
        }
    }
    for (size_t i = 0; i < lhs.use_locations.size(); ++i) {
        if (lhs.use_locations[i] != rhs.use_locations[i]) {
            return false;
        }
    }
    return true;
}

auto IntervalGroup::assignment_at(size_t pos) const -> std::optional<Operand> {
    // TODO binary search instead
    for (const auto& interval : this->intervals) {
        for (auto crange = interval.ranges.rbegin(); crange != interval.ranges.rend(); crange++) {
            if (pos < crange->first) {
                return std::nullopt;
            }
            if (pos >= crange->first && pos <= crange->second) {
                return interval.op;
            }
        }
    }
    return std::nullopt;
}

auto IntervalGroup::begins_at(size_t pos) const -> bool {
    return this->intervals.front().ranges.back().first == pos;
}

std::array<MachineRegs, 6> arg_regs{
    MachineRegs::RDI, 
    MachineRegs::RSI,
    MachineRegs::RDX,
    MachineRegs::RCX,
    MachineRegs::R8,
    MachineRegs::R9
};

std::array<MachineRegs, 9> caller_save_regs{
    MachineRegs::RDI, 
    MachineRegs::RSI,
    MachineRegs::RDX,
    MachineRegs::RCX,
    MachineRegs::R8,
    MachineRegs::R9,
    MachineRegs::RAX,
    MachineRegs::R10,
    MachineRegs::R11,
};

auto Function::compute_machine_assignments(
    const vector<pair<size_t, size_t>>& block_range
) -> vector<vector<pair<size_t, size_t>>> {
    vector<vector<pair<size_t, size_t>>> machine_intervals;
    machine_intervals.resize(MACHINE_REG_COUNT);

    for (size_t i = 0; i < this->blocks.size(); ++i) {
        for (size_t j = 0; j < this->blocks[i].instructions.size(); ++j) {
            size_t instr_id = block_range[i].first + 2 * (j + 1);
            switch (this->blocks[i].instructions[j].op) {
            case Operation::LOAD_ARG:
                // argument registers must be preserved from beginning to only point of use
                for (size_t arg = 0; arg < this->parameter_count; ++arg) {
                    machine_intervals[arg].push_back({0, instr_id - 1});
                }
                break;
            case Operation::DIV:
            case Operation::MUL:
                machine_intervals[static_cast<size_t>(MachineRegs::RAX)].push_back({instr_id, instr_id + 1});
                machine_intervals[static_cast<size_t>(MachineRegs::RDX)].push_back({instr_id, instr_id + 1});
                break;
            case Operation::CALL:
                // all caller saved registers must be flushed (worst case)
                // clobbers do not help, as we don't know what we're gonna call statically
                for (auto reg : caller_save_regs) {
                    machine_intervals[static_cast<size_t>(reg)].push_back({instr_id, instr_id + 1});
                }
                break;
            default:
                break;
            }
        }
    }
    return machine_intervals;
}

auto Function::compute_live_intervals(
    const vector<std::pair<size_t, size_t>>& block_range
) -> vector<LiveInterval> {
    vector<LiveInterval> intervals;
    intervals.resize(this->virt_reg_count);
    for (size_t i = 0; i < intervals.size(); ++i) {
        intervals[i].vreg_id = i;
    }

    std::vector<std::unordered_set<size_t>> block_live_regs;
    block_live_regs.resize(this->blocks.size());
    for (size_t i = 0; i < this->blocks.size(); ++i) {
        // debug_live_intervals(intervals);
        size_t block_index = this->blocks.size() - i - 1;
        const BasicBlock& block = this->blocks[block_index];
        std::unordered_set<size_t> live;
        for (size_t successor : block.successors) {
            live.merge(block_live_regs[successor]);
        }

        for (size_t successor : block.successors) {
            for (const PhiNode& phi : this->blocks[successor].phi_nodes) {
                for (const auto& [id, reg] : phi.args) {
                    if (id == block_index && reg.type == Operand::VIRT_REG) {
                        live.insert(reg.index);
                        // successor is loop header
                        intervals[reg.index].use_locations.push_back(block_range[block_index].second);
                        break;
                    }
                }
            }
        }

        for (size_t opd : live) {
            intervals[opd].push_range(block_range[block_index]);
        }

        for (size_t inr = block.instructions.size(); inr > 0; --inr) {
            const Instruction& instr = block.instructions[inr - 1];
            // instruction id is shifted by 2, first instruction has index 2
            size_t instr_id = block_range[block_index].first + 2 * inr;

            if (instr.out.type == Operand::VIRT_REG) {
                // temporary debug assert
                assert(intervals[instr.out.index].ranges.size() > 0);
                // output operands
                intervals[instr.out.index].ranges.back().first = instr_id;
                live.erase(instr.out.index);
            }

            for (size_t arg_id = 0; arg_id < 3; ++arg_id) {
                if (instr.args[arg_id].type == Operand::VIRT_REG) {
                    intervals[instr.args[arg_id].index].push_range({block_range[block_index].first, instr_id - 1});
                    intervals[instr.args[arg_id].index].use_locations.push_back(instr_id - 1);
                    live.insert(instr.args[arg_id].index);
                }
            }
        }

        for (const PhiNode& phi : block.phi_nodes) {
            assert(phi.out.type == Operand::VIRT_REG);
            live.erase(phi.out.index);
        }

        if (block.is_loop_header) {
            for (size_t opd : live) {
                intervals[opd].push_loop_range({block_range[block_index].first, block_range[block.final_loop_block].second});
            }
        }

        block_live_regs[block_index] = std::move(live);
    }
    for (auto& interval : intervals) {
        std::sort(interval.use_locations.begin(), interval.use_locations.end());
        std::cout << interval << std::endl;
    }
    return intervals;
}

auto try_alloc_reg(
    LiveInterval& current,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>& unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive) -> bool {
    std::array<size_t, MACHINE_REG_COUNT> free_until_pos;
    free_until_pos.fill(std::numeric_limits<size_t>::max());

    for (const auto& interval : active) {
        assert(interval.op.type != Operand::NONE);
        free_until_pos[interval.op.index] = 0;
    }

    // TODO omit intersection test when possible
    for (const auto& interval : inactive) {
        if (auto intersection = current.next_intersection(interval)) {
            assert(interval.op.type != Operand::NONE);
            size_t& free_until = free_until_pos[interval.op.index];
            free_until = std::min(free_until, *intersection);
        }
    }

    size_t max_free = free_until_pos[0];
    size_t max_free_index = 0;
    for (size_t i = 0; i < free_until_pos.size(); ++i) {
        if (free_until_pos[i] > max_free) {
            max_free = free_until_pos[i];
            max_free_index = i;
        }
    }

    // auto max_free = std::max_element(free_until_pos.begin(), free_until_pos.end());
    // size_t max_free_index = std::distance(free_until_pos.begin(), max_free);
    if (max_free > 0) {
        // register allocation succeeded
        if (current.ranges[0].second >= max_free) {
            unhandled.push(current.split_at(max_free));
        }
        current.op.type = Operand::MACHINE_REG;
        current.op.index = max_free_index;
        return true;
    }
    return false;
}

void alloc_blocked_reg(
    LiveInterval& current,
    const size_t& position,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>& unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    size_t& stack_slot) {
    std::array<size_t, MACHINE_REG_COUNT> next_use_pos;
    next_use_pos.fill(std::numeric_limits<size_t>::max());

    for (const auto& interval : active) {
        assert(!interval.ranges.empty());
        assert(interval.op.type == Operand::MACHINE_REG);
        next_use_pos[interval.op.index] = interval.next_use_after(current.ranges.back().first);
    }

    // TODO omit intersection test when possible
    for (const auto& interval : inactive) {
        assert(!interval.ranges.empty());
        if (auto intersection = current.next_intersection(interval)) {
            assert(interval.op.type == Operand::MACHINE_REG);
            size_t& next_use = next_use_pos[interval.op.index];
            next_use = std::min(next_use, *intersection);
        }
    }

    auto max_use = std::max_element(next_use_pos.begin(), next_use_pos.end());
    size_t max_use_index = std::distance(next_use_pos.begin(), max_use);

    if (current.use_locations.front() > *max_use) {
        current.op.type = Operand::STACK_SLOT;
        current.op.index = stack_slot;
        stack_slot++;

        // TODO split current before first use pos requiring register
    } else {
        current.op.type = Operand::MACHINE_REG;
        current.op.index = max_use_index;
        for (auto& interval : active) {
            assert(interval.op.type == Operand::MACHINE_REG);
            if (interval.op.index == current.op.index) {
                unhandled.push(interval.split_at(position));
                break;
            }
        }
        for (auto& interval : inactive) {
            assert(interval.op.type == Operand::MACHINE_REG);
            if (interval.op.index == current.op.index) {
                unhandled.push(interval.split_at(interval.next_alive_after(position)));
            }
        }
    }
    // TODO intersection check with fixed interval
}

auto Function::allocate_registers() -> std::vector<LiveInterval> {
    // compute live intervals
    std::vector<std::pair<size_t, size_t>> block_range;
    size_t current_from = 0;
    for (const BasicBlock& block : this->blocks) {
        block_range.push_back({current_from, current_from + 2 * block.instructions.size() + 1});
        current_from += 2 * block.instructions.size() + 2;
    }

    auto intervals = this->compute_live_intervals(block_range);
    auto machine_reg_uses = this->compute_machine_assignments(block_range);

    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>> unhandled(
        std::make_move_iterator(intervals.begin()), std::make_move_iterator(intervals.end()));

    std::vector<LiveInterval> active;
    std::vector<LiveInterval> inactive;
    size_t stack_slot{0};

    std::vector<LiveInterval> handled;

    while (!unhandled.empty()) {
        LiveInterval current = unhandled.top();
        unhandled.pop();
        size_t position = current.ranges.back().first;

        std::vector<LiveInterval> new_active;
        std::vector<LiveInterval> new_inactive;

        for (auto& interval : active) {
            assert(!interval.ranges.empty());
            if (interval.ranges[0].second < position) {
                handled.push_back(std::move(interval));
            } else if (interval.covers(position)) {
                new_active.push_back(std::move(interval));
            } else {
                new_inactive.push_back(std::move(interval));
            }
        }

        for (auto& interval : inactive) {
            assert(!interval.ranges.empty());
            if (interval.ranges[0].second < position) {
                handled.push_back(std::move(interval));
            } else if (interval.covers(position)) {
                new_active.push_back(std::move(interval));
            } else {
                new_inactive.push_back(std::move(interval));
            }
        }

        active = std::move(new_active);
        inactive = std::move(new_inactive);

        // try allocate register
        if (!try_alloc_reg(current, unhandled, active, inactive)) {
            alloc_blocked_reg(current, position, unhandled, active, inactive, stack_slot);
            // TODO check intersection with fixed registers
        }

        if (current.op.type == Operand::MACHINE_REG) {
            active.push_back(current);
        } else if (current.op.type == Operand::STACK_SLOT) {
            handled.push_back(current);
        } else {
            assert(false && "interval with invalid register assignment");
        }
    }  // while (!unhandled.empty())

    // TODO think about this
    handled.insert(handled.end(),
                   std::make_move_iterator(active.begin()), std::make_move_iterator(active.end()));
    handled.insert(handled.end(),
                   std::make_move_iterator(inactive.begin()), std::make_move_iterator(inactive.end()));

    for (auto& interval : handled) {
        std::cout << interval << std::endl;
    }

    // group intervals by vreg_id
    std::vector<std::vector<LiveInterval>> intervals_by_vreg;
    intervals_by_vreg.resize(this->virt_reg_count);
    for (LiveInterval& interval : handled) {
        intervals_by_vreg[interval.vreg_id].push_back(std::move(interval));
    }
    for (auto& group : intervals_by_vreg) {
        std::sort(group.begin(), group.end());
    }

    std::vector<IntervalGroup> interval_groups;
    for (auto& group : intervals_by_vreg) {
        interval_groups.emplace_back(std::move(group));
    }

    // resolution
    for (size_t pred = 0; pred < this->blocks.size(); ++pred) {
        // iterate over successors
        for (size_t succ : this->blocks[pred].successors) {
            // for every interval group
            std::cout << "predecessor " << pred << " successor " << succ << std::endl;
            for (const auto& group : interval_groups) {
                // if the corresponding vreg is live at the beginning of the successor
                if (auto move_to = group.assignment_at(block_range[succ].first)) {
                    Operand move_from;
                    // if the vreg is *defined* at the beginning of successor (through a phi function)
                    if (group.begins_at(block_range[succ].first)) {
                        Operand defining_op;
                        for (const auto& phi : this->blocks[succ].phi_nodes) {
                            for (const auto& [block_index, operand] : phi.args) {
                                if (block_index == pred) {
                                    defining_op = operand;
                                    break;
                                }
                            }
                        }
                        assert(defining_op.type != Operand::NONE);
                        if (defining_op.type == Operand::VIRT_REG) {
                            move_from = interval_groups[defining_op.index].assignment_at(block_range[pred].second).value();
                        } else if (defining_op.type == Operand::IMMEDIATE) {
                            move_from = defining_op;
                        } else {
                            assert(false && "unsupported operand");
                        }
                    } else {  // if the vreg continues living from a previous block
                        move_from = group.assignment_at(block_range[pred].second).value();
                    }
                    if (!(move_from == *move_to)) {
                        this->blocks[pred].resolution_map.push_back({move_from, *move_to});
                    }
                }
            }
        }
    }

    std::cout << "printing resolution" << std::endl;
    for (size_t i = 0; i < this->blocks.size(); ++i) {
        std::cout << "block " << i << std::endl;
        for (const auto& [from, to] : this->blocks[i].resolution_map) {
            std::cout << from << " -> " << to << std::endl;
        }
    }
    // instruction rewriting

    return handled;
}

};  // namespace IR
