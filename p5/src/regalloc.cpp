#include <cassert>
#include <unordered_set>
#include <iostream>

#include "ir.h"

namespace IR {
    
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

bool operator<(const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.ranges.empty()) {
        return false;
    }
    if (rhs.ranges.empty()) {
        return true;
    }
    return lhs.ranges.back().first < rhs.ranges.back().first;
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
    for (size_t i = 0 ; i < intervals.size(); ++i) {
        std::cout << "Virtual Register " << i << '\n';
        for (auto [from, to] : intervals[i].ranges) {
            std::cout << from << " " << to << '\n';
        }
    }
}

void Function::allocate_registers() {
    // step 1: set fixed registers, generate moves around mul/div
    // step 2: 
    
    auto all_intervals = this->compute_live_intervals();
    
}

};