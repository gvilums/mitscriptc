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

#include "value.h"

namespace IR {

enum class MachineReg : size_t {
    RDI, // gp, first arg, volatile
    RSI, // gp, second arg, volatile
    RDX, // gp, third arg, volatile
    RCX, // gp, fourth arg, volatile
    R8,  // gp, fifth arg, volatile
    R9,  // gp, sixth arg, volatile
    RAX, // gp, seventh arg, volatile
    R12, // gp, nonvolatile
    R13, // gp, nonvolatile
    R14, // gp, nonvolatile
    R15, // gp, nonvolatile
    RBX, // current function, nonvolatile
    R10, // temporary, volatile
    R11, // temporary, volatile
};

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
    REF_STORE,  // REF_STORE NONE <- (VIRT_REG id) (VIRT_REG id)
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
    BRANCH,
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

    auto get_machine() const -> std::optional<MachineReg>;

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
    std::vector<Instruction> instructions;
    std::vector<size_t> predecessors;
    std::vector<size_t> successors;

    bool is_loop_header{false};
    size_t final_loop_block{0};
};

struct Function {
    std::vector<BasicBlock> blocks;
    size_t virt_reg_count;
    size_t parameter_count;

    size_t stack_slots;
    
    auto split_edge(size_t from, size_t to) -> BasicBlock&;
};

struct Program {
    std::vector<Function> functions;
    std::vector<runtime::Value> immediates;
    size_t num_globals{0};
    runtime::Runtime* rt{nullptr};

    Program();
};

}; 
