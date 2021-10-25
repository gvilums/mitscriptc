#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <limits>

// #include "../gc/gc.h"
#include "types.h"
#include "value.h"

#include "allocator.h"

namespace VM {

class VirtualMachine {
   private:
    struct Function* source;
    TrackedMap globals;
    std::vector<Value, Allocation::TrackingAlloc<Value>> opstack;
    std::vector<Value, Allocation::TrackingAlloc<Value>> arg_stage;

    size_t heap_size{0};
    size_t max_heap_size{std::numeric_limits<size_t>::max()};

    size_t base_index = 0;
    size_t iptr = 0;
    size_t num_locals = 0;
    struct Function* ctx;

    auto get_unary_op() -> Value;
    auto get_binary_ops() -> std::pair<Value, Value>;
    void reset();

    HeapObject* heap_head{nullptr};

    template <typename T>
    auto alloc(T t) -> HeapObject*;
    void gc_collect();
    void gc_check();

   public:
    VirtualMachine(struct Function* prog);
    VirtualMachine(struct Function* prog, size_t heap_limit);
    VirtualMachine(const VirtualMachine&) = delete;
    auto operator=(const VirtualMachine&) -> VirtualMachine& = delete;

    void exec();
    auto step() -> bool;
};
}  // namespace VM
