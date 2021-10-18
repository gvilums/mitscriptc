#pragma once

#include <map>
#include <string>

#include "types.h"

struct RecordRef;
struct ValueRef;
struct ClosureRef;

// possible values on the program stack
using Value = std::variant<None, int, bool, std::string, RecordRef, ClosureRef, ValueRef, struct Function*, size_t>;

enum class FnType { DEFAULT,
                    PRINT,
                    INPUT,
                    INTCAST };

struct RecordRef {
    // pointer to hashmap holding record state
    std::map<std::string, Value>* internal;
};

struct ValueRef {
    Value* ref;
};

struct Closure {
    FnType type;
    struct Function* fn;
    std::vector<ValueRef> refs;
};

struct ClosureRef {
    Closure* closure;
};

auto value_from_constant(Constant c) -> Value;
auto value_to_string(Value v) -> std::string;
auto value_eq(Value l, Value r) -> bool;
