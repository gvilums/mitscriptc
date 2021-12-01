#include "value.h"
#include "ir.h"
#include "type_inferer.h"
#include <set>
#include <map>

typedef runtime::ValueType vt;

TypeInferer::TypeInferer(IR::Program* prog) : prog_(prog){}

IR::Program* TypeInferer::optimize() {
    infer_type();
    return prog_;
}

/*bool TypeInferer::is_string(runtime::ValueType vt){
    return (vt == runtime::ValueType::HeapString) || (vt == runtime::ValueType::InlineString);
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
}*/

vt TypeInferer::get_type(IR::Operand op, std::map<std::pair<size_t, int>, vt> &known_regs, size_t fun_idx) { // if no type return NONE
    if (op.type == IR::Operand::IMMEDIATE)
        return runtime::value_get_type(prog_->immediates[op.index]);
    else if (op.type == IR::Operand::VIRT_REG && known_regs.count({fun_idx, op.index}))
        return known_regs[{fun_idx, op.index}];
    return vt::None;
}

vt TypeInferer::get_phi_node_reg(size_t fun_idx, size_t block_idx, size_t phi_node_idx, size_t arg_idx, std::map<std::pair<size_t, int>, vt> &known_regs) {
    return get_type(prog_->functions[fun_idx].blocks[block_idx].phi_nodes[phi_node_idx].args[arg_idx].second, known_regs, fun_idx);
}

void TypeInferer::infer_type() {
    std::map<std::pair<size_t, int>, vt> known_regs;
    std::map<int, vt> known_fun;
    std::map<std::pair<int, int>, vt> known_free_var;
    std::map<int, vt> known_glob;
    std::map<std::pair<int, int>, int> fun_idx;
    std::set<std::pair<int, int>> valid_ref_vars;

    size_t cur_size = known_regs.size() + known_fun.size() + known_free_var.size();
    while(true) {
        for (size_t i = 0; i < prog_->functions.size(); i++) {
            std::vector<vt> return_recs;
        
            for (size_t j = 0; j < prog_->functions[i].blocks.size(); j++) {
                // phi nodes
                for (size_t k = 0; k < prog_->functions[i].blocks[j].phi_nodes.size(); k++) {
                    vt reg0 = get_phi_node_reg(i, j, k, 0, known_regs);
                    vt reg1 = get_phi_node_reg(i, j, k, 1, known_regs);

                    if (reg0 == vt::None || reg1 == vt::None || reg0 != reg1)
                        continue;
                    known_regs[{i, prog_->functions[i].blocks[j].phi_nodes[k].out.index}] = reg0;
                }
                    
                std::vector<IR::Instruction> new_ins;
                for (size_t k = 0; k < prog_->functions[i].blocks[j].instructions.size(); k++) {
                    IR::Instruction cur_ins = prog_->functions[i].blocks[j].instructions[k];

                    /*if (cur_ins.op == IR::Operation::ASSERT_RECORD && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index}))
                        continue;
                    if (cur_ins.op == IR::Operation::MOV  && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index}))
                        known_regs[{i, cur_ins.out.index}] = known_regs[{i, cur_ins.args[0].index}];
                   
                    if (cur_ins.op == IR::Operation::LOAD_FREE_REF && known_free_var.count({i, cur_ins.args[0].index})) {
                        known_regs[{i, cur_ins.out.index}] = known_free_var[{i, cur_ins.args[0].index}];
                        valid_ref_vars.insert({i, cur_ins.out.index});
                    }
                
                    if (cur_ins.op == IR::Operation::SET_CAPTURE && known_regs.count({i, cur_ins.args[2].index}) && valid_ref_vars.count({i, cur_ins.args[2].index})) {
                        known_free_var[{fun_idx[{i, cur_ins.args[1].index}], cur_ins.args[0].index}] = known_regs[{i, cur_ins.args[2].index}];
                    }

                    if (cur_ins.op == IR::Operation::ALLOC_REF && cur_ins.args[0].index <= 1)
                        valid_ref_vars.insert({i, cur_ins.out.index});

                    if (cur_ins.op == IR::Operation::ALLOC_CLOSURE) {
                        if (known_fun.count(cur_ins.args[0].index))
                            known_regs[{i, cur_ins.out.index}] = known_fun[cur_ins.args[0].index];
                        fun_idx[{i, cur_ins.out.index}] = cur_ins.args[0].index;
                    }

                    if (cur_ins.op == IR::Operation::REF_LOAD && known_regs.count({i, cur_ins.args[0].index}) && valid_ref_vars.count({i, cur_ins.args[0].index})) {
                        known_regs[{i, cur_ins.out.index}] = known_regs[{i, cur_ins.args[0].index}];
                    }

                    if (cur_ins.op == IR::Operation::REF_STORE && cur_ins.args[1].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[1].index}) && valid_ref_vars.count({i, cur_ins.args[0].index})) {
                        known_regs[{i, cur_ins.args[0].index}] = known_regs[{i, cur_ins.args[1].index}];
                    }

                    if (cur_ins.op == IR::Operation::RETURN) {
                        if (cur_ins.args[0].type == IR::Operand::IMMEDIATE || (cur_ins.args[0].type == IR::Operand::VIRT_REG && !known_regs.count({i, cur_ins.args[0].index})))
                            return_recs.push_back(-1);
                        else
                            return_recs.push_back(known_regs[{i, cur_ins.args[0].index}]);
                    }

                    if (cur_ins.op == IR::Operation::LOAD_GLOBAL && known_glob.count(cur_ins.args[0].index)) {
                        known_regs[{i, cur_ins.out.index}] = known_glob[cur_ins.args[0].index];
                    }
                    if (cur_ins.op == IR::Operation::STORE_GLOBAL && cur_ins.args[1].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[1].index}) && !prog_->ref_globals.count(cur_ins.args[0].index)) {
                            known_glob[cur_ins.args[0].index] = known_regs[{i, cur_ins.args[1].index}];
                    }
                    
                    if (cur_ins.op == IR::Operation::EXEC_CALL && known_regs.count({i, cur_ins.args[0].index}))
                        known_regs[{i, cur_ins.out.index}] = known_regs[{i, cur_ins.args[0].index}];

                    if (cur_ins.op == IR::Operation::ALLOC_REC)
                        known_regs[{i, cur_ins.out.index}] = cur_ins.args[1].index;
                    
                    if (cur_ins.op == IR::Operation::REC_STORE_NAME && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index})) {
                        cur_ins.op = IR::Operation::REC_STORE_STATIC;
                        runtime::Value field = prog_->immediates[cur_ins.args[1].index];
                        int rec_idx = known_regs[{i, cur_ins.args[0].index}];
                        int field_idx = (int) (find(prog_->struct_layouts[rec_idx].begin(), prog_->struct_layouts[rec_idx].end(), field) - prog_->struct_layouts[rec_idx].begin());
                        cur_ins.args[1] = {IR::Operand::LOGICAL, field_idx};
                    }
        
                    if (cur_ins.op == IR::Operation::REC_LOAD_NAME && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index})) {
                        cur_ins.op = IR::Operation::REC_LOAD_STATIC;
                        runtime::Value field = prog_->immediates[cur_ins.args[1].index];
                        int rec_idx = known_regs[{i, cur_ins.args[0].index}];
                        int field_idx = (int) (find(prog_->struct_layouts[rec_idx].begin(), prog_->struct_layouts[rec_idx].end(), field) - prog_->struct_layouts[rec_idx].begin());
                        cur_ins.args[1] = {IR::Operand::LOGICAL, field_idx};
                    }*/

                    new_ins.push_back(cur_ins);
                }

                prog_->functions[i].blocks[j].instructions = new_ins;
            }

            vt ret_rec = return_recs[0];
            for (const auto &r : return_recs)
                if (r == vt::None || r != ret_rec)
                    ret_rec = vt::None;
            
            if (ret_rec != vt::None)
                known_fun[(int) i] = ret_rec;
        }

        size_t new_size = known_regs.size() + known_fun.size() + known_free_var.size();
        if (new_size == cur_size)
            break;
        cur_size = new_size;
    }
}