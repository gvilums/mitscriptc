#pragma once

#include "value.h"
#include "ir.h"

class DeadCodeRemover {
private:
    IR::Program* prog_;
public:
    DeadCodeRemover(IR::Program* prog);
    IR::Program* optimize();

    bool is_dc_op(const IR::Operation op);
    void optimize();
};