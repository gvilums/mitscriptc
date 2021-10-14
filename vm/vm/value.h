#pragma once

#include <map>
#include <string>

#include "../types.h"

struct Record;
struct RefCell;
struct Closure;

using ProgVal = std::variant<None, int, bool, std::string, Record, Closure>;
using StackVal = std::variant<ProgVal, RefCell, Function*>;


enum class BuiltinFn {
    PRINT,
    INPUT,
    TO_STRING
};

struct Record {
    // pointer to hashmap holding record state
    std::map<std::string, ProgVal>* internal;
};

struct RefCell {
    ProgVal* ref;
};

struct Closure {
    Function* fn;
    std::vector<RefCell> refs;
};

ProgVal value_from_constant(Constant c);
std::string value_to_string(ProgVal v);
bool value_eq(ProgVal l, ProgVal r);