#pragma once

#include "x86.h"
#include "ir.h"
#include "regalloc.h"

namespace codegen {

/*
 * calling conventions:
 * pointer to current function resides in rdi (this is fixed, although this could be made dynamic in theory)
 * -> should be removed from register allocator consideration
 */

struct CodeGenState {
    std::vector<asmjit::Label> function_labels;
    asmjit::Label function_address_base_label;
    size_t current_stack_args{0};
};

class CodeGenerator {
    IR::Program program;
    asmjit::x86::Assembler assembler;

    std::vector<asmjit::Label> function_labels;
    asmjit::Label function_address_base_label;
    size_t current_stack_args{0};

   public:
    CodeGenerator(IR::Program&& program1, asmjit::CodeHolder* code_holder);

    void process_block(const IR::Function& func, size_t block_index, std::vector<asmjit::Label>& block_labels);
    void process_function(size_t func_index);

    void load(const asmjit::x86::Gp& reg, const IR::Operand& op);
    void store(const IR::Operand& op, const asmjit::x86::Gp& reg);
};

class Executable {
    asmjit::JitRuntime jit_rt;
    runtime::ProgramContext* ctx_ptr;
    int (*function)(){nullptr};

   public:
    explicit Executable(IR::Program&& program1);
    void run();

};

auto get_block_dfs_order(const IR::Function& func) -> std::vector<size_t>;
auto to_reg(size_t reg_index) -> asmjit::x86::Gp;
auto to_mem(int32_t stack_slot) -> asmjit::x86::Mem;

};