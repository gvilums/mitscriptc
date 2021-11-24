#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "AST.h"
#include "MITScript.h"
#include "antlr4-runtime.h"
#include "compiler.h"
#include "parsercode.h"
#include "irprinter.h"
#include "ir.h"
#include "regalloc.h"
#include "dead_code_remover.h"
#include "const_propagator.h"
#include "codegen.h"


auto main(int argc, const char* argv[]) -> int {
    std::ifstream file;
    if (argc == 1) {
        file.open("../inputs/test.mit");
    } else if (argc == 2) {
        file.open(argv[1]);
    } else {
        std::cout << "Usage: mitscript <filename>\n";
        return 1;
    }

    if (!file.is_open()) {
        std::cout << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }

    antlr4::ANTLRInputStream input(file);
    lexer::MITScript lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    tokens.fill();

    AST::Program* program = Program(tokens);
    if (program == nullptr) {
        std::cout << "Parsing failed" << std::endl;
        return 1;
    }
    
    Compiler compiler;
    program->accept(compiler);
   	IR::Program* prog = compiler.get_program();

//    std::cout << *prog << std::endl;

//    DeadCodeRemover dc_opt(prog);
//    prog = dc_opt.optimize();

//    try {
//        ConstPropagator c_prop(prog);
//        prog = c_prop.optimize();
//    } catch (const std::string& e) {
//        std::cout << "ERROR: " << e << std::endl;
//    }

    std::cout << *prog << std::endl;

    for (auto& func : prog->functions) {
        IR::allocate_registers(func);
    }
//    pretty_print_function(std::cout, prog->functions.back()) << std::endl;
//    IR::allocate_registers(prog->functions.back());

    std::cout << *prog << std::endl;

    codegen::Executable compiled(std::move(*prog));
    compiled.run();

    // std::cout << *prog << std::endl;
    
    // IR::Function& func = prog->functions[prog->functions.size() - 2];
    // pretty_print_function(std::cout, func) << std::endl;
    // IR::allocate_registers(func);
    // pretty_print_function(std::cout, func) << std::endl;
    
    delete program;
    return 0;
}
