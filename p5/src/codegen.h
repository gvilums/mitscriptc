#pragma once

#include <ostream>
#include "ir.h"
#include "regalloc.h"
#include "x86.h"

namespace codegen {

/*
 * return value indicator table
 * 0 - OK
 * 1 - UninitializedVariableException
 * 2 - IllegalCastException
 * 3 - IllegalArithmeticException
 * 4 - RuntimeException
 */

class RuntimeException {
    int type{0};


   public:
    explicit RuntimeException(int kind);
    friend std::ostream& operator<<(std::ostream& os, const RuntimeException& exception);
};

class CodeGenerator {
    const IR::Program& program;
    asmjit::x86::Assembler assembler;

    std::vector<int32_t> layout_offsets;
    std::vector<asmjit::Label> function_labels;
    asmjit::Label layout_base_label;
    asmjit::Label uninit_var_label, illegal_cast_label, illegal_arith_label, rt_exception_label;

    int current_args{0};

    void process_instruction(const IR::Instruction& instr);
    void process_block(const IR::Function& func, size_t block_index, std::vector<asmjit::Label>& block_labels);
    void process_function(size_t func_index);

    void load(const asmjit::x86::Gp& reg, const IR::Operand& op);
    void store(const IR::Operand& op, const asmjit::x86::Gp& reg);

    void generate_prelude();
    void save_volatile();
    void restore_volatile();
    void init_labels();

   public:
    CodeGenerator(const IR::Program& program1, asmjit::CodeHolder* code_holder);
};

class Executable {
    asmjit::JitRuntime jit_rt;
    runtime::ProgramContext* ctx_ptr;
    int (*function)(){nullptr};

   public:
    explicit Executable(IR::Program program1);
    ~Executable();
    void run();

};

auto get_block_dfs_order(const IR::Function& func) -> std::vector<size_t>;
auto to_reg(size_t reg_index) -> asmjit::x86::Gp;
auto to_mem(int32_t stack_slot) -> asmjit::x86::Mem;

};