#pragma once

#include "value.h"
#include "ir.h"

class const_propagator {
private:
    IR::Program* prog_;
public:
    const_propagator(IR::Program* prog);
    IR::Program* optimize();

    void propagate_const();
};