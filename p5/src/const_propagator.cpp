#include "value.h"
#include "ir.h"
#include "const_propagator.h"

const_propagator::const_propagator(IR::Program* prog) : prog_(prog){}

IR::Program* const_propagator::optimize() {
    propagate_const();
    return prog_;
}

void const_propagator::propagate_const() {

}
