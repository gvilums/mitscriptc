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
    struct StackFrame {
        struct Function* ctx;
        size_t iptr;
        size_t num_locals;
    };

    struct Function* source;
    std::vector<StackFrame> callstack;
    std::map<std::string, Value> globals;
    std::vector<Value> opstack;
    size_t base_index = 0;

   public:
    VM(struct Function* prog);
    VM(const VM&) = delete;
    auto operator=(const VM&) -> VM& = delete;

    void exec();
    auto step() -> bool;
    auto get_unary_op() -> Value;
    auto get_binary_ops() -> std::pair<Value, Value>;
};
