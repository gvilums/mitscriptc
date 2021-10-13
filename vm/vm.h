#include "types.h"
#include <string>
#include <memory>
#include <variant>
#include <map>
#include <vector>

struct Record {
    // pointer to hashmap holding record state
    std::map<std::string, Value>* internal;
};

struct RefCell {
    Value* ref;
};

struct FnPtr {
    Function* fn;
};

enum class BuiltinFn {
    PRINT,
    INPUT,
    TO_STRING
};

using Value = std::variant<None, bool, int, std::string, Record, RefCell, FnPtr, BuiltinFn>;

class VM {
private:
    struct StackFrame {
        Function* ctx;
        std::vector<Value> opstack;
        size_t iptr;
    };

    Function source;
    std::vector<StackFrame> callstack;

public:
    VM(Function&& prog);
    VM(const VM&) = delete;
    VM& operator=(const VM&) = delete;

    void exec();
    bool step();
};