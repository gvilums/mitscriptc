#pragma once

#include "value.h"
#include "ir.h"

class optimizer {
private:
    IR::Program* prog_;
public:
    optimizer(IR::Program* prog);
    IR::Program* optimize();

    bool is_unused_op(const IR::Operation op);
    void rm_unused_var();
};