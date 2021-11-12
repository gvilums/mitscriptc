#pragma once

#include <cstddef>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_set>
#include <array>

#include "value.h"
#include "allocator.h"

namespace IR {
    
using LiveInterval = std::vector<std::pair<size_t, size_t>>;
    
enum class MachineRegs {
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
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
    
    LOAD_ARG,           // LOAD_ARG (VIRT_REG id) <- (LOGICAL index)

    LOAD_FREE_REF,      // LOAD_FREE_REF (VIRT_REG id) <- (LOGICAL index)

    REF_LOAD,           // REF_LOAD (VIRT_REG id) <- (VIRT_REG id)
    REF_STORE,          // REF_STORE (VIRT_REG id) <- (VIRT_REG id)
    REC_LOAD_NAME,
    REC_LOAD_INDX,
    REC_STORE_NAME,
    REC_STORE_INDX,
    
    ALLOC_REF,
    ALLOC_REC,
    ALLOC_CLOSURE,
    
    SET_CAPTURE,        // SET_CAPTURE NONE <- (LOGICAL index) (VIRT_REG id) (VIRT_REG id)

    SET_ARG,            // SET_ARG NONE <- (LOGICAL index) (VIRT_REG id)
    CALL,               // CALL NONE <- (LOGICAL num_args) (VIRT_REG id)
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
        NONE,
        VIRT_REG,
        IMMEDIATE,
        LOGICAL,
        MACHINE_REG,
        STACK_SLOT,
    } type{NONE};
    size_t index{0};
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
};

struct Function {
    std::vector<BasicBlock> blocks;
    size_t virt_reg_count;
    size_t parameter_count;
    
    std::vector<Operand> clobbered_regs;
    
    auto compute_live_intervals() -> std::vector<LiveInterval>;
    void set_fixed_machine_regs();
    void allocate_registers();
};

struct Program {
    std::vector<Function> functions;
    std::vector<Value> immediates;
    int num_globals;
};

void debug_live_intervals(const std::vector<LiveInterval>& intervals);

}; // namespace IR

