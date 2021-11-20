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

struct Executable {
    asmjit::JitRuntime jit_rt;
    runtime::Runtime* program_context;
    int (*code)(){nullptr};

    Executable(asmjit::JitRuntime rt, std::unique_ptr<runtime::Runtime> ctx)
        : jit_rt{std::move(rt)}, program_context{std::move(ctx)};

    void run();
};

struct CodeGenerator {
    CodeGenerator(IR::Program program1, runtime::Runtime* ctx);
    IR::Program program;
    asmjit::CodeHolder code;
    asmjit::JitRuntime jit_rt;
    asmjit::x86::Assembler assembler;
    runtime::Runtime* program_context;

    std::vector<asmjit::Label> function_labels;
    asmjit::Label globals_label;
    asmjit::Label function_address_label;

    CodeGenerator(IR::Program program1, std::unique_ptr<runtime::Runtime> ctx);

    void process_block(const IR::Function& func, size_t block_index, std::vector<asmjit::Label>& block_labels);
    void process_function(size_t func_index);
    auto generate() -> Executable;

    void load(const asmjit::x86::Gp& reg, const IR::Operand& op);
    void store(const IR::Operand& op, const asmjit::x86::Gp& reg);

};

auto get_block_dfs_order(const IR::Function& func) -> std::vector<size_t>;
auto to_reg(size_t reg_index) -> asmjit::x86::Gp;
auto to_mem(size_t stack_slot) -> asmjit::x86::Mem;

};