#pragma once

#include <cstddef>
#include <utility>
#include <vector>
#include <deque>

#include "value.h"
#include "allocator.h"

namespace irepr {

enum class Operation {
    ADD,
    ADD_INT,
    SUB,
    MUL,
    DIV,
    EQ,
    GT,
    GEQ,
    AND,
    OR,
    NOT,

    REF_LOAD,
    REF_STORE,
    REC_LOAD_NAME,
    REC_LOAD_INDX,
    REC_STORE_NAME,
    REC_STORE_INDX,
    
    ALLOC_REF,
    ALLOC_REC,
    ALLOC_CLOSURE,

    CALL,
    RETURN,

    MOV,
    LOAD_GLOBAL,
    STORE_GLOBAL,
    ASSERT_BOOL,
    ASSERT_INT,
    ASSERT_STRING,
    ASSERT_RECORD,
    ASSERT_CLOSURE,
    ASSERT_NONZERO,
    
    PRINT,
    INPUT,
    INTCAST,
};

struct Operand {
    enum OpType {
        None,
        VirtReg,
        Argument,
        Immediate,
        MachineReg,
        StackSlot,
    } type;
    size_t index;
};

class Instruction {
    Operation op;
    Operand out;
    std::vector<Operand> args;
};

class PhiNode {
    Operand out;
    std::vector<std::pair<size_t, Operand>> args;
};

class BasicBlock {
    std::vector<PhiNode> phi_nodes;
    std::deque<Instruction> instructions;
    std::vector<size_t> predecessors;
    size_t true_successor;
    size_t false_successor;
};

class Function {
    std::vector<Operand> clobbered_regs;
    std::vector<BasicBlock> blocks;
    
    void allocate_registers();
};

class Program {
    std::vector<Function> functions;
    std::vector<Value> constants;
};

}; // namespace IR