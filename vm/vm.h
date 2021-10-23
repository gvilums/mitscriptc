#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../gc/gc.h"
#include "types.h"
#include "value.h"

namespace VM {
class VirtualMachine {
   private:
    struct Function* source;
    std::map<std::string, Value> globals;
    std::vector<Value> opstack;
    std::vector<Value> arg_stage;

    size_t heap_size;

    size_t base_index = 0;
    size_t iptr = 0;
    size_t num_locals = 0;
    struct Function* ctx;

    auto get_unary_op() -> Value;
    auto get_binary_ops() -> std::pair<Value, Value>;
    void reset();
    
    HeapObject* heap_head{nullptr};

    template<typename T>
    auto alloc(T t) -> HeapObject*;
    void gc_collect();

   public:
    VirtualMachine(struct Function* prog);
    VirtualMachine(const VirtualMachine&) = delete;
    auto operator=(const VirtualMachine&) -> VirtualMachine& = delete;

    void exec();
    auto step() -> bool;
};
}  // namespace VM
