#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <limits>
#include <parallel_hashmap/phmap.h>
#include <unordered_map>
#include <iostream>

#include "types.h"
#include "allocator.h"

namespace VM {

class VirtualMachine;
class Value;
struct ValueHash;
struct HeapObj;
struct String;
struct Record;
struct Closure;

class Value {
   public:
    enum ValueTag {
        NONE,
        NUM,
        BOOL,
        STRING_PTR,
        RECORD_PTR,
        CLOSURE_PTR,
        VALUE_PTR,
        FN_PTR,
        USIZE
    };

   private:
    ValueTag tag{NONE};
    union {
        None none;
        int num;
        bool boolean;
        String* string;
        Record* record;
        Closure* closure;
        Value* value;
        struct Function* fnptr;
        size_t usize;
    };

   public:
    Value()
        : none{None{}} {}
    Value(None none_val)
        : none{None{}} {}
    Value(int n)
        : tag{NUM}, num{n} {}
    Value(bool b)
        : tag{BOOL}, boolean{b} {}
    Value(String* ptr)
        : tag{STRING_PTR}, string{ptr} {}
    Value(Record* ptr)
        : tag{RECORD_PTR}, record{ptr} {}
    Value(Closure* ptr)
        : tag{CLOSURE_PTR}, closure{ptr} {}
    Value(Value* ptr)
        : tag{VALUE_PTR}, value{ptr} {}
    Value(struct Function* ptr)
        : tag{FN_PTR}, fnptr{ptr} {}
    Value(size_t u)
        : tag{USIZE}, usize{u} {}
    
    static auto from_constant(Constant c, VirtualMachine& vm) -> Value;
    
    Value(const Value& other) = default;
    Value(Value&& other) = default;
    
    auto add(Value& other, VirtualMachine& vm) -> Value;
    void trace() const;
    
    auto operator=(const Value& other) -> Value& = default;
    auto operator=(Value&& other) noexcept -> Value& = default;

    friend auto operator==(const Value& lhs, const Value& rhs) -> bool;
    friend auto operator>=(Value const& lhs, Value const& rhs) -> bool;
    friend auto operator>(Value const& lhs, Value const& rhs) -> bool;

    auto to_string(VirtualMachine& vm) const -> String*;

    auto get_tag() -> ValueTag;
    auto get_bool() -> bool;
    auto get_int() -> int;
    auto get_string() -> String&;
    auto get_string_const() const -> const String&;
    auto get_record() -> Record&;
    auto get_closure() -> Closure&;
    auto get_val_ref() -> Value&;
    auto get_fnptr() -> struct Function*;
    auto get_usize() -> size_t;
    
    friend ValueHash;
};

struct String {
    size_t size;
    char data[];
    
    String(size_t len) : size{len} {}
    
    auto to_std_string() const -> std::string {
        std::string output{};
        for (size_t i = 0; i < this->size; ++i) {
            output.push_back(data[i]);
        }
        return output;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const String& str);
};
    
struct ValueHash {
    std::size_t operator()(const Value& val) const noexcept {
        if (val.tag == Value::STRING_PTR) {
            String* ptr = val.string;
            const size_t l = ptr->size;
            const size_t p = 1000000007;
            size_t current_p = p;
            size_t output = 0;
            for (size_t i = 0; i < l; ++i) {
                output += ptr->data[i] * current_p;
                current_p *= p;
            }
            return output;
        } 
        // TODO check if only hashing strings is ok
        return 0;
    }
};

struct HeapObj {
    HeapObj* next{nullptr};
    bool marked{false};
    
    size_t data[];
    
    HeapObj(HeapObj* next) : next{next} {}
};


struct Closure {
    using TrackedVec = std::vector<Value*, Allocation::TrackingAlloc<Value*>>;

    enum FnType {
        DEFAULT,
        PRINT,
        INPUT,
        INTCAST
    } type;
    struct Function* fn;
    TrackedVec refs;

    Closure(FnType type, struct Function* fn)
        : type{type}, fn{fn} {}
    Closure(FnType type, struct Function* fn, TrackedVec refs)
        : type{type}, fn{fn}, refs{std::move(refs)} {}
    
    void trace() const;
};

using TrackedMap = phmap::flat_hash_map<
    Value,
    Value,
    ValueHash,
    std::equal_to<Value>,
    Allocation::TrackingAlloc<std::pair<const Value, Value>>>;

struct Record {
    TrackedMap fields;
    
    void trace() const;
};

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
    
    Closure* print_closure;
    Closure* input_closure;
    Closure* intcast_closure;
    String* none_string;
    String* true_string;
    String* false_string;
    String* function_string;

    auto get_unary_op() -> Value;
    auto get_binary_ops() -> std::pair<Value, Value>;
    void reset();

    HeapObj* heap_head{nullptr};


    template<typename ...Args>
    auto alloc_closure(Args&&... args) -> Closure*;
    auto alloc_record() -> Record*;
    auto alloc_string(size_t len) -> String*;
    auto alloc_string(const std::string& str) -> String*;
    auto alloc_value() -> Value*;
    
    void gc_collect();
    void gc_check();

   public:
    VirtualMachine(struct Function* prog);
    VirtualMachine(struct Function* prog, size_t heap_limit);
    ~VirtualMachine();
    VirtualMachine(const VirtualMachine&) = delete;
    auto operator=(const VirtualMachine&) -> VirtualMachine& = delete;

    void exec();
    auto step() -> bool;
    
    friend Value;
};
}  // namespace VM
