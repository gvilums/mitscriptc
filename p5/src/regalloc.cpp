#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <queue>
#include <unordered_set>
#include <algorithm>

#include "doctest.h"

#include "AST.h"
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
    
auto LiveInterval::split_at(size_t pos) -> LiveInterval {
    LiveInterval result;
    result.vreg_id = this->vreg_id;
    std::vector<std::pair<size_t, size_t>> remaining;

    for (size_t rev_i = 0; rev_i < this->ranges.size(); ++rev_i) {
        size_t i = this->ranges.size() - rev_i;
        if (this->ranges[i].first < pos && this->ranges[i].second >= pos) {

        }
    }
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

auto LiveInterval::next_intersection(const LiveInterval &other) const -> std::optional<size_t> {
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

// should MUL and DIV be handled here?
void Function::set_fixed_machine_regs() {
    for (BasicBlock& block : this->blocks) {
        for (Instruction& instr : block.instructions) {
            if (instr.op == Operation::LOAD_ARG) {
                // convert LOAD_ARG to MOV
            } else if (instr.op == Operation::SET_ARG) {
            }
        }
    }
}

auto Function::compute_live_intervals() -> std::vector<LiveInterval> {
    std::vector<std::pair<size_t, size_t>> block_range;
    size_t current_from = 0;
    for (const BasicBlock& block : this->blocks) {
        // TODO check the -1
        block_range.push_back({current_from, current_from + 2 * block.instructions.size() + 1});
        current_from += 2 * block.instructions.size() + 2;
    }

    std::vector<LiveInterval> intervals;
    intervals.resize(this->virt_reg_count);

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
                    live.insert(instr.args[arg_id].index);
                }
            }
        }

        for (const PhiNode& phi : block.phi_nodes) {
            assert(phi.out.type == Operand::VIRT_REG);
            live.erase(phi.out.index);
        }

        if (block.is_loop_header) {
            // TODO
            for (size_t opd : live) {
                intervals[opd].push_loop_range({block_range[block_index].first, block_range[block.final_loop_block].second});
            }
        }

        block_live_regs[block_index] = std::move(live);
    }
    return intervals;
}

void debug_live_intervals(const std::vector<LiveInterval>& intervals) {
    for (size_t i = 0; i < intervals.size(); ++i) {
        std::cout << "Virtual Register " << i << '\n';
        for (auto [from, to] : intervals[i].ranges) {
            std::cout << from << " " << to << '\n';
        }
    }
}

void Function::allocate_registers() {
    // step 1: set fixed registers, generate moves around mul/div
    // step 2: compute live intervals
    // step 3: execute linear scan algorithm

    auto all_intervals = this->compute_live_intervals();
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, std::less<LiveInterval>> unhandled(
        std::make_move_iterator(all_intervals.begin()), std::make_move_iterator(all_intervals.end()));

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
            if (interval.ranges[0].second >= position) {
                assert(!interval.ranges.empty());
                if (interval.covers(position)) {
                    new_active.push_back(std::move(interval));
                } else {
                    new_inactive.push_back(std::move(interval));
                }
            }  // else handled
        }

        for (auto& interval : inactive) {
            assert(!interval.ranges.empty());
            if (interval.ranges[0].second >= position) {
                if (interval.covers(position)) {
                    new_active.push_back(std::move(interval));
                } else {
                    new_inactive.push_back(std::move(interval));
                }
            }  // else handled
        }

        active = std::move(new_active);
        inactive = std::move(new_inactive);

        // try allocate register
        // std::vector<size_t> free_until_pos;
        std::array<size_t, MACHINE_REG_COUNT> free_until_pos = {std::numeric_limits<size_t>::max()};

        for (const auto& interval : active) {
            assert(interval.reg != RegAssignment::UNINIT);
            free_until_pos[interval.assign_index] = 0;
        }
        
        for (const auto& interval : inactive) {
            if (auto intersection = current.next_intersection(interval)) {
                assert(interval.reg != RegAssignment::UNINIT);
                size_t& free_until = free_until_pos[interval.assign_index];
                free_until = std::min(free_until, *intersection);
            }
        }
        
        auto max_free = std::max_element(free_until_pos.begin(), free_until_pos.end());
        size_t max_free_index = std::distance(free_until_pos.begin(), max_free);
        
        if (*max_free > 0) {
            // register allocation succeeded
            if (current.ranges[0].second >= *max_free) {
                unhandled.push(current.split_at(*max_free));
            }
            current.reg = RegAssignment::MACHINE_REG;
            current.assign_index = max_free_index;
        } else {
            // register allocation failed
            std::array<size_t, MACHINE_REG_COUNT> next_use_pos = {std::numeric_limits<size_t>::max()};
            
            for (const auto& interval : active) {
                assert(!interval.ranges.empty());
                assert(interval.reg != RegAssignment::UNINIT);
                next_use_pos[interval.assign_index] = interval.next_use_after(current.ranges.back().first);
            }
            
            for (const auto& interval : inactive) {
                assert(!interval.ranges.empty());
                if (auto intersection = current.next_intersection(interval)) {
                    assert(interval.reg != RegAssignment::UNINIT);
                    size_t& next_use = next_use_pos[interval.assign_index];
                    next_use = std::min(next_use, *intersection);
                }
            }
            
            auto max_use = std::max_element(next_use_pos.begin(), next_use_pos.end());
            size_t max_use_index = std::distance(free_until_pos.begin(), max_use);
            
            if (current.use_locations.front() > *max_use) {
                current.reg = RegAssignment::STACK_SLOT;
                current.assign_index = stack_slot;
                stack_slot++;
                
                // TODO split current before first use pos requiring register
            } else {
                current.reg = RegAssignment::MACHINE_REG;
                current.assign_index = max_use_index;
                for (auto& interval : active) {
                    if (interval.reg == current.reg) {
                        unhandled.push(interval.split_at(position));
                        break;
                    }
                }
                for (auto& interval : inactive) {
                    if (interval.reg == current.reg) {
                        unhandled.push(interval.split_at(interval.next_alive_after(position)));
                    }
                }
                
                // TODO intersection check with fixed interval
            }
        }
        
        if (current.reg == RegAssignment::MACHINE_REG) {
            active.push_back(current);
        }
    } // while (!unhandled.empty())
    
    // process handled
}

};  // namespace IR

TEST_CASE("LiveInterval operator==") {
    CHECK(IR::LiveInterval{} == IR::LiveInterval{});    
}