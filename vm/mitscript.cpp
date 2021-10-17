#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

#include "AST.h"
#include "MITScript.h"
#include "antlr4-runtime.h"
#include "bc-lexer.h"
#include "bc-parser.h"
#include "compiler.h"
#include "parsercode.cpp"
#include "vm.h"

int main(int argc, const char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    if (argc != 3) {
        std::cout << "Usage: mitscript <type> <filename>\n";
        return 1;
    }

    std::string flag{argv[1]};

    Compiler compiler;
    struct Function* fn;
    if (flag == "-s") {
        std::ifstream file;
        file.open(argv[2]);
        if (!file.is_open()) {
            std::cout << "Failed to open file: " << argv[2] << "\n";
            return 1;
        }
        antlr4::ANTLRInputStream input(file);
        MITScript lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);

        tokens.fill();

        AST::Program* program = Program(tokens);
        if (!program) {
            std::cout << "ERROR: parse error" << std::endl;
            return 1;
        }

        program->accept(compiler);
        fn = compiler.get_function();
    } else if (flag == "-b") {
        std::FILE* file = std::fopen(argv[2], "r");
        if (!file) {
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
        VM vm(fn);
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
