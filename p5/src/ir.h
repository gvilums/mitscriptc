#pragma once

#include <sys/ucontext.h>
#include <array>
#include <cstddef>
#include <deque>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>
#include <deque>

#include "allocator.h"
#include "value.h"

namespace IR {

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

    LOAD_ARG,  // LOAD_ARG (VIRT_REG id) <- (LOGICAL index)

    LOAD_FREE_REF,  // LOAD_FREE_REF (VIRT_REG id) <- (LOGICAL index)

    REF_LOAD,   // REF_LOAD (VIRT_REG id) <- (VIRT_REG id)
    REF_STORE,  // REF_STORE (VIRT_REG id) <- (VIRT_REG id)
    REC_LOAD_NAME,
    REC_LOAD_INDX,
    REC_STORE_NAME,
    REC_STORE_INDX,

    ALLOC_REF,
    ALLOC_REC,
    ALLOC_CLOSURE,		// (VIRT_REG id), FUNCTION IDX, #SET_CAPTUREs
    
    SET_CAPTURE,        // SET_CAPTURE NONE <- (LOGICAL index) (VIRT_REG id) (VIRT_REG id)

    SET_ARG,            // SET_ARG NONE <- (LOGICAL index) (VIRT_REG id)
    CALL,               // CALL (VIRT_REG id) <- (LOGICAL num_args) (VIRT_REG id)
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
    
    SWAP,
};

struct Operand {
    enum OpType {
        NONE,
        VIRT_REG,
        IMMEDIATE,
        LOGICAL,
        MACHINE_REG,
        STACK_SLOT,
    } type{NONE};
    size_t index{0};

    bool operator==(const Operand& other) const {
        return this->type == other.type && this->index == other.index;
    }

    bool operator!=(const Operand& other) const {
        return this->type != other.type || this->index != other.index;
    }
};

struct Instruction {
    Operation op;
    Operand out;
    std::array<Operand, 3> args;
};

struct PhiNode {
    Operand out;
    std::vector<std::pair<size_t, Operand>> args;
};

struct BasicBlock {
    std::vector<PhiNode> phi_nodes;
    std::deque<Instruction> instructions;
    std::vector<size_t> predecessors;
    std::vector<size_t> successors;

    bool is_loop_header;
    size_t final_loop_block;

    std::vector<std::pair<Operand, Operand>> resolution_map;
};

struct Function {
    std::vector<BasicBlock> blocks;
    size_t virt_reg_count;
    size_t parameter_count;

    std::vector<Operand> clobbered_regs;
};

class BasePass;

struct Program {
    std::vector<Function> functions;
    std::vector<Value> immediates;
    int num_globals;
    
    void apply_pass(BasePass& pass);
};

class BasePass {
   public:
    virtual void apply_to(Program& program) = 0;
    virtual ~BasePass() {};
};

}; 
