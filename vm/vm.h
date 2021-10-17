#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "types.h"
#include "value.h"

class VM {
   private:
    struct StackFrame {
        struct Function* ctx;
        std::vector<StackVal> opstack;
        std::vector<ProgVal> locals;
        std::vector<RefCell> refs;
        size_t iptr;

        StackFrame(struct Function* fn,
                   std::vector<ProgVal>&& params,
                   const std::vector<RefCell>& free_vars) {
            size_t nparams = params.size();
            this->ctx = fn;
            this->iptr = 0;
            this->locals = params;
            this->refs = std::vector<RefCell>();
            this->opstack = std::vector<StackVal>();
            for (size_t i = 0; i < fn->local_vars_.size() - nparams; ++i) {
                this->locals.push_back(None{});
            }
            for (size_t i = 0; i < fn->local_reference_vars_.size(); ++i) {
                ProgVal* ptr = new ProgVal();
                std::string name = fn->local_reference_vars_.at(i);
                int j = -1;
                for (size_t k = 0; k < fn->local_vars_.size(); ++k) {
                    if (fn->local_vars_.at(k) == name) {
                        j = k;
                        break;
                    }
                }
                // a local ref var is also a local var
                if (j != -1) {
                    *ptr = this->locals.at(j);
                } else {
                    *ptr = None{};
                }
                this->refs.push_back(RefCell{.ref = ptr});
            }
            for (size_t i = 0; i < free_vars.size(); ++i) {
                this->refs.push_back(free_vars[i]);
            }
        }
        StackFrame(struct Function* fn) {
            this->ctx = fn;
            this->iptr = 0;
            this->refs = std::vector<RefCell>();
            this->locals = std::vector<ProgVal>();
            this->opstack = std::vector<StackVal>();
        }
    };

    struct Function* source;
    std::vector<StackFrame> callstack;
    std::map<std::string, ProgVal> globals;

   public:
    VM(struct Function* prog);
    VM(const VM&) = delete;
    VM& operator=(const VM&) = delete;

    void exec();
    bool step();
};
