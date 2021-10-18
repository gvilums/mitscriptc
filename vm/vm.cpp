#include "vm.h"

#include <algorithm>
#include <iostream>

#include "instructions.h"
#include "types.h"
#include "value.h"

auto VM::get_unary_op() -> Value {
    if (this->opstack.empty()) {
        throw std::string{"ERROR: empty stack for unary operation"};
    }
    Value operand = this->opstack.back();
    this->opstack.pop_back();
    return operand;
}

auto VM::get_binary_ops() -> std::pair<Value, Value> {
    size_t stack_size = this->opstack.size();
    if (stack_size < 2) {
        throw std::string{"ERROR: stack holds too few operands for binary operation"};
    }
    Value lop = this->opstack[stack_size - 2];
    Value rop = this->opstack[stack_size - 1];

    this->opstack.pop_back();
    this->opstack.pop_back();
    
    return std::pair{lop, rop};
}

VM::VM(struct Function* prog) : source(prog) { }

void VM::reset() {
    this->ctx = this->source;
    this->base_index = 0;
    this->iptr = 0;
    this->num_locals = 0;

    auto* print_closure = new Closure;
    *print_closure = Closure{.type = FnType::PRINT, .fn = nullptr};
    auto* input_closure = new Closure;
    *input_closure = Closure{.type = FnType::INPUT, .fn = nullptr};
    auto* intcast_closure = new Closure;
    *intcast_closure = Closure{.type = FnType::INTCAST, .fn = nullptr};

    this->globals = {
        { "print", ClosureRef{print_closure} },
        { "input", ClosureRef{input_closure} },
        { "intcast", ClosureRef{intcast_closure} }
    };
}

void VM::exec() {
    this->reset();
    while ((this->ctx != nullptr) && this->iptr < this->ctx->instructions.size()) {
        this->step();
    }
}

auto VM::step() -> bool {
    if (this->ctx == nullptr) {
        return false;
    }

    struct Function* fn = this->ctx;
    if (this->iptr >= fn->instructions.size()) {
        throw std::string{"instruction pointer overran instruction list"};
    }
    Instruction instr = fn->instructions[this->iptr];

    if (instr.operation == Operation::LoadConst) {
        Value v = value_from_constant(fn->constants_.at(instr.operand0.value()));
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadGlobal) {
        std::string name = fn->names_.at(instr.operand0.value());
        if (this->globals.find(name) == this->globals.end()) {
            throw std::string{"ERROR: uninitialized variable"};
        }
        this->opstack.push_back(this->globals.at(name));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadLocal) {
        this->opstack.push_back(this->opstack.at(this->base_index + instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadFunc) {
        this->opstack.emplace_back(fn->functions_.at(instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadReference) {
        if (this->opstack.empty()) {
            throw std::string{"ERROR: trying to load reference from empty stack"};
        }
        ValueRef ref_cell = std::get<ValueRef>(this->opstack.back());
        this->opstack.pop_back();
        this->opstack.push_back(*ref_cell.ref);
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreGlobal) {
        Value val = this->get_unary_op();
        std::string& name = fn->names_.at(instr.operand0.value());
        this->globals[name] = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreLocal) {
        Value val = this->get_unary_op();
        this->opstack.at(this->base_index + instr.operand0.value()) = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreReference) {
        Value val = this->get_unary_op();
        if (this->opstack.empty()) {
            throw std::string{"ERROR: trying to store to reference with stack of size 1"};
        }
        ValueRef ref_cell = std::get<ValueRef>(this->get_unary_op());
        *ref_cell.ref = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::PushReference) {
        int32_t i = instr.operand0.value();
        this->opstack.push_back(this->opstack.at(this->base_index + this->num_locals + i));
        this->iptr += 1;
    } else if (instr.operation == Operation::Neg) {
        auto val = this->get_unary_op();
        try {
            this->opstack.emplace_back(-std::get<int>(val));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in negation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Add) {
        auto ops = this->get_binary_ops();
        this->opstack.push_back(std::visit(
            overloaded{
                [](int x, int y) -> Value { return x + y; },
                [](const std::string& l, const std::string& r) -> Value { return l + r; },
                [](const std::string& l, auto r) -> Value { return l + value_to_string(r); },
                [](auto l, const std::string& r) -> Value { return value_to_string(l) + r; },
                [](auto l, auto r) -> Value {
                    throw std::string{"ERROR: invalid cast in '+' operator"};
                }},
            ops.first, ops.second));
        this->iptr += 1;
    } else if (instr.operation == Operation::Sub) {
        auto [x, y] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<int>(x) - std::get<int>(y));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in '-' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Mul) {
        auto [x, y] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<int>(x) * std::get<int>(y));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in '*' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Div) {
        auto [x, y] = this->get_binary_ops();
        int divisor = std::get<int>(y);
        if (divisor == 0) {
            throw std::string{"ERROR: division by zero"};
        }
        try {
            this->opstack.emplace_back(std::get<int>(x) / divisor);
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in '/' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Gt) {
        auto [l, r] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<int>(l) > std::get<int>(r));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in '>' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Geq) {
        auto [l, r] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<int>(l) >= std::get<int>(r));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to int in '>=' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Eq) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(value_eq(l, r));
        this->iptr += 1;
    } else if (instr.operation == Operation::And) {
        auto [l, r] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<bool>(l) && std::get<bool>(r));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to bool in 'and' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Or) {
        auto [l, r] = this->get_binary_ops();
        try {
            this->opstack.emplace_back(std::get<bool>(l) || std::get<bool>(r));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to bool in 'and' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::Not) {
        auto val = this->get_unary_op();
        try {
            this->opstack.emplace_back(!std::get<bool>(val));
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to bool in 'not' operation"};
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::AllocClosure) {
        int32_t m = instr.operand0.value();
        if (this->opstack.size() <= m) {
            throw std::string{"ERROR: not enough stack arguments while allocating closure"};
        }
        std::vector<ValueRef> refs;
        for (size_t i = 0; i < m; ++i) {
            try {
                refs.push_back(std::get<ValueRef>(this->opstack.back()));
            } catch (const std::bad_variant_access& e) {
                throw std::string{"ERROR: expected reference as argument to AllocClosure"};
            }
            this->opstack.pop_back();
        }
        std::reverse(refs.begin(), refs.end());
        struct Function* fn;
        try {
            fn = std::get<struct Function*>(this->opstack.back());
            this->opstack.pop_back();
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: expected function pointer as argument to AllocClosure"};
        }

        auto* closure = new Closure;
        *closure = Closure{.type = FnType::DEFAULT, .fn = fn, .refs = std::move(refs)};
        this->opstack.emplace_back(ClosureRef{.closure = closure});
        this->iptr += 1;
    } else if (instr.operation == Operation::AllocRecord) {
        auto* map = new std::map<std::string, Value>();
        this->opstack.emplace_back(RecordRef{.internal = map});
        this->iptr += 1;
    } else if (instr.operation == Operation::Call) {
        int32_t n_params = instr.operand0.value();
        if (this->opstack.size() <= n_params) {
            throw std::string{"ERROR: not enough stack arguments while calling closure"};
        }
        std::vector<Value> args;
        for (size_t i = 0; i < n_params; ++i) {
            args.push_back(this->get_unary_op());
        }
        std::reverse(args.begin(), args.end());
        Value val = this->get_unary_op();
        ClosureRef ref;
        try {
            ref = std::get<ClosureRef>(val);
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to closure in function call"};
        }
        Closure& c = *ref.closure;
        this->iptr += 1;
        if (c.type == FnType::DEFAULT) {
            // note: this invalidates frame
            if (c.fn->parameter_count_ != n_params) {
                throw std::string{"ERROR: invalid parameter count for function call"};
            }
            
            // push rbp to stack
            this->opstack.emplace_back(this->ctx);
            this->opstack.emplace_back(this->num_locals);
            this->opstack.emplace_back(this->iptr);
            this->opstack.emplace_back(this->base_index);

            this->base_index = this->opstack.size();
            this->ctx = c.fn;
            this->iptr = 0;
            this->num_locals = c.fn->local_vars_.size();

            // push arguments to stack as local variables
            this->opstack.insert(this->opstack.end(), args.begin(), args.end());
            for (size_t i = 0; i < c.fn->local_vars_.size() - n_params; ++i) {
                this->opstack.emplace_back(None{});
            }
            for (size_t i = 0; i < c.fn->local_reference_vars_.size(); ++i) {
                auto* ptr = new Value;
                std::string name = c.fn->local_reference_vars_.at(i);
                int j = -1;
                for (int k = 0; k < c.fn->local_vars_.size(); ++k) {
                    if (c.fn->local_vars_.at(k) == name) {
                        j = k;
                        break;
                    }
                }
                // a local ref var is also a local var
                if (j != -1) {
                    *ptr = this->opstack.at(this->base_index + j);
                } else {
                    *ptr = None{};
                }
                this->opstack.emplace_back(ValueRef{.ref = ptr});
            }
            for (auto ref : c.refs) {
                this->opstack.emplace_back(ref);
            }
        } else if (c.type == FnType::PRINT) {
            if (args.size() != 1) {
                throw std::string{"ERROR: print requires exactly one argument"};
            }
            std::cout << value_to_string(args.at(0)) << '\n';
        } else if (c.type == FnType::INPUT) {
            if (!args.empty()) {
                throw std::string{"ERROR: input requires exactly zero arguments"};
            }
            std::string input;
            std::getline(std::cin, input);
            this->opstack.emplace_back(input);
        } else if (c.type == FnType::INTCAST) {
            if (args.size() != 1) {
                throw std::string{"ERROR: intcast requires exactly one argument"};
            }
            this->opstack.emplace_back(std::stoi(std::get<std::string>(args.at(0))));
        } else {
            throw std::string{"ICE: undefined function type"};
        }
    } else if (instr.operation == Operation::FieldLoad) {
        auto val = this->get_unary_op();
        std::string& field_name = fn->names_.at(instr.operand0.value());
        try {
            RecordRef r = std::get<RecordRef>(val);
            if (r.internal->find(field_name) == r.internal->end()) {
                this->opstack.emplace_back(None{});
            } else {
                this->opstack.push_back(r.internal->at(field_name));
            }
            this->iptr += 1;
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to record in field load"};
        }
    } else if (instr.operation == Operation::FieldStore) {
        auto [record_val, val] = this->get_binary_ops();
        std::string& field_name = fn->names_.at(instr.operand0.value());
        RecordRef r = std::get<RecordRef>(record_val);
        r.internal->insert_or_assign(field_name, val);
        this->iptr += 1;
    } else if (instr.operation == Operation::IndexLoad) {
        auto [record_val, index] = this->get_binary_ops();
        std::string index_string = value_to_string(index);
        try {
            RecordRef r = std::get<RecordRef>(record_val);
            if (r.internal->find(index_string) == r.internal->end()) {
                this->opstack.emplace_back(None{});
            } else {
                this->opstack.push_back(r.internal->at(index_string));
            }
            this->iptr += 1;
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to record in index load"};
        }
    } else if (instr.operation == Operation::IndexStore) {
        auto value = this->get_unary_op();
        auto [record_val, index] = this->get_binary_ops();
        RecordRef r = std::get<RecordRef>(record_val);
        r.internal->insert_or_assign(value_to_string(index), value);
        this->iptr += 1;
    } else if (instr.operation == Operation::Return) {
        // returning from final frame
        if (this->base_index == 0) {
            this->ctx = nullptr;
            this->opstack.resize(0);
        } else {
            if (this->opstack.empty()) {
                throw std::string{"ERROR: called return on empty stack"};
            }
            Value retval = this->opstack.back();
            size_t old_base = std::get<size_t>(this->opstack.at(this->base_index - 1));
            size_t old_iptr = std::get<size_t>(this->opstack.at(this->base_index - 2));
            size_t old_num_locals = std::get<size_t>(this->opstack.at(this->base_index - 3));
            struct Function* old_ctx = std::get<struct Function*>(this->opstack.at(this->base_index - 4));
            this->opstack.resize(this->base_index - 4);
            this->opstack.push_back(retval);
            this->base_index = old_base;
            this->iptr = old_iptr;
            this->num_locals = old_num_locals;
            this->ctx = old_ctx;
        }
    } else if (instr.operation == Operation::Pop) {
        if (this->opstack.empty()) {
            throw std::string{"ERROR: called pop on empty opstack"};
        }
        this->opstack.pop_back();
        this->iptr += 1;
    } else if (instr.operation == Operation::Goto) {
        this->iptr += instr.operand0.value();
    } else if (instr.operation == Operation::If) {
        auto val = this->get_unary_op();
        try {
            if (std::get<bool>(val)) {
                this->iptr += instr.operand0.value();
            } else {
                this->iptr += 1;
            }
        } catch (const std::bad_variant_access& e) {
            throw std::string{"ERROR: invalid cast to bool in 'if' condition"};
        }
    } else if (instr.operation == Operation::Dup) {
        if (this->opstack.empty()) {
            throw std::string{"ERROR: called dup on empty stack"};
        }
        Value v = this->opstack.back();
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::Swap) {
        size_t stack_size = this->opstack.size();
        if (stack_size < 2) {
            throw std::string{"ERROR: swap executed on stack with less than 2 elements"};
        }
        Value tmp = this->opstack.back();
        this->opstack.back() = this->opstack[stack_size - 2];
        this->opstack[stack_size - 2] = tmp;
        this->iptr += 1;
    } else {
        throw std::string{"ICE: invalid opcode"};
    }
    return true;
}
