#pragma once

#include "value.h"
#include "ir.h"

class ConstPropagator {
private:
    IR::Program* prog_;
public:
    ConstPropagator(IR::Program* prog);
    IR::Program* optimize();

    void propagate_const();
};