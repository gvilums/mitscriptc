#pragma once

#include <map>
#include <string>

#include "types.h"

struct RecordCell;
struct RefCell;
struct ClosureRef;

using ProgVal = std::variant<None, int, bool, std::string, RecordCell, ClosureRef>;
using StackVal = std::variant<ProgVal, RefCell, struct Function*>;


enum class FnType {
    DEFAULT,
    PRINT,
    INPUT,
    INTCAST
};

struct RecordCell {
    // pointer to hashmap holding record state
    std::map<std::string, ProgVal>* internal;
};

struct RefCell {
    ProgVal* ref;
};

struct Closure {
    FnType type;
    struct Function* fn;
    std::vector<RefCell> refs;
};

struct ClosureRef {
	Closure* closure;
};


ProgVal value_from_constant(Constant c);
std::string value_to_string(ProgVal v);
bool value_eq(ProgVal l, ProgVal r);
