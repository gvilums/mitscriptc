#pragma once

#include "value.h"
#include "ir.h"

class TypeInferer {
private:
    IR::Program* prog_;
public:
    TypeInferer(IR::Program* prog);
    IR::Program* optimize();

    void infer_type();
};