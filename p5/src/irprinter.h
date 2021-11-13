#include <iostream>

#include "ir.h"

std::ostream& operator<<(std::ostream& os, const IR::Operand& opr);
std::ostream& operator<<(std::ostream& os, const IR::Operation& op);
std::ostream& operator<<(std::ostream& os, const IR::Instruction& ins);
std::ostream& operator<<(std::ostream& os, const IR::BasicBlock& bb);
std::ostream& operator<<(std::ostream& os, const IR::Function& fun);
std::ostream& operator<<(std::ostream& os, const IR::Program& prog);