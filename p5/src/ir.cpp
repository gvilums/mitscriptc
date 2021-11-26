#include <memory>

#include "ir.h"

namespace IR {

Program::Program(size_t heap_size) {
    this->ctx_ptr = new runtime::ProgramContext(heap_size);
}
Program::~Program() {
    delete this->ctx_ptr;
}
Program::Program(Program&& other) noexcept {
    this->ctx_ptr = other.ctx_ptr;
    other.ctx_ptr = nullptr;
    this->num_globals = other.num_globals;
    this->functions = std::move(other.functions);
    this->immediates = std::move(other.immediates);
}

auto Function::split_edge(int32_t from, int32_t to) -> BasicBlock& {
    auto new_block_index = (int)this->blocks.size();
    this->blocks.push_back({{}, {}, {from}, {to}});
    for (int32_t& succ : this->blocks[from].successors) {
        if (succ == to) {
            succ = new_block_index;
        }
    }
    for (int32_t& pred : this->blocks[to].predecessors) {
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

};