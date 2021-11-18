#pragma once

#include "value.h"
#include "ir.h"

class dead_code_remover {
private:
    IR::Program* prog_;
public:
    dead_code_remover(IR::Program* prog);
    IR::Program* optimize();

    bool is_dc_op(const IR::Operation op);
    void rm_dead_code();
};