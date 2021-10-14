#pragma once

#include <string>
#include <memory>
#include <variant>
#include <vector>
#include <map>

#include "../types.h"
#include "value.h"


class VM {
private:
    struct StackFrame {
        Function* ctx;
        std::vector<StackVal> opstack;
        std::vector<ProgVal> locals;
        std::vector<RefCell> local_ref_vars;
        std::vector<RefCell> free_vars;
        size_t iptr;
        
        StackFrame(Function* fn, std::vector<RefCell> closure_free_vars) {
            this->ctx = fn;
            this->free_vars = closure_free_vars;
            for (size_t i = 0; i < fn->local_vars_.size(); ++i) {
                this->locals.push_back(None{});
            }
            for (size_t i = 0; i < fn->local_reference_vars_.size(); ++i) {
                ProgVal* ptr = new ProgVal();
                this->local_ref_vars.push_back(RefCell{.ref=ptr});
            }
        }
    };

    Function source;
    std::vector<StackFrame> callstack;
    std::map<std::string, ProgVal> globals;

public:
    VM(Function&& prog);
    VM(const VM&) = delete;
    VM& operator=(const VM&) = delete;

    void exec();
    bool step();
};