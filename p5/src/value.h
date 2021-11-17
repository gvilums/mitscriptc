#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

namespace runtime {
    
struct Runtime;

enum class ValueType : uint64_t {
    None,
    Bool,
    Int,
    InlineString,
    HeapString,
    Record,
    Closure,
    Reference,
};

/*
    3 lower bits are used to store type. possibilities are:
    000 - None
    001 - Bool
    010 - Int
    011 - Inline String
    100 - Heap String
    101 - Record
    110 - Closure
    111 - Reference
    
    Formats:
    None            - 0000...0000
    Bool            - 0000...[bool 0/1]0001
    Int             - 0000... [int32_t 32 x 0/1]0010
    Inline String   - [char data[7] 56 x 0/1][length 4 x 0/1]0011
    Pointer Based   - ptr | 0000...0100
*/
using Value = std::uint64_t;

struct ValueHash {
    std::size_t operator()(const Value& val) const noexcept;
};

const std::uint64_t TAG_MASK = 0b111;
const std::uint64_t DATA_MASK = ~TAG_MASK;


struct String {
    std::uint64_t len;
    char data[];
};

struct Closure {
    std::uint64_t fnptr;
    std::uint64_t n_args;
    std::uint64_t n_free_vars;
    Value free_vars[];
};

struct Record {
    std::unordered_map<Value, Value, ValueHash> entries;
};

auto value_get_type(Value val) -> ValueType;
auto value_get_bool(Value val) -> bool;
auto value_get_int32(Value val) -> int;
auto value_get_ref(Value val) -> Value*;
auto value_get_string_ptr(Value val) -> String*;
auto value_get_record(Value val) -> Record*;
auto value_get_closure(Value val) -> Closure*;

// checks types
Value value_add(Value lhs, Value rhs);

// below assume integer values
Value value_add_int32(Value lhs, Value rhs);
Value value_sub(Value lhs, Value rhs);
Value value_mul(Value lhs, Value rhs);
Value value_div(Value lhs, Value rhs);
Value value_eq(Value lhs, Value rhs);
Value value_geq(Value lhs, Value rhs);
Value value_gt(Value lhs, Value rhs);
Value value_and(Value lhs, Value rhs);
Value value_or(Value lhs, Value rhs);
Value value_not(Value val);

Value value_to_string(Value val);
auto value_to_std_string(Value val) -> std::string;

Value to_value(bool b);
Value to_value(int32_t i);
Value to_value(const std::string& str, Runtime& alloc);
Value to_value(const std::string& str);
Value to_value(const char* str);
Value to_value(Value* ref);
Value to_value(String* str);
Value to_value(Record* rec);
Value to_value(Closure*);

struct HeapObject {
    HeapObject* next{nullptr};
    enum {
        REF,
        STR,
        REC,
        CLOSURE,
    } type;
    bool marked{false};
    std::uint64_t data[];
};

struct Runtime {
    size_t total_alloc{0};
    HeapObject* heap_head{nullptr};
    
    Value none_string;
    Value false_string;
    Value true_string;
    Value function_string;
    
    Runtime();
    
    auto alloc_ref() -> Value*;
    auto alloc_string(size_t length) -> String*;
    auto alloc_record() -> Record*;
    auto alloc_closure(size_t num_free) -> Closure*;
    
    auto alloc_tracked(size_t data_size) -> HeapObject*;
    void dealloc_tracked(HeapObject* obj);
    
    void collect();
};

auto global_runtime() -> Runtime&;

};
