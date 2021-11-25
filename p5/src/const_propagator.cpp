#include "value.h"
#include "ir.h"
#include "const_propagator.h"
#include <set>

ConstPropagator::ConstPropagator(IR::Program* prog) : prog_(prog){}

bool ConstPropagator::propagate_instruction(IR::Instruction ins, std::unordered_map<size_t, runtime::Value> &const_var) {
    bool can_eliminate = true;
    for (auto& arg : ins.args) {
        if (arg.type == IR::Operand::VIRT_REG && const_var.count(arg.index))  {
            // TODO FIXME
//            arg = {IR::Operand::IMMEDIATE, const_var[arg.index]};
        } else if (arg.type == IR::Operand::IMMEDIATE) {
            continue;
        } else if (arg.type != IR::Operand::NONE) {
            can_eliminate = false;
        }
    }

    if (!can_eliminate)
        return false;

    runtime::Value new_value;
    runtime::Value imm0 = prog_->immediates[ins.args[0].index];
    runtime::Value imm1 = prog_->immediates[ins.args[1].index];
    
    switch (ins.op) {
        case IR::Operation::ADD:
            new_value = runtime::value_add(prog_->ctx_ptr, imm0, imm1);
            break;
        case IR::Operation::ADD_INT:
            new_value = runtime::value_add_int32(imm0, imm1);
            break;
        case IR::Operation::SUB:  
            new_value = runtime::value_sub(imm0, imm1);
            break;
         case IR::Operation::DIV:  
            new_value = runtime::value_div(imm0, imm1); // check if we are dividing by zero
            break;
        case IR::Operation::MUL:  
            new_value = runtime::value_mul(imm0, imm1);
            break;
        case IR::Operation::EQ:
            new_value = runtime::value_eq(imm0, imm1);
            break;
        case IR::Operation::GT:
            new_value = runtime::value_gt(imm0, imm1);
            break;
        case IR::Operation::GEQ:
            new_value = runtime::value_geq(imm0, imm1);
            break;
        case IR::Operation::AND:
            new_value = runtime::value_and(imm0, imm1);
            break;
        case IR::Operation::OR:
            new_value = runtime::value_or(imm0, imm1);
            break;
        case IR::Operation::NOT:
            new_value = runtime::value_not(imm0);
            break;
        case IR::Operation::MOV:
            new_value = imm0;
            break;
        default:
            return false;
    }

    size_t val_idx;
    if (!count(prog_->immediates.begin(), prog_->immediates.end(), new_value)) {
        val_idx = prog_->immediates.size();
        prog_->immediates.push_back(new_value);
    }
    else
        val_idx = find(prog_->immediates.begin(), prog_->immediates.end(), new_value) - prog_->immediates.begin();

    const_var[ins.out.index] = val_idx;
    return true;
}

bool ConstPropagator::eliminate_assert(IR::Instruction ins, std::unordered_map<size_t, runtime::Value> &const_var) {

    runtime::Value imm;
    if (ins.args[0].type == IR::Operand::VIRT_REG && const_var.count(ins.args[0].index))
        imm = const_var[ins.args[0].index];
    else if (ins.args[0].type == IR::Operand::IMMEDIATE)
        imm = prog_->immediates[ins.args[0].index];
    else
        return false;

    switch (ins.op) {
        case IR::Operation::ASSERT_BOOL:
            if (runtime::value_get_type(imm) != runtime::ValueType::Bool)
                throw std::string("IllegalCastException");
            break;
        case IR::Operation::ASSERT_INT:
            if (runtime::value_get_type(imm) != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            break;
        case IR::Operation::ASSERT_STRING:
            if (runtime::value_get_type(imm) != runtime::ValueType::HeapString && runtime::value_get_type(imm) != runtime::ValueType::InlineString)
                throw std::string("IllegalCastException");  
            break;
        case IR::Operation::ASSERT_CLOSURE:
            if (runtime::value_get_type(imm) != runtime::ValueType::Closure)
                throw std::string("IllegalCastException");
            break;
        case IR::Operation::ASSERT_NONZERO:
            if (runtime::value_get_type(imm) != runtime::ValueType::Int || runtime::value_get_int32(imm) == 0)
                throw std::string("DivideByZero");
            break;
        default:
            return false;
    }

    return true;
}

IR::Program* ConstPropagator::optimize() {
    propagate_const();
    return prog_;
}

void ConstPropagator::propagate_const() {
    for (auto& fun : prog_->functions) {
        std::unordered_map<size_t, runtime::Value> const_var;
        for (size_t idx = 0; idx < fun.blocks.size() - 1; idx++) {
            IR::BasicBlock new_block;

            for (int i = 0; i < fun.blocks[idx].instructions.size(); i++) {
                IR::Instruction ins = fun.blocks[idx].instructions[i];
                
                if (propagate_instruction(ins, const_var))
                    continue;
                if (eliminate_assert(ins, const_var))
                    continue;

                new_block.instructions.push_back(ins);
            }
            
            new_block.predecessors = fun.blocks[idx].predecessors;
            new_block.successors = fun.blocks[idx].successors;
            new_block.is_loop_header = fun.blocks[idx].is_loop_header;
            new_block.final_loop_block = fun.blocks[idx].final_loop_block;

            if (!fun.blocks[idx].successors.empty() && fun.blocks[idx].successors[0] < idx) {
                size_t header = fun.blocks[idx].successors[0];
                for (const auto& pn : fun.blocks[header].phi_nodes) {
                    for (auto arg : pn.args)
                        if (arg.second.type == IR::Operand::VIRT_REG && const_var.count(arg.second.index)) {
                            size_t val_idx = find(prog_->immediates.begin(), prog_->immediates.end(), const_var[arg.second.index]) - prog_->immediates.begin();
                            // TODO FIXME
//                            arg.second = {IR::Operand::IMMEDIATE, val_idx};
                        }
                }
            }

            fun.blocks[idx] = new_block;
        }
    }
}