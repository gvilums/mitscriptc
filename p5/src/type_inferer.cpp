#include "value.h"
#include "ir.h"
#include "type_inferer.h"
#include <set>

TypeInferer::TypeInferer(IR::Program* prog) : prog_(prog){}

IR::Program* TypeInferer::optimize() {
    infer_type();
    return prog_;
}

bool TypeInferer::is_string(runtime::ValueType vt){
    return (vt == runtime::ValueType::HeapString) || (vt == runtime::ValueType::InlineString);
}

bool TypeInferer::has_type(IR::Operand op, std::unordered_map<size_t, runtime::ValueType> &type_var) {
    if (op.type == IR::Operand::MACHINE_REG && type_var.count(op.index))
        return true;
    if (op.type == IR::Operand::IMMEDIATE)
        return true;
    return false;
}

runtime::ValueType TypeInferer::get_type(IR::Operand op, std::unordered_map<size_t, runtime::ValueType> &type_var) {
    if (op.type == IR::Operand::MACHINE_REG)
        return type_var[op.index];
    return runtime::value_get_type(prog_->immediates[op.index]);
}

bool TypeInferer::proc_assert(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var) { // return true if we can eliminate the assert
    switch (ins.op) {
        case IR::Operation::ASSERT_BOOL:
            if (has_type(ins.args[0], type_var)) {
                if (get_type(ins.args[0], type_var) != runtime::ValueType::Bool)
                    throw std::string("IllegalCastException");
                return true;
            }
            type_var[ins.args[0].index] = runtime::ValueType::Bool;
            break;
        case IR::Operation::ASSERT_INT:
            if (has_type(ins.args[0], type_var)) {
                if (get_type(ins.args[0], type_var) != runtime::ValueType::Int)
                    throw std::string("IllegalCastException");
                return true;
            }
            type_var[ins.args[0].index] = runtime::ValueType::Int;
            break;
        case IR::Operation::ASSERT_STRING:
            if (has_type(ins.args[0], type_var)) {
                if (!is_string(get_type(ins.args[0], type_var)))
                    throw std::string("IllegalCastException");
                return true;
            }
            type_var[ins.args[0].index] = runtime::ValueType::HeapString; // it shouldn't matter which string type we choose
            break;
        case IR::Operation::ASSERT_CLOSURE:
            if (has_type(ins.args[0], type_var)) {
                if (get_type(ins.args[0], type_var) != runtime::ValueType::Closure)
                    throw std::string("IllegalCastException");
                return true;
            }
            type_var[ins.args[0].index] = runtime::ValueType::Closure;
            break;
        default:
            return false;
    }
    return false;
}

bool TypeInferer::proc_opr(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var) { // return true if we changed the add
    runtime::ValueType vt1 = get_type(ins.args[0], type_var);
    runtime::ValueType vt2 = get_type(ins.args[1], type_var);
    bool kt1 = has_type(ins.args[0], type_var);
    bool kt2 = has_type(ins.args[1], type_var);

    if (ins.op == IR::Operation::ADD) {
        if ((is_string(vt1) && kt1) || (is_string(vt2) && kt2)) {
            type_var[ins.out.index] =  runtime::ValueType::HeapString; // it shouldn't matter which string type we choose
            return false;
        } else if (!kt1 || !kt2) {
            return false;
        } else if (vt1 == runtime::ValueType::Int && vt2 == runtime::ValueType::Int) {
            type_var[ins.out.index] =  runtime::ValueType::Int;
            return true;
        } else
            throw std::string("IllegalCastException");
    }

    if (ins.op == IR::Operation::NOT) {
        if (!kt1) 
            return false;
        if (vt1 != runtime::ValueType::Bool)
            throw std::string("IllegalCastException");
        type_var[ins.out.index] =  runtime::ValueType::Bool;
        return false;
    }

    if (ins.op == IR::Operation::MOV) {
        if (kt1) 
            type_var[ins.out.index] =  vt1;
        return false;
    }

    if (!kt1 || !kt2)
        return false;

    switch (ins.op) {
        case IR::Operation::ADD_INT:
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Int;
            break;
        case IR::Operation::MUL:  
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Int;
            break;
        case IR::Operation::SUB:  
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Int;
            break;
        case IR::Operation::DIV:  
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Int;
            break;
        case IR::Operation::EQ:
            if (vt1 != vt2)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Bool;
            break;
        case IR::Operation::GT:
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Bool;
            break;
        case IR::Operation::GEQ:
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Bool;
            break;
        case IR::Operation::AND:
            if (vt1 != runtime::ValueType::Bool || vt2 != runtime::ValueType::Bool)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Bool;
            break;
        case IR::Operation::OR:
            if (vt1 != runtime::ValueType::Int || vt2 != runtime::ValueType::Int)
                throw std::string("IllegalCastException");
            type_var[ins.out.index] =  runtime::ValueType::Bool;
        default:
            return false;
    }
    return false;
}

void TypeInferer::infer_type() {
    for (auto& fun : prog_->functions) {
        std::unordered_map<size_t, runtime::ValueType> type_var;
        size_t old_size = type_var.size();
        while (true) {
            for (size_t idx = 0; idx < fun.blocks.size(); idx++) {
                IR::BasicBlock new_block;

                for (int i = 0; i < fun.blocks[idx].instructions.size(); i++) {
                    IR::Instruction ins = fun.blocks[idx].instructions[i];
                    
                    if (proc_assert(ins, type_var))
                        continue;
                    if (proc_opr(ins, type_var))
                        ins.op = IR::Operation::ADD_INT;
                    
                    new_block.instructions.push_back(ins);
                }
                
                new_block.predecessors = fun.blocks[idx].predecessors;
                new_block.successors = fun.blocks[idx].successors;
                new_block.is_loop_header = fun.blocks[idx].is_loop_header;
                new_block.final_loop_block = fun.blocks[idx].final_loop_block;

                if (!fun.blocks[idx].successors.empty() && fun.blocks[idx].successors[0] < idx) {
                    size_t header = fun.blocks[idx].successors[0];
                    for (const auto& pn : fun.blocks[header].phi_nodes) {
                        if (!has_type(pn.args[0].second, type_var) || !has_type(pn.args[1].second, type_var))
                            continue;

                        runtime::ValueType vt1 = get_type(pn.args[0].second, type_var);
                        runtime::ValueType vt2 = get_type(pn.args[1].second, type_var);
                        
                        if (vt1 == vt2)
                            type_var[pn.out.index] = vt1;
                    }
                }

                fun.blocks[idx] = new_block;
            }

            if (old_size == type_var.size())
                break;
            old_size = type_var.size();
        }
    }
}