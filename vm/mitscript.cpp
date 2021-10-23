#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>

#include "AST.h"
#include "MITScript.h"
#include "antlr4-runtime.h"
#include "bc-lexer.h"
#include "bc-parser.h"
#include "compiler.h"
#include "parsercode.h"
#include "vm.h"

auto main(int argc, const char* argv[]) -> int {
    std::ios_base::sync_with_stdio(false);
    auto mem_limit = std::numeric_limits<size_t>::max();

    std::string flag;
    std::string filename;
    if (argc == 3) {
        flag = argv[1];
        filename = argv[2];
    } else if (argc == 5) {
        const size_t megabyte = 1000000;
        mem_limit = std::atoi(argv[2]) * megabyte;
        flag = argv[3];
        filename = argv[4];
    } else {
        std::cout << "Usage: mitscript [-mem N] <type> <filename>\n";
        return 1;
    }

    Compiler compiler;
    struct Function* fn;
    if (flag == "-s") {
        std::ifstream file;
        file.open(filename);
        if (!file.is_open()) {
            std::cout << "Failed to open file: " << argv[2] << "\n";
            return 1;
        }
        antlr4::ANTLRInputStream input(file);
        MITScript lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);

        tokens.fill();

        AST::Program* program = Program(tokens);
        if (program == nullptr) {
            std::cout << "ERROR: parse error" << std::endl;
            return 1;
        }

        program->accept(compiler);
        fn = compiler.get_function();
    } else if (flag == "-b") {
        std::FILE* file = std::fopen(filename.c_str(), "r");
        if (file == nullptr) {
            std::cout << "Failed to open file: " << argv[2] << std::endl;
            return 1;
        }
        void* scanner;
        bclex_init(&scanner);
        bcset_in(file, scanner);

        if (bcparse(scanner, fn) == 1) {
            cout << "ERROR: parse error" << endl;
            return 1;
        }
    } else {
        std::cout << "ERROR: invalid flag" << std::endl;
        return 1;
    }

    try {
        // temporary
        VM::VirtualMachine vm(fn, mem_limit);
        vm.exec();
    } catch (std::string s) {
        std::cout << s << std::endl;
        return 1;
    } catch (...) {
        std::cout << "ERROR: runtime exception" << std::endl;
        return 1;
    }

    return 0;
}
