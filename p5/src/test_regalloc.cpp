#include <iostream>

#include "ir.h"

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

    auto res = test_fn.compute_live_intervals();
    debug_live_intervals(res);

    return 0;
}