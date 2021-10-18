#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "types.h"
#include "value.h"

class VM {
   private:
    struct Function* source;
    std::map<std::string, Value> globals;
    std::vector<Value> opstack;
    std::vector<Value> arg_stage;

    size_t base_index = 0;
    size_t iptr = 0;
    size_t num_locals = 0;
    struct Function* ctx;

    auto get_unary_op() -> Value;
    auto get_binary_ops() -> std::pair<Value, Value>;
    void reset();

   public:
    VM(struct Function* prog);
    VM(const VM&) = delete;
    auto operator=(const VM&) -> VM& = delete;

    void exec();
    auto step() -> bool;
};
