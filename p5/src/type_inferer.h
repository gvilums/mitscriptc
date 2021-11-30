#pragma once

#include "value.h"
#include "ir.h"

class TypeInferer {
private:
    IR::Program* prog_;
public:
    TypeInferer(IR::Program* prog);
    IR::Program* optimize();

    bool is_string(runtime::ValueType vt);
    bool proc_opr(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var);
    bool has_type(IR::Operand op, std::unordered_map<size_t, runtime::ValueType> &type_var);
    runtime::ValueType get_type(IR::Operand op, std::unordered_map<size_t, runtime::ValueType> &type_var);
    bool proc_assert(IR::Instruction ins, std::unordered_map<size_t, runtime::ValueType> &type_var);
    void infer_type();
};