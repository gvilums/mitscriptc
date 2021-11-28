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
#include <bitset>

#include "ir.h"
#include "irprinter.h"
#include "regalloc.h"

namespace std {

template <>
struct hash<::IR::LiveInterval> {
    size_t operator()(const ::IR::LiveInterval& interval) const {
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

template <>
struct hash<::IR::Operand> {
    size_t operator()(const ::IR::Operand& operand) const {
        std::hash<int> inthash;
        std::hash<size_t> sizethash;
        return inthash(static_cast<int>(operand.type)) ^ (sizethash(operand.index) << 1);
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
    if (pos >= this->ranges.back().second) {
        assert(false);
    }
    LiveInterval result;
    result.reg_id = this->reg_id;
    result.split_off = true;

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

auto IntervalBuilder::empty() -> bool {
    return this->ranges.empty();
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
    if (lhs.ranges.size() != rhs.ranges.size() ||
        lhs.use_locations.size() != lhs.use_locations.size()) {
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

std::array<MachineReg, 6> arg_regs{MachineReg::RDI, MachineReg::RSI, MachineReg::RDX,
                                   MachineReg::RCX, MachineReg::R8,  MachineReg::R9};

 std::array<MachineReg, 7> caller_save_regs{
     MachineReg::RDI,
     MachineReg::RSI,
     MachineReg::RDX,
     MachineReg::RCX,
     MachineReg::R8,
     MachineReg::R9,
     MachineReg::RAX,
//     MachineReg::R10, R10 is temporary register
//     MachineReg::R11, R11 is temporary register
 };

auto compute_machine_assignments(const Function& func) -> std::vector<LiveInterval> {
    std::vector<IntervalBuilder> builders;
    builders.resize(MACHINE_REG_COUNT);
    for (size_t i = 0; i < builders.size(); ++i) {
        builders[i].reg_id = i;
    }

    std::optional<size_t> next_call = std::nullopt;
    size_t arg_index = 0;
    size_t instr_id = 0;
    for (size_t i = 0; i < func.blocks.size(); ++i) {
        instr_id += 2;
        for (size_t j = 0; j < func.blocks[i].instructions.size(); ++j) {
            switch (func.blocks[i].instructions[j].op) {
                case Operation::ADD:
                case Operation::ALLOC_CLOSURE:
                case Operation::ALLOC_REC:
                case Operation::ALLOC_REF:
                case Operation::ALLOC_STRUCT:
                case Operation::REC_LOAD_NAME:
                case Operation::REC_LOAD_INDX:
                case Operation::REC_STORE_NAME:
                case Operation::REC_STORE_INDX:
                case Operation::EQ:
                case Operation::INPUT:
                case Operation::INTCAST:
                case Operation::PRINT:
                    // push ranges for instructions which call back into c++
                    for (const auto& reg : caller_save_regs) {
                        builders[static_cast<size_t>(reg)].push_range({instr_id, instr_id});
                    }
                    break;
                case Operation::LOAD_ARG:
                    // argument registers must be preserved from beginning to only point of use
                    for (size_t arg = 0; arg < std::min(func.parameter_count, 6); ++arg) {
                        builders[static_cast<size_t>(arg_regs[arg])].push_range({0, instr_id - 1});
                    }
                    break;
                case Operation::DIV:
                case Operation::MUL:
                    builders[static_cast<size_t>(MachineReg::RAX)].push_range({instr_id, instr_id});
                    builders[static_cast<size_t>(MachineReg::RDX)].push_range({instr_id, instr_id});
                    break;
                case Operation::SET_ARG:
                    // TODO arguments must survive until function call
                    if (!next_call.has_value()) {
                        size_t temp_instr_id = instr_id;
                        size_t k = j;
                        while (func.blocks[i].instructions[k].op != Operation::EXEC_CALL) {
                            ++k;
                            temp_instr_id += 2;
                        }
                        next_call = temp_instr_id;
                    }
                    arg_index = func.blocks[i].instructions[j].args[0].index;
                    if (arg_index < 6) {
                        builders[static_cast<size_t>(arg_regs[arg_index])].push_range(
                            {instr_id, *next_call - 1});
                    }
                    break;
                case Operation::EXEC_CALL:
                    // call invalidates all registers (no callee saved registers in this model)
                    for (auto& builder : builders) {
                        builder.push_range({instr_id, instr_id});
                    }
                    next_call = std::nullopt;
                    break;
                default:
                    break;
            }
            instr_id += 2;
        }
    }

    std::vector<LiveInterval> intervals;
    intervals.reserve(builders.size());
    for (auto& builder : builders) {
        intervals.push_back(builder.finish());
    }
    return intervals;
}

auto mapping_to_instructions(const std::vector<std::pair<Operand, Operand>>& mapping,
                             std::vector<Instruction>& instructions) {
    instructions.reserve(instructions.size() + mapping.size());

    // get vector of all operands, mapping index to op
    std::vector<Operand> operands;
    for (const auto& [from, to] : mapping) {
        if (std::find(operands.begin(), operands.end(), from) == operands.end()) {
            operands.push_back(from);
        }
        if (std::find(operands.begin(), operands.end(), to) == operands.end()) {
            operands.push_back(to);
        }
    }

    std::unordered_map<Operand, int> operand_indices;
    for (int i = 0; i < operands.size(); ++i) {
        operand_indices[operands[i]] = i;
    }

    // for each operand, compute its parent
    // -1 indicates no parent, operand is only input
    std::vector<int> parent(operands.size(), -1);
    // for each operand, indicates how many operands read its value
    std::vector<int> child_count(operands.size(), 0);
    for (const auto& [from, to] : mapping) {
        parent[operand_indices[to]] = operand_indices[from];
        child_count[operand_indices[from]] += 1;
    }


    bool updated = true;
    while (updated) {
        updated = false;
        for (int i = 0; i < operands.size(); ++i) {
            // operands without parent are irrelevant
            if (parent[i] == -1) {
                continue;
            }
            // if child_count == 0, update mapping
            if (child_count[i] == 0) {
                instructions.push_back(
                    Instruction{Operation::MOV, operands[i], operands[parent[i]]});
                child_count[parent[i]] -= 1;
                parent[i] = -1;
                updated = true;
            }
        }
    }

    // now only permuted registers remain
    std::vector<std::pair<Operand, Operand>> permuted;
    for (int i = 0; i < operands.size(); ++i) {
        if (parent[i] == -1) {
            continue;
        }
        if (child_count[parent[i]] > 0) {
            permuted.push_back({operands[parent[i]], operands[i]});
        }
    }

    // handle all permuted registers
    for (size_t i = 0; i < permuted.size(); ++i) {
        if (permuted[i].first != permuted[i].second) {
            size_t j = 0;
            while (permuted[j].first != permuted[i].second) {
                ++j;
                if (j == permuted.size()) {
                    assert(false);
                }
            }
            instructions.push_back(
                Instruction{Operation::SWAP, {}, permuted[i].first, permuted[j].first, {}});
            std::swap(permuted[i].second, permuted[j].second);
        }
    }
}

auto compute_live_intervals(const Function& func,
                            const std::vector<std::pair<size_t, size_t>>& block_ranges)
    -> std::vector<LiveInterval> {
    std::vector<IntervalBuilder> builders;
    builders.resize(func.virt_reg_count);
    for (size_t i = 0; i < builders.size(); ++i) {
        builders[i].reg_id = i;
    }

    std::vector<std::unordered_set<size_t>> block_live_regs;
    block_live_regs.resize(func.blocks.size());
    for (size_t i = 0; i < func.blocks.size(); ++i) {
        size_t block_index = func.blocks.size() - i - 1;
        const BasicBlock& block = func.blocks[block_index];
        std::unordered_set<size_t> live;
        for (size_t successor : block.successors) {
            live.merge(block_live_regs[successor]);
        }

        for (size_t successor : block.successors) {
            for (const PhiNode& phi : func.blocks[successor].phi_nodes) {
                for (const auto& [id, reg] : phi.args) {
                    if (id == block_index && reg.type == Operand::VIRT_REG) {
                        live.insert(reg.index);
                        builders[reg.index].use_locations.push_back(
                            block_ranges[block_index].second);
                        break;
                    }
                }
            }
        }

        for (size_t opd : live) {
            builders[opd].push_range(
                {block_ranges[block_index].first, block_ranges[block_index].second});
        }

        for (size_t inr = block.instructions.size(); inr > 0; --inr) {
            const Instruction& instr = block.instructions[inr - 1];
            // instruction id is shifted by 2, first instruction has index 2
            size_t instr_id = block_ranges[block_index].first + 2 * inr;

            if (instr.out.type == Operand::VIRT_REG) {
                // temporary debug assert
                // output operands
                builders[instr.out.index].shorten(instr_id + 1);
                live.erase(instr.out.index);
            }

            for (size_t arg_id = 0; arg_id < 3; ++arg_id) {
                if (instr.args[arg_id].type == Operand::VIRT_REG) {
                    builders[instr.args[arg_id].index].push_range(
                        {block_ranges[block_index].first, instr_id - 1});
                    builders[instr.args[arg_id].index].use_locations.push_back(instr_id - 1);
                    live.insert(instr.args[arg_id].index);
                }
            }
        }

        for (const PhiNode& phi : block.phi_nodes) {
            assert(phi.out.type == Operand::VIRT_REG);
            // phi outputs are defined to start living at one past the beginning of the block
            builders[phi.out.index].shorten(block_ranges[block_index].first + 1);
            live.erase(phi.out.index);
        }

        if (block.is_loop_header) {
            for (size_t opd : live) {
                builders[opd].push_range(
                    {block_ranges[block_index].first, block_ranges[block.final_loop_block].second});
                builders[opd].push_use(block_ranges[block.final_loop_block].second);
            }
        }

        block_live_regs[block_index] = std::move(live);
    }

    std::vector<LiveInterval> intervals;
    intervals.reserve(builders.size());
    for (auto& builder : builders) {
        if (!builder.empty()) {
            intervals.push_back(builder.finish());
        }
    }
    return intervals;
}

auto try_alloc_reg(
    LiveInterval& current,
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>&
        unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    const std::vector<LiveInterval>& machine_reg_uses) -> bool {
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
            if (*intersection == current.start_pos()) {
                free_until_pos[interval.reg_id] = 0;
            } else {
                size_t& free_until = free_until_pos[interval.reg_id];
                free_until = std::min(free_until, *intersection);
            }
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
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>&
        unhandled,
    std::vector<LiveInterval>& active,
    std::vector<LiveInterval>& inactive,
    size_t& stack_slot,
    const std::vector<LiveInterval>& machine_reg_uses) {
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

    for (const auto& interval : machine_reg_uses) {
        if (auto intersection = current.next_intersection(interval)) {
            // disallow allocation to blocked physical registers
            if (intersection == current.start_pos()) {
                next_use_pos[interval.reg_id] = 0;
            }
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

    if (current.first_use() >= max_use) {
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
                // TODO do we need the splitting rounding here? because we are currently in lifetime
                // hole
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

auto set_instr_machine_regs(const Instruction& instr,
                            size_t instr_id,
                            std::vector<IntervalGroup> groups) -> Instruction {
    Instruction changed{instr};
    for (auto& arg : changed.args) {
        if (arg.type == Operand::VIRT_REG) {
            arg = groups[arg.index].assignment_at(instr_id - 1).value();
        }
    }
    if (instr.out.type == Operand::VIRT_REG) {
        if (auto out_reg = groups[instr.out.index].assignment_at(instr_id + 1)) {
            changed.out = *out_reg;
        } else {
            changed.out = Operand{};
        }
    }
    if (instr.op == Operation::GC) {
        std::bitset<MACHINE_REG_COUNT> live_regs;
        for (const auto& group : groups) {
            if (auto assign = group.assignment_at(instr_id)) {
                if (assign->type == Operand::MACHINE_REG) {
                    live_regs.set(assign->index);
                }
            }
        }
        changed.args[0] = Operand{Operand::LOGICAL, (int)live_regs.to_ulong()};
    }
    return changed;
}

void generate_instr_mapping(const Instruction& instr, std::vector<std::pair<Operand, Operand>>& mapping) {
    switch (instr.op) {
        case Operation::ADD:
        case Operation::REC_LOAD_INDX:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RSI));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::RDX));
            break;
        case Operation::EQ:
        case Operation::REC_LOAD_NAME:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RDI));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::RSI));
            break;
        case Operation::MUL:
        case Operation::DIV:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RAX));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::R10));
            break;
        case Operation::ADD_INT:
        case Operation::SUB:
        case Operation::GT:
        case Operation::GEQ:
        case Operation::AND:
        case Operation::OR:
        case Operation::REF_STORE:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::R10));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::R11));
            break;
        case Operation::NOT:
        case Operation::REF_LOAD:
        case Operation::ASSERT_BOOL:
        case Operation::ASSERT_INT:
        case Operation::ASSERT_STRING:
        case Operation::ASSERT_RECORD:
        case Operation::ASSERT_CLOSURE:
        case Operation::ASSERT_NONZERO:
        case Operation::BRANCH:
        case Operation::STRUCT_LOAD:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::R10));
            break;
        case Operation::REC_STORE_NAME:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RDI));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::RSI));
            mapping.emplace_back(instr.args[2], Operand::from(MachineReg::RDX));
            break;
        case Operation::REC_STORE_INDX:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RSI));
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::RDX));
            mapping.emplace_back(instr.args[2], Operand::from(MachineReg::RCX));
            break;
        case Operation::SET_CAPTURE:
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::R10));
            mapping.emplace_back(instr.args[2], Operand::from(MachineReg::R11));
            break;
        case Operation::SET_ARG:
        case Operation::STORE_GLOBAL:
            mapping.emplace_back(instr.args[1], Operand::from(MachineReg::R10));
            break;
        case Operation::RETURN:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RAX));
            break;
        case Operation::PRINT:
        case Operation::INTCAST:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::RSI));
            break;
        case Operation::ALLOC_REF:
        case Operation::ALLOC_REC:
        case Operation::ALLOC_CLOSURE:
        case Operation::ALLOC_STRUCT:
        case Operation::EXEC_CALL:
        case Operation::INPUT:
        case Operation::SWAP:
        case Operation::INIT_CALL:
        case Operation::LOAD_ARG:
        case Operation::LOAD_FREE_REF:
        case Operation::MOV:
        case Operation::LOAD_GLOBAL:
        case Operation::GC:
            break;
        case Operation::STRUCT_STORE:
            mapping.emplace_back(instr.args[0], Operand::from(MachineReg::R10));
            mapping.emplace_back(instr.args[2], Operand::from(MachineReg::R11));
            break;
    }
}

//void insert_mapping(std::vector<Instruction>& instructions,
//                    const std::vector<std::pair<size_t, std::pair<Operand, Operand>>>& resolves,
//                    const Instruction& current_instr,
//                    size_t current_index) {
//}

void rewrite_instructions(Function& func,
                          const std::vector<IntervalGroup>& groups,
                          std::vector<std::pair<size_t, std::pair<Operand, Operand>>> resolves) {
    // current index in split resolve array
    size_t resolve_index = 0;

    size_t instr_id = 0;
    for (auto& block : func.blocks) {
        // initial block offset
        instr_id += 2;
        std::vector<Instruction> new_instructions;

        for (const auto& instr : block.instructions) {
            Instruction new_instr = set_instr_machine_regs(instr, instr_id, groups);

            std::vector<std::pair<Operand, Operand>> current_resolves;
            while (resolve_index < resolves.size() && resolves[resolve_index].first < instr_id) {
                current_resolves.push_back(resolves[resolve_index].second);
                ++resolve_index;
            }
            generate_instr_mapping(new_instr, current_resolves);

            mapping_to_instructions(current_resolves, new_instructions);

            new_instructions.push_back(new_instr);
            instr_id += 2;

        }

        // TODO check if the following is necessary
//        std::vector<std::pair<Operand, Operand>> edge_resolves;
//        while (resolve_index < resolves.size() && resolves[resolve_index].first < instr_id) {
//            edge_resolves.push_back(resolves[resolve_index].second);
//            ++resolve_index;
//        }
//        mapping_to_instructions(edge_resolves, new_instructions);


        block.phi_nodes.clear();
        block.instructions = std::move(new_instructions);
    }
}

void allocate_registers(Function& func) {
    // compute live intervals
    std::vector<std::pair<size_t, size_t>> block_ranges;
    size_t current_from = 0;
    for (const BasicBlock& block : func.blocks) {
        block_ranges.push_back({current_from, current_from + 2 * block.instructions.size() + 1});
        current_from += 2 * block.instructions.size() + 2;
    }

    auto intervals = compute_live_intervals(func, block_ranges);

//    std::cout << "----------" << std::endl;
//    for (const auto& interval : intervals) {
//         std::cout << interval << std::endl;
//     }

    auto machine_reg_uses = compute_machine_assignments(func);

//    std::cout << "-------- machine regs ---------" << std::endl;
//    for (const auto& interval : machine_reg_uses) {
//        std::cout << interval << std::endl;
//    }

//    for (const auto& interval : intervals) {
//        if (interval.empty()) {
//            std::cout << interval << std::endl;
//            assert(false);
//        }
//    }

    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::greater<LiveInterval>>
        unhandled(std::make_move_iterator(intervals.begin()),
                  std::make_move_iterator(intervals.end()));


    std::vector<LiveInterval> active;
    std::vector<LiveInterval> inactive;
    size_t stack_slot{0};

    std::vector<LiveInterval> handled;

    // vector holding positions and virtual register descriptors of registers split in middle of
    // lifetime
    std::vector<std::pair<size_t, Operand>> interval_splits;

    while (!unhandled.empty()) {
        LiveInterval current = unhandled.top();
        unhandled.pop();
        if (current.empty()) {
            continue;
        }
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
        if (!try_alloc_reg(current, unhandled, active, inactive, machine_reg_uses)) {
            alloc_blocked_reg(current, position, unhandled, active, inactive, stack_slot,
                              machine_reg_uses);
        }

        if (current.op.type == Operand::MACHINE_REG) {
            active.push_back(current);
        } else if (current.op.type == Operand::STACK_SLOT) {
            handled.push_back(current);
        } else {
            assert(false && "interval with invalid register assignment");
        }
    }  // while (!unhandled.empty())

    handled.insert(handled.end(), std::make_move_iterator(active.begin()),
                   std::make_move_iterator(active.end()));
    handled.insert(handled.end(), std::make_move_iterator(inactive.begin()),
                   std::make_move_iterator(inactive.end()));

    // for (auto& interval : handled) {
    //     std::cout << interval << std::endl;
    // }

    // group intervals by vreg_id
    std::vector<std::vector<LiveInterval>> intervals_by_vreg;
    intervals_by_vreg.resize(func.virt_reg_count);
    for (LiveInterval& interval : handled) {
        intervals_by_vreg[interval.reg_id].push_back(std::move(interval));
    }
    // sort intervals in ascending order (they are guaranteed to be disjoint)
    for (auto& group : intervals_by_vreg) {
        std::sort(group.begin(), group.end());
    }

    std::vector<IntervalGroup> interval_groups;
    interval_groups.reserve(intervals_by_vreg.size());
    for (auto& group : intervals_by_vreg) {
        interval_groups.push_back({std::move(group)});
    }

    // nonsuccessive interval split handling
    size_t initial_blocks = func.blocks.size();
    for (size_t pred = 0; pred < initial_blocks; ++pred) {
        // TODO CHECK THIS
        for (size_t i = 0;i < func.blocks[pred].successors.size(); ++i) {
            size_t succ = func.blocks[pred].successors[i];
            std::vector<std::pair<Operand, Operand>> resolve_moves;
            for (const auto& phi : func.blocks[succ].phi_nodes) {
                for (const auto& [phi_pred, operand] : phi.args) {
                    if (phi_pred != pred) {
                        continue;
                    }
                    Operand move_from;
                    if (operand.type == Operand::VIRT_REG) {
                        move_from = interval_groups[operand.index]
                                        .assignment_at(block_ranges[phi_pred].second)
                                        .value();
                    } else if (operand.type == Operand::IMMEDIATE) {
                        move_from = operand;
                    } else {
                        assert(false && "unsupported operand");
                    }
                    if (auto to = interval_groups[phi.out.index].assignment_at(
                            block_ranges[succ].first + 1)) {
                        Operand move_to = *to;
                        if (move_from != move_to) {
                            resolve_moves.push_back({move_from, move_to});
                        }
                    }
                }
            }
            for (const auto& group : interval_groups) {
                // check intervals that span multiple blocks
                if (auto move_to = group.assignment_at(block_ranges[succ].first)) {
                    Operand move_from = group.assignment_at(block_ranges[pred].second).value();
                    if (!(move_from == *move_to)) {
                        resolve_moves.push_back({move_from, *move_to});
                    }
                }
            }
            if (!resolve_moves.empty()) {
                mapping_to_instructions(resolve_moves, func.split_edge(pred, succ).instructions);
            }
        }
    }

    std::vector<std::pair<size_t, std::pair<Operand, Operand>>> resolve_moves;

    // successive interval split handling
    for (const auto& group : interval_groups) {
        if (group.intervals.size() < 2) {
            continue;
        }
        for (size_t i = 0; i < group.intervals.size() - 1; ++i) {
            // if two intervals are adjacent and are assigned to different registers, insert a move
            size_t prev_end = group.intervals[i].end_pos();
            size_t next_start = group.intervals[i + 1].start_pos();
            Operand prev_op = group.intervals[i].op;
            Operand next_op = group.intervals[i + 1].op;
            // TODO check in same block
            bool same_block = true;
            for (const auto& [begin, end] : block_ranges) {
                if (prev_end <= end && next_start > end) {
                    same_block = false;
                    break;
                }
            }
            if (same_block && prev_end + 1 == next_start && prev_op != next_op) {
                resolve_moves.push_back({prev_end, {prev_op, next_op}});
            }
        }
    }

    std::sort(resolve_moves.begin(), resolve_moves.end(),
              [](auto lhs, auto rhs) { return lhs.first < rhs.first; });

    rewrite_instructions(func, interval_groups, resolve_moves);
    func.stack_slots = stack_slot;
}

};  // namespace IR
