#pragma once

#include <unordered_map>

#include "value.h"
#include "ir.h"

class ConstPropagator {
private:
    IR::Program* prog_;
public:
    ConstPropagator(IR::Program* prog);
    IR::Program* optimize();

    void propagate_const();
    bool propagate_instruction(IR::Instruction ins, std::unordered_map<size_t, runtime::Value> &const_var);
    bool eliminate_assert(IR::Instruction ins, std::unordered_map<size_t, runtime::Value> &const_var);
};