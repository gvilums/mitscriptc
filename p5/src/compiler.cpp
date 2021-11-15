#include <cassert>
#include <fstream>
#include <iostream>

#include "AST.h"
#include "MITScript.h"
#include "antlr4-runtime.h"
#include "compiler.h"
#include "parsercode.h"
#include "irprinter.h"


auto main(int argc, const char* argv[]) -> int {
    if (argc != 2) {
        std::cout << "Usage: mitscript <filename>\n";
        return 1;
    }

    std::ifstream file;
    file.open(argv[1]);

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
    
    std::cout << *prog << std::endl;
    
    pretty_print_function(std::cout, prog->functions.back()) << std::endl;
  	prog->functions.back().allocate_registers();
    
    delete program;
    return 0;
}
