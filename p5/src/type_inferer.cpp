#include "value.h"
#include "ir.h"
#include "type_inferer.h"
#include <set>

TypeInferer::TypeInferer(IR::Program* prog) : prog_(prog){}

IR::Program* TypeInferer::optimize() {
    infer_type();
    return prog_;
}

// IR::Operand get_type

void TypeInferer::infer_type() {
    for (auto& fun : prog_->functions) {
        std::unordered_map<size_t, runtime::ValueType> type_var;
        for (int idx = fun.blocks.size() - 1; idx >= 0; idx--) {
            IR::BasicBlock new_block;

            for (int i = 0; i < fun.blocks[idx].instructions.size(); i++) {
                IR::Instruction ins = fun.blocks[idx].instructions[i];
                
                /*if (propagate_instruction(ins, const_var))
                    continue;
                if (eliminate_assert(ins, const_var))
                    continue;*/

                new_block.instructions.push_back(ins);
            }
            
            new_block.predecessors = fun.blocks[idx].predecessors;
            new_block.successors = fun.blocks[idx].successors;
            new_block.is_loop_header = fun.blocks[idx].is_loop_header;
            new_block.final_loop_block = fun.blocks[idx].final_loop_block;

            if (!fun.blocks[idx].successors.empty() && fun.blocks[idx].successors[0] < idx) {
                size_t header = fun.blocks[idx].successors[0];
                for (const auto& pn : fun.blocks[header].phi_nodes) {
                    /*bool know_type = true;
                    for (auto arg : pn.args) {
                        if (arg.second.type == IR::Operand::VIRT_REG && const_var.count(arg.second.index)) {
                            size_t val_idx = find(prog_->immediates.begin(), prog_->immediates.end(), const_var[arg.second.index]) - prog_->immediates.begin();
                            arg.second = {IR::Operand::IMMEDIATE, val_idx};
                        }
                    }*/
                }
            }

            fun.blocks[idx] = new_block;
        }
    }
}