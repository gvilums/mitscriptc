#include "ir.h"

namespace IR {

void Program::apply_pass(BasePass& pass) {
    pass.apply_to(*this);
}

};