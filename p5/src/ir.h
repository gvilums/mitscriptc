#pragma once

#include <cstddef>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_set>
#include <array>
#include <optional>
#include <iostream>

#include "value.h"
#include "allocator.h"

namespace IR {
    
const size_t MACHINE_REG_COUNT = 2;
    
// enum class RegAssignment {
//     MACHINE_REG,
//     STACK_SLOT,
//     UNINIT,
// };


// struct VregAssignment {
//     size_t vreg_id{0};
//     std::vector<std::pair<std::pair<RegAssignment, size_t>, std::pair<size_t, size_t>>> assignments;
    
//     VregAssignment() = default;
//     VregAssignment(std::vector<LiveInterval>);
    
//     auto assignment_at(size_t pos) const -> std::optional<std::pair<RegAssignment, size_t>>;
//     auto begins_at(size_t pos) const -> bool;
// };


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

struct LiveInterval {
    std::vector<std::pair<size_t, size_t>> ranges;
    std::vector<size_t> use_locations;

    size_t vreg_id{0};

    RegAssignment reg{RegAssignment::UNINIT};
    size_t assign_index{0};
    bool split_off{false};
    
    void push_range(std::pair<size_t, size_t>);
    void push_loop_range(std::pair<size_t, size_t>);

    bool covers(size_t position) const;
    auto next_intersection(const LiveInterval& other) const -> std::optional<size_t>;
    LiveInterval split_at(size_t pos);
    size_t next_alive_after(size_t pos) const;
    size_t next_use_after(size_t pos) const;
    
    friend bool operator<(const LiveInterval& lhs, const LiveInterval& rhs);
    friend bool operator>(const LiveInterval& lhs, const LiveInterval& rhs);
    friend bool operator==(const LiveInterval& lhs, const LiveInterval& rhs);
    friend std::ostream& operator<<(std::ostream& os, const LiveInterval& interval);
    friend ::std::hash<LiveInterval>;
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

    std::vector<std::pair<Operand, Operand>> resolution;
};

struct Function {
    std::vector<BasicBlock> blocks;
    size_t virt_reg_count;
    size_t parameter_count;
    
    std::vector<Operand> clobbered_regs;
    
    auto compute_live_intervals(const std::vector<std::pair<size_t, size_t>>& block_ranges) -> std::vector<LiveInterval>;
    auto allocate_registers() -> std::vector<LiveInterval>;
};


struct Program {
    std::vector<Function> functions;
    std::vector<Value> immediates;
    int num_globals;
};

}; // namespace IR