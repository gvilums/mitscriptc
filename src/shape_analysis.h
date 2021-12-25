#pragma once

#include "value.h"
#include "ir.h"
#include <map>
#include <string>

struct istruct {
    bool is_unknown;
    bool maybe_none;
    std::map<std::string, istruct*> fields;
};

class ShapeAnalysis {
private:
    IR::Program* prog_;
public:
    ShapeAnalysis(IR::Program* prog);
    IR::Program* optimize();
    
    int get_phi_node_reg(size_t fun_idx, size_t block_idx, size_t phi_node_idx, size_t arg_idx, std::map<std::pair<size_t, int>, int> &known_regs);
    void infer_structs();
};