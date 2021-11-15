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
#include "irprinter.h"
#include "regalloc.h"

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


bool LiveInterval::empty() const {
    return this->ranges.empty();
}

size_t LiveInterval::start_pos() const {
    assert(!this->empty());
    return this->ranges.front().first;
}

size_t LiveInterval::end_pos() const {
    assert(!this->empty());
    return this->ranges.back().second;
}

size_t LiveInterval::first_use() const {
    assert(!this->use_locations.empty());
    return this->use_locations.front();
}

auto LiveInterval::split_at(size_t pos) -> LiveInterval {
    LiveInterval result;
    result.reg_id = this->reg_id;
    
    for (size_t i = 0; i < this->ranges.size(); ++i) {
        auto [begin, end] = this->ranges[i];  
        if (begin >= pos) {
            for (size_t j = i; j < this->ranges.size(); ++j) {
                result.ranges.push_back(this->ranges[j]);
            }
            this->ranges.resize(i);
            break;
        }
        if (begin < pos && pos <= end) {
            this->ranges[i] = {begin, pos - 1};
            result.ranges.push_back({pos, end});
            for (size_t j = i + 1; j < this->ranges.size(); ++j) {
                result.ranges.push_back(this->ranges[j]);
            }
            this->ranges.resize(i + 1);
        }
    }
    
    size_t remaining_use_locs = 0;
    for (size_t use_loc : this->use_locations) {
        if (use_loc >= pos) {
            result.use_locations.push_back(use_loc);
        } else {
            remaining_use_locs++;
        }
    }
    this->use_locations.resize(remaining_use_locs);
    return result;
}

void IntervalBuilder::push_range(std::pair<size_t, size_t> range) {
    this->ranges.push_back(range);
}

void IntervalBuilder::push_use(size_t pos) {
    this->use_locations.push_back(pos);
}

void IntervalBuilder::shorten(size_t new_begin) {
    for (auto& [begin, end] : this->ranges) {
        if (begin < new_begin) {
            begin = new_begin;
        }
    }
}

auto IntervalBuilder::finish() -> LiveInterval {
    std::sort(this->ranges.begin(), this->ranges.end());
    std::sort(this->use_locations.begin(), this->use_locations.end());
    std::vector<std::pair<size_t, size_t>> merged_ranges;
    for (auto range : this->ranges) {
        if (merged_ranges.empty() || range.first > merged_ranges.back().second + 1) {
            merged_ranges.push_back(range);
        }
        if (merged_ranges.back().second < range.second) {
            merged_ranges.back().second = range.second;
        }
    }
    return LiveInterval{std::move(merged_ranges), std::move(this->use_locations), this->reg_id};
}

auto LiveInterval::next_intersection(const LiveInterval& other) const -> std::optional<size_t> {
    auto this_current = this->ranges.begin();
    auto other_current = other.ranges.begin();

    while (this_current != this->ranges.end() && other_current != other.ranges.end()) {
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
    for (auto [begin, end] : this->ranges) {
        if (position < begin) {
            return false;
        }
        if (begin <= position && position <= end) {
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
    for (auto [begin, end] : this->ranges) {
        if (begin >= pos) {
            return begin;
        }
    }
    assert(false && "invalid interval state, inactive after end");
}

bool operator<(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.empty()) {
        return false;
    }
    if (rhs.empty()) {
        return true;
    }
    return lhs.start_pos() < rhs.start_pos();
}

bool operator>(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.empty()) {
        return false;
    }
    if (rhs.empty()) {
        return true;
    }
    return lhs.start_pos() > rhs.start_pos();
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
        for (auto [begin, end] : interval.ranges) {
            if (pos < begin) {
                return std::nullopt;
            }
            if (pos >= begin && pos <= end) {
                return interval.op;
            }
        }
    }
    return std::nullopt;
}

auto IntervalGroup::begins_at(size_t pos) const -> bool {
    return this->intervals.front().start_pos() == pos;
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

auto Function::compute_machine_assignments() -> std::vector<LiveInterval> {
    std::vector<IntervalBuilder> builders;
    builders.resize(MACHINE_REG_COUNT);
    for (size_t i = 0; i < builders.size(); ++i) {
        builders[i].reg_id = i;
    }

    std::optional<size_t> next_call = std::nullopt;
    size_t arg_index = 0;
    size_t instr_id = 0;
    for (size_t i = 0; i < this->blocks.size(); ++i) {
        instr_id += 2;
        for (size_t j = 0; j < this->blocks[i].instructions.size(); ++j) {
            switch (this->blocks[i].instructions[j].op) {
            case Operation::LOAD_ARG:
                // argument registers must be preserved from beginning to only point of use
                for (size_t arg = 0; arg < std::min(this->parameter_count, 6UL); ++arg) {
                    builders[static_cast<size_t>(arg_regs[arg])].push_range({0, instr_id - 1});
                }
                break;
            case Operation::DIV:
            case Operation::MUL:
                builders[static_cast<size_t>(MachineRegs::RAX)].push_range({instr_id, instr_id});
                builders[static_cast<size_t>(MachineRegs::RDX)].push_range({instr_id, instr_id});
                break;
            case Operation::SET_ARG:
                // TODO arguments must survive until function call
                if (!next_call.has_value()) {
                    size_t temp_instr_id = instr_id;
                    size_t k = j;
                    while (this->blocks[i].instructions[k].op != Operation::CALL) {
                        ++k;
                        temp_instr_id += 2;
                    }
                    next_call = temp_instr_id;
                }
                arg_index = this->blocks[i].instructions[j].args[0].index;
                if (arg_index < 6) {
                    builders[static_cast<size_t>(arg_regs[arg_index])].push_range({instr_id, *next_call - 1});
                }
                break;
            case Operation::CALL:
                // all caller saved registers must be flushed (worst case)
                // clobbers do not help, as we don't know what we're gonna call statically
                for (auto reg : caller_save_regs) {
                    builders[static_cast<size_t>(reg)].push_range({instr_id, instr_id + 1});
                }
                break;
            default:
                break;
            }
            instr_id += 2;
        }
    }

    std::vector<LiveInterval> intervals;
    for (auto& builder : builders) {
        intervals.push_back(builder.finish());
    }
    return intervals;
}

auto Function::compute_live_intervals(
    const std::vector<std::pair<size_t, size_t>>& block_range
) -> std::vector<LiveInterval> {
    std::vector<IntervalBuilder> builders;
    builders.resize(this->virt_reg_count);
    for (size_t i = 0; i < builders.size(); ++i) {
        builders[i].reg_id = i;
    }

    std::vector<std::unordered_set<size_t>> block_live_regs;
    block_live_regs.resize(this->blocks.size());
    for (size_t i = 0; i < this->blocks.size(); ++i) {
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
                        builders[reg.index].use_locations.push_back(block_range[block_index].second);
                        break;
                    }
                }
            }
        }

        for (size_t opd : live) {
            builders[opd].push_range({block_range[block_index].first, block_range[block_index].second});
        }

        for (size_t inr = block.instructions.size(); inr > 0; --inr) {
            const Instruction& instr = block.instructions[inr - 1];
            // instruction id is shifted by 2, first instruction has index 2
            size_t instr_id = block_range[block_index].first + 2 * inr;

            if (instr.out.type == Operand::VIRT_REG) {
                // temporary debug assert
                // output operands
                builders[instr.out.index].shorten(instr_id + 1);
                live.erase(instr.out.index);
            }

            for (size_t arg_id = 0; arg_id < 3; ++arg_id) {
                if (instr.args[arg_id].type == Operand::VIRT_REG) {
                    builders[instr.args[arg_id].index].push_range({block_range[block_index].first, instr_id - 1});
                    builders[instr.args[arg_id].index].use_locations.push_back(instr_id - 1);
                    live.insert(instr.args[arg_id].index);
                }
            }
        }

        for (const PhiNode& phi : block.phi_nodes) {
            assert(phi.out.type == Operand::VIRT_REG);
            // phi outputs are defined to start living at one past the beginning of the block
            builders[phi.out.index].shorten(block_range[block_index].first + 1);
            live.erase(phi.out.index);
        }

        if (block.is_loop_header) {
            for (size_t opd : live) {
                builders[opd].push_range({block_range[block_index].first, block_range[block.final_loop_block].second});
            }
        }
        
        block_live_regs[block_index] = std::move(live);
    }

    std::vector<LiveInterval> intervals;
    for (auto& builder : builders) {
        intervals.push_back(builder.finish());
    }
    return intervals;
}

auto try_alloc_reg(
    LiveInterval& current,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>& unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    const std::vector<LiveInterval>& machine_reg_uses,
    std::vector<std::pair<size_t, Operand>>& interval_splits) -> bool {
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
    
    for (const auto& interval : machine_reg_uses) {
        if (auto intersection = current.next_intersection(interval)) {
            size_t& free_until = free_until_pos[interval.reg_id];
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

    if (max_free > 0) {
        // register allocation succeeded
        if (current.end_pos() >= max_free) {
            // split should happen *before* an instruction
            size_t split_pos = (max_free >> 1) << 1;
            unhandled.push(current.split_at(split_pos));
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
    size_t& stack_slot,
    const std::vector<LiveInterval>& machine_reg_uses,
    std::vector<std::pair<size_t, Operand>>& interval_splits) {
    std::array<size_t, MACHINE_REG_COUNT> next_use_pos;
    next_use_pos.fill(std::numeric_limits<size_t>::max());

    for (const auto& interval : active) {
        assert(!interval.empty());
        assert(interval.op.type == Operand::MACHINE_REG);
        next_use_pos[interval.op.index] = interval.next_use_after(current.start_pos());
    }

    // TODO omit intersection test when possible
    for (const auto& interval : inactive) {
        assert(!interval.empty());
        if (auto intersection = current.next_intersection(interval)) {
            assert(interval.op.type == Operand::MACHINE_REG);
            size_t& next_use = next_use_pos[interval.op.index];
            next_use = std::min(next_use, *intersection);
        }
    }

    size_t max_use = next_use_pos[0];
    size_t max_use_index = 0;
    for (size_t i = 0; i < next_use_pos.size(); ++i) {
        if (next_use_pos[i] > max_use) {
            max_use = next_use_pos[i];
            max_use_index = i;
        }
    }

    if (current.first_use() > max_use) {
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
                size_t split_pos = (position >> 1) << 1;
                unhandled.push(interval.split_at(split_pos));
                break;
            }
        }
        for (auto& interval : inactive) {
            assert(interval.op.type == Operand::MACHINE_REG);
            if (interval.op.index == current.op.index) {
                // TODO do we need the splitting rounding here? because we are currently in lifetime hole
                unhandled.push(interval.split_at(interval.next_alive_after(position)));
            }
        }
        // intersection check with fixed interval        
        if (auto pos = current.next_intersection(machine_reg_uses[current.op.index])) {
            size_t split_pos = (*pos >> 1) << 1;
            unhandled.push(current.split_at(split_pos));
        }
    }
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
    // for (const auto& interval : intervals) {
    //     std::cout << interval << std::endl;
    // }
    // std::cout << "------------------" << std::endl;

    auto machine_reg_uses = this->compute_machine_assignments();
    // std::cout << "machine intervals" << std::endl;
    // for (const auto& interval : machine_reg_uses) {
    //     std::cout << interval << std::endl;
    // }
    // std::cout << "-----------------" << std::endl;

    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>> unhandled(
        std::make_move_iterator(intervals.begin()), std::make_move_iterator(intervals.end()));

    std::vector<LiveInterval> active;
    std::vector<LiveInterval> inactive;
    size_t stack_slot{0};

    std::vector<LiveInterval> handled;
    
    // vector holding positions and virtual register descriptors of registers split in middle of lifetime
    std::vector<std::pair<size_t, Operand>> interval_splits;

    while (!unhandled.empty()) {
        LiveInterval current = unhandled.top();
        unhandled.pop();
        size_t position = current.start_pos();

        std::vector<LiveInterval> new_active;
        std::vector<LiveInterval> new_inactive;

        for (auto& interval : active) {
            assert(!interval.empty());
            if (interval.end_pos() < position) {
                handled.push_back(std::move(interval));
            } else if (interval.covers(position)) {
                new_active.push_back(std::move(interval));
            } else {
                new_inactive.push_back(std::move(interval));
            }
        }

        for (auto& interval : inactive) {
            assert(!interval.empty());
            if (interval.end_pos() < position) {
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
        if (!try_alloc_reg(current, unhandled, active, inactive, machine_reg_uses, interval_splits)) {
            alloc_blocked_reg(current, position, unhandled, active, inactive, stack_slot, machine_reg_uses, interval_splits);
        }

        if (current.op.type == Operand::MACHINE_REG) {
            active.push_back(current);
        } else if (current.op.type == Operand::STACK_SLOT) {
            handled.push_back(current);
        } else {
            assert(false && "interval with invalid register assignment");
        }
    }  // while (!unhandled.empty())

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
        intervals_by_vreg[interval.reg_id].push_back(std::move(interval));
    }
    // sort intervals in ascending order (they are guaranteed to be disjoint)
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
            // handle phi nodes
            for (const auto& phi : this->blocks[succ].phi_nodes) {
                for (const auto& [block_index, operand] : phi.args) {
                    if (block_index == pred) {
                        // phi function defining some value in successor
                        Operand move_from;
                        if (operand.type == Operand::VIRT_REG) {
                            move_from = interval_groups[operand.index].assignment_at(block_range[pred].second).value();
                        } else if (operand.type == Operand::IMMEDIATE) {
                            move_from = operand;
                        } else {
                            assert(false && "unsupported operand");
                        }
                        Operand move_to = interval_groups[phi.out.index].assignment_at(block_range[succ].first + 1).value();
                        if (!(move_from == move_to)) {
                            this->blocks[pred].resolution_map.push_back({move_from, move_to});
                        }
                    }
                }
            }
            // for every interval group
            for (const auto& group : interval_groups) {
                // check intervals that span multiple blocks
                if (auto move_to = group.assignment_at(block_range[succ].first)) {
                    Operand move_from = group.assignment_at(block_range[pred].second).value();
                    if (!(move_from == *move_to)) {
                        this->blocks[pred].resolution_map.push_back({move_from, *move_to});
                    }
                }
            }
        }
    }
    
    // check adjacent split intervals
    std::vector<std::pair<size_t, std::pair<Operand, Operand>>> split_resolves;
    for (const auto& group : interval_groups) {
        for (size_t i = 0; i < group.intervals.size() - 1; ++i) {
            // if two intervals are adjacent and are assigned to different registers, insert a move
            size_t prev_end = group.intervals[i].end_pos();
            size_t next_start = group.intervals[i + 1].start_pos();
            Operand prev_op = group.intervals[i].op;
            Operand next_op = group.intervals[i + 1].op;
            if (prev_end + 1 == next_start && !(prev_op == next_op)) {
                split_resolves.push_back({group.intervals[i + 1].start_pos(), {prev_op, next_op}});
            }
        }
    }

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
