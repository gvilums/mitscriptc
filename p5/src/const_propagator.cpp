#include "value.h"
#include "ir.h"
#include "const_propagator.h"

ConstPropagator::ConstPropagator(IR::Program* prog) : prog_(prog){}

IR::Program* ConstPropagator::optimize() {
    propagate_const();
    return prog_;
}

void ConstPropagator::propagate_const() {
    
}
