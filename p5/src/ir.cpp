#include "ir.h"

namespace IR {

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

};