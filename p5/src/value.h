#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

namespace runtime {
    
struct ProgramContext;

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

const int32_t BOOL_TAG = static_cast<int32_t>(ValueType::Bool);
const int32_t INT_TAG = static_cast<int32_t>(ValueType::Int);
const int32_t INLINE_STRING_TAG = static_cast<int32_t>(ValueType::InlineString);
const int32_t HEAP_STRING_TAG = static_cast<int32_t>(ValueType::HeapString);
const int32_t RECORD_TAG = static_cast<int32_t>(ValueType::Record);
const int32_t CLOSURE_TAG = static_cast<int32_t>(ValueType::Closure);
const int32_t REFERENCE_TAG = static_cast<int32_t>(ValueType::Reference);

bool is_heap_type(ValueType type) {
    return type == ValueType::HeapString || type == ValueType::Record
           || type == ValueType::Closure || type == ValueType::Reference;
}

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

struct ValueEq {
    bool operator()(const Value& lhs, const Value& rhs) const noexcept;
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
    std::unordered_map<Value, Value, ValueHash, ValueEq> fields;
};

auto value_get_type(Value val) -> ValueType;
auto value_get_bool(Value val) -> bool;
auto value_get_int32(Value val) -> int;
auto value_get_ref(Value val) -> Value*;
auto value_get_string_ptr(Value val) -> String*;
auto value_get_record(Value val) -> Record*;
auto value_get_closure(Value val) -> Closure*;
auto value_get_std_string(Value val) -> std::string;
auto value_eq_bool(Value lhs, Value rhs) -> bool;

// checks types
Value value_add(ProgramContext* rt, Value lhs, Value rhs);
// below assume correct types
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

Value value_to_string(ProgramContext* rt, Value val);

Value to_value(bool b);
Value to_value(int32_t i);
Value to_value(ProgramContext* rt, const std::string& str);
Value to_value(ProgramContext* rt, const char* str);
Value to_value(Value* ref);
Value to_value(String* str);
Value to_value(Record* rec);
Value to_value(Closure*);

struct HeapObj {
    uint8_t region;
    uint64_t data[];
};

struct ProgramContext {
    void* region0{nullptr};
    void* region1{nullptr};

    // start with region 0
    uint8_t current_region{0};

    // track allocation amount
    size_t current_alloc{0};
    size_t next_alloc{0};

    Value none_string{0};
    Value false_string{0};
    Value true_string{0};
    Value function_string{0};

    Value* globals{nullptr};

    uint64_t saved_rsp{0};

    ProgramContext();
    ~ProgramContext();
    
    auto alloc_ref() -> Value*;
    auto alloc_string(size_t length) -> String*;
    auto alloc_record() -> Record*;
    auto alloc_closure(size_t num_free) -> Closure*;
    
    auto alloc_tracked(size_t data_size) -> HeapObject*;
    void dealloc_tracked(HeapObject* obj);

    void init_globals(size_t num_globals);
    
    void collect();
};

void extern_print(Value val);
auto extern_intcast(Value val) -> Value;
auto extern_input(ProgramContext* rt) -> Value;

auto extern_rec_load_name(Value rec, Value name) -> Value;
void extern_rec_store_name(Value rec, Value name, Value val);
auto extern_rec_load_index(ProgramContext* rt, Value rec, Value index_val) -> Value;
void extern_rec_store_index(ProgramContext* rt, Value rec, Value index_val, Value val);

Value extern_alloc_ref(ProgramContext* rt);
Value extern_alloc_string(ProgramContext* rt, size_t length);
Value extern_alloc_record(ProgramContext* rt);
Value extern_alloc_closure(ProgramContext* rt, size_t num_free);


void trace_value(ProgramContext* ctx, Value* ptr) {
    auto type = value_get_type(*ptr);
    if (is_heap_type(type)) {
        auto* heap_obj = reinterpret_cast<HeapObj*>((*ptr & DATA_MASK) - sizeof(HeapObj));
        if (heap_obj->region != ctx->current_region) {
            // if moved, new value is in data field
            //! IMPORTANT note: data already has tag bit set
            *ptr = heap_obj->data[0];
            return;
        }
        // heap object has not been moved
        if (type == ValueType::Reference) {

        } else if (type == ValueType::HeapString) {

        } else if (type == ValueType::Record) {

        } else if (type == ValueType::Closure) {

        }
    }
}

void trace_collect(ProgramContext* ctx, const uint64_t* rbp, uint64_t* rsp) {
    // TODO CHECK
    // base rbp is pointing two slots above saved rsp on stack
    auto* base_rbp = reinterpret_cast<uint64_t*>(ctx->saved_rsp - 16);
    while (rbp != base_rbp) {
        while (rsp != rbp) {
            trace_value(ctx, rsp);
            rsp -= 1;
        }
        rbp = reinterpret_cast<uint64_t*>(*rsp);
        rsp -= 2;
    }
    ctx->switch_region();
}

};
