#include "value.h"
#include "ir.h"
#include "shape_analysis.h"
#include <string>
#include <vector>

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
    std::vector<int> known_globals(prog_->num_globals, - 1);
    std::map<int, int> known_refs;

    bool changed = false;
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
                changed = true;
            }
                
            std::vector<IR::Instruction> new_ins;
            for (size_t k = 0; k < prog_->functions[i].blocks[j].instructions.size(); k++) {
                IR::Instruction cur_ins = prog_->functions[i].blocks[j].instructions[k];

                if (cur_ins.op == IR::Operation::ASSERT_RECORD && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index})) {
                    changed = true;
                    continue;
                }

                // move instruction missing
                
                /*if (cur_ins.op == IR::Operation::LOAD_FREE_REF && known_free_var.count(cur_ins.args[0].index)) {
                    known_local_ref[cur_ins.args[0].index] = known_free_var[cur_ins.args[0].index];
                    changed = true;
                }*/

                /*if (cur_ins.op == IR::Operation::LOAD_GLOBAL && known_globals[cur_ins.args[0].index] != - 1) {
                    known_regs[{i, cur_ins.out.index}] = known_globals[cur_ins.args[0].index];
                    changed = true;
                }*/

                if (cur_ins.op == IR::Operation::REF_LOAD && known_refs.count(cur_ins.args[0].index)) {
                    known_refs[{i, cur_ins.out.index}] = known_refs[cur_ins.args[0].index];
                    changed = true;
                }
  
                /*if (cur_ins.op == IR::Operation::STORE_GLOBAL) {
                     if (cur_ins.args[0].type == IR::Operand::IMMEDIATE || (cur_ins.args[1].type == IR::Operand::VIRT_REG && !known_regs.count({i, cur_ins.args[1].index})))
                        known_globals[cur_ins.args[0].index] = -1;
                    else
                        known_globals[cur_ins.args[0].index] = known_regs[{i, cur_ins.args[1].index}];
                    changed = true;
                }*/

                if (cur_ins.op == IR::Operation::REF_STORE) {
                     if (cur_ins.args[0].type == IR::Operand::IMMEDIATE || (cur_ins.args[1].type == IR::Operand::VIRT_REG && !known_regs.count({i, cur_ins.args[1].index})))
                        known_local_ref.erase(cur_ins.args[0].index);
                    else
                        known_local_ref[cur_ins.args[0].index] = known_regs[{i, cur_ins.args[1].index}];
                    changed = true;
                }

                if (cur_ins.op == IR::Operation::RETURN) {
                    if (cur_ins.args[0].type == IR::Operand::IMMEDIATE || (cur_ins.args[0].type == IR::Operand::VIRT_REG && !known_regs.count({i, cur_ins.args[0].index})))
                        return_recs.push_back(-1);
                    else
                        return_recs.push_back(known_regs[{i, cur_ins.args[0].index}]);
                    changed = true;
                }
                
                if (cur_ins.op == IR::Operation::EXEC_CALL && known_fun.count(cur_ins.args[1].index)) {
                    known_regs[{i, cur_ins.out.index}] = known_fun[cur_ins.args[1].index];
                    changed = true;
                }

                if (cur_ins.op == IR::Operation::ALLOC_REC) {
                    known_regs[{i, cur_ins.out.index}] = cur_ins.args[2].index;
                    changed = true;
                }
                
                if (cur_ins.op == IR::Operation::REC_STORE_NAME && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index})) {
                    cur_ins.op = IR::Operation::REC_STORE_STATIC;
                    runtime::Value field = prog_->immediates[cur_ins.args[1].index];
                    int rec_idx = known_regs[{i, cur_ins.args[0].index}];
                    int field_idx = (int) (find(prog_->struct_layouts[rec_idx].begin(), prog_->struct_layouts[rec_idx].end(), field) - prog_->struct_layouts[rec_idx].begin());
                    cur_ins.args[1] = {IR::Operand::LOGICAL, field_idx};
                    changed = true;
                }
    
                if (cur_ins.op == IR::Operation::REC_LOAD_NAME && cur_ins.args[0].type == IR::Operand::VIRT_REG && known_regs.count({i, cur_ins.args[0].index})) {
                    cur_ins.op = IR::Operation::REC_LOAD_STATIC;
                    runtime::Value field = prog_->immediates[cur_ins.args[1].index];
                    int rec_idx = known_regs[{i, cur_ins.args[0].index}];
                    int field_idx = (int) (find(prog_->struct_layouts[rec_idx].begin(), prog_->struct_layouts[rec_idx].end(), field) - prog_->struct_layouts[rec_idx].begin());
                    cur_ins.args[1] = {IR::Operand::LOGICAL, field_idx};
                    changed = true;
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
}
