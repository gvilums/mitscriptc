#pragma once

#include "asmjit.h"
#include "ir.h"

struct Executable {
    asmjit::JitRuntime jit_rt;
    std::unique_ptr<runtime::Runtime> program_context;
    int (* code)(void){nullptr};

    void run();
};

struct CodeGenerator {
    IR::Program program;
    asmjit::CodeHolder code;
    asmjit::JitRuntime jit_rt;
    asmjit::x86::Assembler assembler;

    std::vector<asmjit::Label> function_labels;

    CodeGenerator(IR::Program program1);

    void process_function(size_t index);
    auto generate() -> Executable;
};