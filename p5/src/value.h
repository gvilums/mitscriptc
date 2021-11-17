#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

namespace runtime {

enum class ValueKind : uint64_t {
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

ValueKind value_get_kind(Value val);

bool value_get_bool(Value val);
int value_get_int32(Value val);
std::string value_get_std_string(Value val);
Value* value_get_ref(Value val);
String* value_get_string_ptr(Value val);
Record* value_get_record(Value val);
Closure* value_get_closure(Value val);

// checks types
Value value_add(Value lhs, Value rhs);

// below assume integer values
Value value_add_int32(Value lhs, Value rhs);
Value value_sub(Value lhs, Value rhs);
Value value_mul(Value lhs, Value rhs);
Value value_div(Value lhs, Value rhs);

Value value_to_string(Value val);

Value from_bool(bool b);
Value from_int32(int32_t i);
Value from_std_string(const std::string& str);
Value from_ref(Value* ref);
Value from_string_ptr(String* str);
Value from_record_ptr(Record* rec);
Value from_closure_ptr(Closure*);

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

struct AllocationContext {
    size_t total_alloc{0};
    
    Value none_string;
    Value false_string;
    Value true_string;
    Value function_string;
    
    AllocationContext();
    
    auto alloc_ref() -> Value*;
    auto alloc_string(size_t length) -> String*;
    auto alloc_record() -> Record*;
    auto alloc_closure(size_t num_free) -> Closure*;
    
    auto allocate(size_t n_bytes) -> void*;
    void deallocate(HeapObject* obj);
};

auto get_alloc() -> AllocationContext&;

};
