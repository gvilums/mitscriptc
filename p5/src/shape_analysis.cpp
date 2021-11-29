#include "value.h"
#include "ir.h"
#include "shape_analysis.h"
#include <string>
#include <utility>
#include <vector>
#include <set>

ShapeAnalysis::ShapeAnalysis(IR::Program* prog) : prog_(prog){}

IR::Program* ShapeAnalysis::optimize() {
    infer_structs();
    return prog_;
}

int ShapeAnalysis::get_phi_node_reg(size_t fun_idx, size_t block_idx, size_t phi_node_idx, size_t arg_idx, std::map<std::pair<size_t, int>, int> &known_regs) {
    if (prog_->functions[fun_idx].blocks[block_idx].phi_nodes[phi_node_idx].args[arg_idx].second.type != IR::Operand::VIRT_REG)
        return -1;
    if (!known_regs.count({fun_idx, prog_->functions[fun_idx].blocks[block_idx].phi_nodes[phi_node_idx].args[arg_idx].second.index}))
        return -1;
    return known_regs[{fun_idx, prog_->functions[fun_idx].blocks[block_idx].phi_nodes[phi_node_idx].args[arg_idx].second.index}];
}

void ShapeAnalysis::infer_structs() {
    std::map<std::pair<size_t, int>, int> known_regs;
    std::map<int, int> known_fun;
    std::map<std::pair<int, int>, int> known_free_var;
    std::map<int, int> known_glob;
    std::map<std::pair<int, int>, int> fun_idx;
    std::set<std::pair<int, int>> valid_ref_vars;

    size_t cur_size = known_regs.size() + known_fun.size() + known_free_var.size();
    while(true) {
        for (size_t i = 0; i < prog_->functions.size(); i++) {
            std::vector<int> return_recs;
        
            for (size_t j = 0; j < prog_->functions[i].blocks.size(); j++) {
                // phi nodes
                for (size_t k = 0; k < prog_->functions[i].blocks[j].phi_nodes.size(); k++) {
                    int reg0 = get_phi_node_reg(i, j, k, 0, known_regs);
                    int reg1 = get_phi_node_reg(i, j, k, 1, known_regs);
                    if (reg0 == - 1 || reg1 == - 1 || reg0 != reg1)
                        continue;
                    known_regs[{i, prog_->functions[i].blocks[j].phi_nodes[k].out.index}] = reg0;
                }
                    
                std::vector<IR::Instruction> new_ins;
                for (size_t k = 0; k < prog_->functions[i].blocks[j].instructions.size(); k++) {
                    IR::Instruction cur_ins = prog_->functions[i].blocks[j].instructions[k];

                    if (cur_ins.op == IR::Operation::ASSERT_RECORD && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index}))
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
                        known_glob[cur_ins.out.index] = known_regs[{i, cur_ins.args[1].index}];
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
                    }

                    new_ins.push_back(cur_ins);
                }

                prog_->functions[i].blocks[j].instructions = new_ins;
            }

            int ret_rec = return_recs[0];
            for (const auto &r : return_recs)
                if (r == - 1 || r != ret_rec)
                    ret_rec = - 1;
            
            if (ret_rec != - 1)
                known_fun[(int) i] = ret_rec;
        }

        size_t new_size = known_regs.size() + known_fun.size() + known_free_var.size();
        if (new_size == cur_size)
            break;
        cur_size = new_size;
    }
}
