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
        std::vector<RefCell> refs;
        size_t iptr;
        
        StackFrame(Function* fn, std::vector<ProgVal>&& params, const std::vector<RefCell>& free_vars) {
            size_t nparams = params.size();
            this->ctx = fn;
            this->iptr = 0;
            this->locals = params;
            for (size_t i = 0; i < fn->local_vars_.size() - nparams; ++i) {
                this->locals.push_back(None{});
            }
            for (size_t i = 0; i < fn->local_reference_vars_.size(); ++i) {
                ProgVal* ptr = new ProgVal();
                this->refs.push_back(RefCell{.ref=ptr});
            }
            for (size_t i = 0; i < free_vars.size(); ++i) {
                this->refs.push_back(free_vars[i]);
            }
        }
        StackFrame(Function* fn) {
            this->ctx = fn;
            this->iptr = 0;
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