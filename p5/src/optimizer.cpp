#include <iterator>
#include "ir.h"
#include "optimizer.h"
#include "value.h"
#include <set>
#include "irprinter.h"

const std::set<IR::Operation> unused_ins = {IR::Operation::ADD, IR::Operation::ADD_INT, IR::Operation::SUB, IR::Operation::MUL, IR::Operation::DIV, IR::Operation::EQ, IR::Operation::GT, IR::Operation::GEQ, IR::Operation::AND, IR::Operation::OR, IR::Operation::NOT, IR::Operation::MOV, IR::Operation::ASSERT_BOOL, IR::Operation::ASSERT_INT, IR::Operation::ASSERT_STRING, IR::Operation::ASSERT_RECORD, IR::Operation::ASSERT_CLOSURE, IR::Operation::ASSERT_NONZERO};

optimizer::optimizer(IR::Program* prog) : prog_(prog){};

IR::Program* optimizer::optimize() {
    rm_unused_var(); // REMOVE NOT USED REGISTERS
    return prog_;
}

bool optimizer::is_unused_op(const IR::Operation op){
    return unused_ins.contains(op);
}

void optimizer::rm_unused_var(){
    for (auto& fun : prog_->functions) {
        std::unordered_set<size_t> used_var;
        for (int idx = fun.blocks.size() - 1; idx >= 0; idx--) {
            IR::BasicBlock new_block;
            size_t b_size = fun.blocks[idx].instructions.size();
            for (int i = b_size - 1; i >= 0; i--) {
                IR::Instruction rit = fun.blocks[idx].instructions[i]; 
                if (is_unused_op(rit.op) && rit.out.type == IR::Operand::VIRT_REG && !used_var.contains(rit.out.index))      
                    continue;
                
                
                for (auto arg : rit.args)
                    if (arg.type == IR::Operand::VIRT_REG)
                        used_var.insert(arg.index);
                
                new_block.instructions.push_back(rit);
            }
            new_block.predecessors = fun.blocks[idx].predecessors;
            new_block.successors = fun.blocks[idx].successors;
            new_block.is_loop_header = fun.blocks[idx].is_loop_header;
            new_block.final_loop_block = fun.blocks[idx].final_loop_block;

            

            for (auto pn : fun.blocks[idx].phi_nodes) {
                if (used_var.contains(pn.out.index)) {
                    for (auto arg : pn.args)
                        if (arg.second.type == IR::Operand::VIRT_REG)
                            used_var.insert(arg.second.index);
                    new_block.phi_nodes.push_back(pn);
                }
            }
        
            std::reverse(new_block.instructions.begin(), new_block.instructions.end());
            fun.blocks[idx] = new_block;
        }
    }
}