#include "ir.h"

namespace IR {

Program::Program() {
    this->rt = new runtime::Runtime;
}
Program::~Program() {
    delete this->rt;
}
Program::Program(Program&& other) noexcept {
    this->rt = other.rt;
    other.rt = nullptr;
    this->num_globals = other.num_globals;
    this->functions = std::move(other.functions);
    this->immediates = std::move(other.immediates);
}

auto Function::split_edge(size_t from, size_t to) -> BasicBlock& {
    size_t new_block_index = this->blocks.size();
    this->blocks.push_back({{}, {}, {from}, {to}});
    for (size_t& succ : this->blocks[from].successors) {
        if (succ == to) {
            succ = new_block_index;
        }
    }
    for (size_t& pred : this->blocks[to].predecessors) {
        if (pred == from) {
            pred = new_block_index;
        }
    }
    for (auto& phi_node : this->blocks[to].phi_nodes) {
        for (auto& [pred, op] : phi_node.args) {
            if (pred == from) {
                pred = new_block_index;
            }
        }
    }
    return this->blocks.back();
}

auto Operand::get_machine() const -> std::optional<MachineReg> {
    if (this->type == MACHINE_REG) {
        return static_cast<MachineReg>(this->index);
    }
    return std::nullopt;
}
};