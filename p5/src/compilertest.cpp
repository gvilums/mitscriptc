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
#include "shape_analysis.h"

struct Arguments {
    std::string filename{"../inputs/test.mit"};
    size_t memory_limit{40 * (1 << 20)};
    bool use_const_propagation{false};
    bool use_dead_code_removal{false};
    bool use_type_inference{false};
    bool use_shape_analysis{false};
    bool emit_ir{false};

    Arguments(int argc, const char* argv[]) {
        int i = 1;
        while (i < argc) {
            std::string arg{argv[i]};
            i += 1;
            if (arg == "--opt=all") {
                use_const_propagation = true;
                use_dead_code_removal = true;
                use_type_inference = true;
                use_shape_analysis = true;
            } else if (arg == "--opt=constant-prop") {
                use_const_propagation = true;
            } else if (arg == "--opt=dead-code-rm") {
                use_dead_code_removal = true;
            } else if (arg == "--opt=shape-analysis") {
                use_shape_analysis = true;
            } else if (arg == "--opt=type-inference") {
                use_type_inference = true;
            } else if (arg == "-mem") {
                assert(i < argc);
                memory_limit = std::stol(argv[i]) * (1 << 20);
                i += 1;
            } else if (arg == "--emit-code") {
                emit_ir = true;
            } else if (arg == "-s") {
                assert(i < argc);
                filename = argv[i];
                i += 1;
            } else {
                // allow specifying filename without specific arg
                filename = arg;
            }
        }
    }
};

auto main(int argc, const char* argv[]) -> int {
    Arguments args(argc, argv);
    std::ifstream file(args.filename);

    if (!file.is_open()) {
        std::cout << "Failed to open file" << std::endl;
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
    
    Compiler compiler(args.memory_limit);
    program->accept(compiler);
   	IR::Program* prog = compiler.get_program();

    if (args.use_const_propagation) {
        try {
            ConstPropagator c_prop(prog);
            prog = c_prop.optimize();
        } catch (const std::string& e) {
            std::cout << "ERROR: " << e << std::endl;
        }
    }

    if (args.use_dead_code_removal) {
        DeadCodeRemover dc_opt(prog);
        prog = dc_opt.optimize();
    }

    // if (args.use_shape_analysis) {
        ShapeAnalysis sa_opt(prog);
        prog = sa_opt.optimize();
    // }

    std::cout << *prog << std::endl;

//    pretty_print_function(std::cout, prog->functions[3]) << std::endl;
    /*for (auto& func : prog->functions) {
        IR::allocate_registers(func);
    }
//    pretty_print_function(std::cout, prog->functions.back()) << std::endl;
//    IR::allocate_registers(prog->functions.back());

//    std::cout << *prog << std::endl;


    codegen::Executable compiled(std::move(*prog));
    try {
        compiled.run();
    } catch (codegen::RuntimeException& exception) {
        std::cout << exception << std::endl;
        return 1;
    }*/

    // std::cout << *prog << std::endl;
    
    // IR::Function& func = prog->functions[prog->functions.size() - 2];
    // pretty_print_function(std::cout, func) << std::endl;
    // IR::allocate_registers(func);
    // pretty_print_function(std::cout, func) << std::endl;
    
    delete program;
    return 0;
}
