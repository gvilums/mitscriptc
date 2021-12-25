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

bool TypeInferer::proc_assert(IR::Instruction &ins, std::map<std::pair<size_t, int>, vt> &known_regs, size_t fun_idx) { // return true if we can eliminate the assert
    if (ins.args[1].type != IR::Operand::NONE)
        return false;

    switch (ins.op) {
        case IR::Operation::ASSERT_BOOL:
            if (get_type(ins.args[0], known_regs, fun_idx) == vt::Bool) 
                return true;
            ins.args[1].type = IR::Operand::IMMEDIATE;
            known_regs[{fun_idx, ins.args[0].index}] = vt::Bool;
            break;
        case IR::Operation::ASSERT_INT:
            if (get_type(ins.args[0], known_regs, fun_idx) == vt::Int) 
                return true;
            ins.args[1].type = IR::Operand::IMMEDIATE;
            known_regs[{fun_idx, ins.args[0].index}] = vt::Int;
            break;
        default:
            return false;
    }
    return false;
}

void TypeInferer::proc_opr(IR::Instruction &ins, std::map<std::pair<size_t, int>, vt> &known_regs, size_t fun_idx) { // return true if we changed the add
    vt vt1 = get_type(ins.args[0], known_regs, fun_idx);
    vt vt2 = get_type(ins.args[1], known_regs, fun_idx);
   
    if (ins.op == IR::Operation::ADD) {
        if (vt1 == vt::Int && vt2 == vt::Int) {
            known_regs[{fun_idx, ins.out.index}] = vt::Int;
            ins.op = IR::Operation::ADD_INT;
        }    
    }
    else if (ins.op == IR::Operation::NOT) {
        if (vt1 == vt::Bool)
            known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
    }
    else if (ins.op == IR::Operation::MOV) {
        if (vt1 != vt::None) 
            known_regs[{fun_idx, ins.out.index}] =  vt1;
    }

    switch (ins.op) {
        case IR::Operation::ADD_INT:
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Int;
            break;
        case IR::Operation::MUL:  
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Int;
            break;
        case IR::Operation::SUB:  
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Int;
            break;
        case IR::Operation::DIV:  
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Int;
            break;
        case IR::Operation::EQ:
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
            break;
        case IR::Operation::GT:
            if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
            break;
        case IR::Operation::GEQ:
           if (vt1 == vt::Int && vt2 == vt::Int)
                known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
            break;
        case IR::Operation::AND:
            if (vt1 == vt::Bool && vt2 == vt::Bool)
                known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
            break;
        case IR::Operation::OR:
            if (vt1 == vt::Bool && vt2 == vt::Bool)
                known_regs[{fun_idx, ins.out.index}] =  vt::Bool;
            break;
        default:
            break;
    }
}

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

                    proc_opr(cur_ins, known_regs, i);

                    if (proc_assert(cur_ins, known_regs, i))
                        continue;
                   
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

                    if (cur_ins.op == IR::Operation::REF_LOAD && get_type(cur_ins.args[1], known_regs, i) != vt::None && valid_ref_vars.count({i, cur_ins.args[0].index})) {
                        known_regs[{i, cur_ins.out.index}] = get_type(cur_ins.args[1], known_regs, i);
                    }

                    if (cur_ins.op == IR::Operation::REF_STORE && get_type(cur_ins.args[1], known_regs, i) != vt::None && valid_ref_vars.count({i, cur_ins.args[0].index})) {
                        known_regs[{i, cur_ins.args[0].index}] = get_type(cur_ins.args[1], known_regs, i);
                    }

                    if (cur_ins.op == IR::Operation::RETURN) {
                        return_recs.push_back(get_type(cur_ins.args[0], known_regs, i));
                    }

                    if (cur_ins.op == IR::Operation::LOAD_GLOBAL && known_glob.count(cur_ins.args[0].index)) {
                        known_regs[{i, cur_ins.out.index}] = known_glob[cur_ins.args[0].index];
                    }
                    if (cur_ins.op == IR::Operation::STORE_GLOBAL && get_type(cur_ins.args[1], known_regs, i) != vt::None && !prog_->ref_globals.count(cur_ins.args[0].index)) {
                            known_glob[cur_ins.args[0].index] = get_type(cur_ins.args[1], known_regs, i);
                    }
                    
                    if (cur_ins.op == IR::Operation::EXEC_CALL && known_regs.count({i, cur_ins.args[0].index}))
                        known_regs[{i, cur_ins.out.index}] = known_regs[{i, cur_ins.args[0].index}];

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