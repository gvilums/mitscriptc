#include <iostream>

#include "ir.h"
#include "regalloc.h"
#include "irprinter.h"

int main() {
    using namespace IR;

    Function test_fn{
        {
            BasicBlock{
                {},
                {
                    Instruction{
                        Operation::MOV,
                        Operand{Operand::VIRT_REG, 0},
                        {Operand{Operand::IMMEDIATE, 0}},
                    },
                    Instruction{
                        Operation::MOV,
                        Operand{Operand::VIRT_REG, 1},
                        {Operand{Operand::IMMEDIATE, 0}},
                    },
                },
                {},
                {1},
            },
            BasicBlock{
                {
                    PhiNode{
                        Operand{Operand::VIRT_REG, 2},
                        {{0, Operand{Operand::IMMEDIATE, 0}}, {2, Operand{Operand::VIRT_REG, 4}}},
                    },
                    PhiNode{
                        Operand{Operand::VIRT_REG, 3},
                        {{0, Operand{Operand::VIRT_REG, 1}}, {2, Operand{Operand::VIRT_REG, 5}}},
                    },
                },
                {Instruction{
                    Instruction{
                        Operation::GT,
                        {},
                        {Operand{Operand::VIRT_REG, 3}, Operand{Operand::IMMEDIATE, 0}},
                    },
                }},
                {0},
                {2, 3},
                true,
                2,
            },
            BasicBlock{
                {},
                {
                    Instruction{
                        Operation::MUL,
                        Operand{Operand::VIRT_REG, 4},
                        {Operand{Operand::VIRT_REG, 2}, Operand{Operand::VIRT_REG, 3}},
                    },
                    Instruction{
                        Operation::SUB,
                        Operand{Operand::VIRT_REG, 5},
                        {Operand{Operand::VIRT_REG, 3}, Operand{Operand::IMMEDIATE, 0}},
                    },
                },
                {1},
                {1},
            },
            BasicBlock{
                {},
                {
                    Instruction{
                        Operation::PRINT,
                        {},
                        {Operand{Operand::VIRT_REG, 0}, Operand{Operand::VIRT_REG, 2}},
                    },
                },
                {1},
                {},
            },
        },
        6,
    };

    // pretty_print_function(std::cout, test_fn) << std::endl;
    // test_fn.allocate_registers();
    
    // Program prog{{test_fn}, {}, 0};
    // RegallocPass regalloc;
    // prog.apply_pass(regalloc);
    
    std::vector<std::pair<Operand, Operand>> mapping{
        {{Operand::VIRT_REG, 0}, {Operand::VIRT_REG, 1}},
        {{Operand::VIRT_REG, 1}, {Operand::VIRT_REG, 2}},
        {{Operand::VIRT_REG, 2}, {Operand::VIRT_REG, 0}},
        {{Operand::VIRT_REG, 3}, {Operand::VIRT_REG, 4}},
        {{Operand::VIRT_REG, 5}, {Operand::VIRT_REG, 3}},
    };
    
    auto instrs = mapping_to_instructions(mapping);
    for (const auto& instr : instrs) {
        std::cout << instr << std::endl;
    }
    

    return 0;
}