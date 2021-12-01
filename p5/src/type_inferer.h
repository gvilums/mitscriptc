#pragma once

#include "value.h"
#include "ir.h"
#include <map>

class TypeInferer {
private:
    IR::Program* prog_;
public:
    TypeInferer(IR::Program* prog);
    IR::Program* optimize();

    runtime::ValueType get_phi_node_reg(size_t fun_idx, size_t block_idx, size_t phi_node_idx, size_t arg_idx, std::map<std::pair<size_t, int>, runtime::ValueType> &known_regs);
    bool is_string(runtime::ValueType vt);
    bool proc_opr(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var);
    runtime::ValueType get_type(IR::Operand op, std::map<std::pair<size_t, int>, runtime::ValueType> &known_regs, size_t fun_idx);
    bool proc_assert(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var);
    void infer_type();
};