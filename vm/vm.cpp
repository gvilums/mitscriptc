#include "vm.h"

#include <algorithm>
#include <iostream>

#include "instructions.h"
#include "types.h"
#include "value.h"


namespace VM {

template<typename T>
auto VirtualMachine::alloc(T t) -> HeapObject* {
    // std::cout << "allocation" << std::endl;
    // std::cout << "previous head: " << this->heap_head << std::endl;
    auto* obj = new HeapObject(std::move(t));
    obj->next = this->heap_head;
    this->heap_head = obj;
    // std::cout << "new head: " << this->heap_head << std::endl;
    return obj;
}

void VirtualMachine::gc_collect() {
    // no current allocations
    if (this->heap_head == nullptr) {
        return;
    }

    // mark
    for (auto& val : this->opstack) {
        val.trace();
    }

    // free unmarked heads
    while (this->heap_head != nullptr && !this->heap_head->marked) {
        HeapObject* next = this->heap_head->next;
        // std::cout << "deleting head at " << this->heap_head << std::endl;
        delete this->heap_head;
        this->heap_head = next;
    }

    // if we just cleared the entire heap, return
    if (this->heap_head == nullptr) {
        return;
    }

    // unmark head as we'll skip it in the following
    this->heap_head->marked = false;
    HeapObject* prev = this->heap_head;
    HeapObject* current = this->heap_head->next;

    while (current != nullptr) {
        if (!current->marked) {
            HeapObject* next = current->next;
            // std::cout << "deletion in list" << std::endl;
            delete current;
            current = next;
            prev->next = current;
        } else {
            current->marked = false;
            prev = current;
            current = current->next;
        }
    }
    // sweep
}

auto VirtualMachine::get_unary_op() -> Value {
    if (this->opstack.empty()) {
        throw std::string{"ERROR: empty stack for unary operation"};
    }
    Value operand = this->opstack.back();
    this->opstack.pop_back();
    return operand;
}

auto VirtualMachine::get_binary_ops() -> std::pair<Value, Value> {
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

VirtualMachine::VirtualMachine(struct Function* prog)
    : source(prog) {}

void VirtualMachine::reset() {
    this->ctx = this->source;
    this->base_index = 0;
    this->iptr = 0;
    this->num_locals = 0;
    
    auto* print_closure = this->alloc(Closure{FnType::PRINT, nullptr});
    auto* input_closure = this->alloc(Closure{FnType::INPUT, nullptr});
    auto* intcast_closure = this->alloc(Closure{FnType::INTCAST, nullptr});

    this->globals = {
        {"print", {print_closure}},
        {"input", {input_closure}},
        {"intcast", {intcast_closure}}};
}

void VirtualMachine::exec() {
    this->reset();
    while ((this->ctx != nullptr) && this->iptr < this->ctx->instructions.size()) {
        this->step();
        // temporary: gc collect after every bytecode instr
    }
    this->gc_collect();
}

auto VirtualMachine::step() -> bool {
    if (this->ctx == nullptr) {
        return false;
    }

    if (this->iptr >= this->ctx->instructions.size()) {
        throw std::string{"instruction pointer overran instruction list"};
    }
    Instruction instr = this->ctx->instructions[this->iptr];

    if (instr.operation == Operation::LoadConst) {
        Value v = value_from_constant(this->ctx->constants_.at(instr.operand0.value()));
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadGlobal) {
        std::string name = this->ctx->names_.at(instr.operand0.value());
        if (this->globals.find(name) == this->globals.end()) {
            throw std::string{"ERROR: uninitialized variable"};
        }
        this->opstack.push_back(this->globals.at(name));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadLocal) {
        this->opstack.push_back(this->opstack.at(this->base_index + instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadFunc) {
        this->opstack.emplace_back(this->ctx->functions_.at(instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadReference) {
        if (this->opstack.empty()) {
            throw std::string{"ERROR: trying to load reference from empty stack"};
        }
        Value& val = this->opstack.back().get_val_ref();
        this->opstack.pop_back();
        this->opstack.push_back(val);
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreGlobal) {
        Value val = this->get_unary_op();
        std::string& name = this->ctx->names_.at(instr.operand0.value());
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
        this->get_unary_op().get_val_ref() = std::move(val);
        this->iptr += 1;
    } else if (instr.operation == Operation::PushReference) {
        int32_t i = instr.operand0.value();
        this->opstack.push_back(this->opstack.at(this->base_index + this->num_locals + i));
        this->iptr += 1;
    } else if (instr.operation == Operation::Neg) {
        auto val = this->get_unary_op();
        this->opstack.emplace_back(-val.get_int());
        this->iptr += 1;
    } else if (instr.operation == Operation::Add) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l + r);
        this->iptr += 1;
    } else if (instr.operation == Operation::Sub) {
        auto [x, y] = this->get_binary_ops();
        this->opstack.emplace_back(x.get_int() - y.get_int());
        this->iptr += 1;
    } else if (instr.operation == Operation::Mul) {
        auto [x, y] = this->get_binary_ops();
        this->opstack.emplace_back(x.get_int() * y.get_int());
        this->iptr += 1;
    } else if (instr.operation == Operation::Div) {
        auto [x, y] = this->get_binary_ops();
        int divisor = y.get_int();
        if (divisor == 0) {
            throw std::string{"ERROR: division by zero"};
        }
        this->opstack.emplace_back(x.get_int() / divisor);
        this->iptr += 1;
    } else if (instr.operation == Operation::Gt) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l > r);
        this->iptr += 1;
    } else if (instr.operation == Operation::Geq) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l >= r);
        this->iptr += 1;
    } else if (instr.operation == Operation::Eq) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l == r);
        this->iptr += 1;
    } else if (instr.operation == Operation::And) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l.get_bool() && r.get_bool());
        this->iptr += 1;
    } else if (instr.operation == Operation::Or) {
        auto [l, r] = this->get_binary_ops();
        this->opstack.emplace_back(l.get_bool() || r.get_bool());
        this->iptr += 1;
    } else if (instr.operation == Operation::Not) {
        auto val = this->get_unary_op();
        this->opstack.emplace_back(!val.get_bool());
        this->iptr += 1;
    } else if (instr.operation == Operation::AllocClosure) {
        int32_t m = instr.operand0.value();
        if (this->opstack.size() <= m) {
            throw std::string{"ERROR: not enough stack arguments while allocating closure"};
        }
        std::vector<HeapObject*> refs;
        refs.reserve(m);
        for (size_t i = this->opstack.size() - m; i < this->opstack.size(); ++i) {
            refs.push_back(this->opstack.at(i).get_heap_ref());
        }
        this->opstack.resize(this->opstack.size() - m);

        struct Function* fn = this->opstack.back().get_fnptr();
        this->opstack.pop_back();

        auto* closure = this->alloc(Closure{FnType::DEFAULT, fn, std::move(refs)});
        this->opstack.emplace_back(closure);
        this->iptr += 1;
    } else if (instr.operation == Operation::AllocRecord) {
        auto* rec = this->alloc(Record{});
        this->opstack.emplace_back(rec);
        this->iptr += 1;
    } else if (instr.operation == Operation::Call) {
        int32_t n_params = instr.operand0.value();
        if (this->opstack.size() <= n_params) {
            throw std::string{"ERROR: not enough stack arguments while calling closure"};
        }
        this->arg_stage.clear();
        this->arg_stage.reserve(n_params);
        this->arg_stage.insert(
            this->arg_stage.begin(),
            std::next(this->opstack.begin(), (long)this->opstack.size() - n_params),
            this->opstack.end());
        this->opstack.resize(this->opstack.size() - n_params);

        Closure& c = this->get_unary_op().get_closure();
        this->iptr += 1;
        if (c.type == FnType::DEFAULT) {
            if (c.fn->parameter_count_ != n_params) {
                throw std::string{"ERROR: invalid parameter count for function call"};
            }

            this->opstack.emplace_back(this->ctx);
            this->opstack.emplace_back(this->num_locals);
            this->opstack.emplace_back(this->iptr);
            this->opstack.emplace_back(this->base_index);

            this->base_index = this->opstack.size();
            this->ctx = c.fn;
            this->iptr = 0;
            this->num_locals = c.fn->local_vars_.size();

            // push arguments to stack as local variables
            // this->opstack.insert(this->opstack.end(), arg_stage.begin(), arg_stage.end());
            for (const auto& arg : arg_stage) {
                this->opstack.push_back(arg);
            }
            for (size_t i = 0; i < c.fn->local_vars_.size() - n_params; ++i) {
                this->opstack.emplace_back(None{});
            }
            for (size_t i = 0; i < c.fn->local_reference_vars_.size(); ++i) {
                std::string name = c.fn->local_reference_vars_.at(i);
                int j = -1;
                for (int k = 0; k < c.fn->local_vars_.size(); ++k) {
                    if (c.fn->local_vars_.at(k) == name) {
                        j = k;
                        break;
                    }
                }
                // a local ref var is also a local var
                HeapObject* ptr;
                if (j != -1) {
                    ptr = this->alloc(this->opstack.at(this->base_index + j));
                } else {
                    ptr = this->alloc(None{});
                }
                this->opstack.emplace_back(ptr);
            }
            for (auto* ref : c.refs) {
                this->opstack.emplace_back(ref);
            }
        } else if (c.type == FnType::PRINT) {
            if (arg_stage.size() != 1) {
                throw std::string{"ERROR: print requires exactly one argument"};
            }
            std::cout << arg_stage.at(0).to_string() << '\n';
        } else if (c.type == FnType::INPUT) {
            if (!arg_stage.empty()) {
                throw std::string{"ERROR: input requires exactly zero arguments"};
            }
            std::string input;
            std::getline(std::cin, input);
            this->opstack.emplace_back(input);
        } else if (c.type == FnType::INTCAST) {
            if (arg_stage.size() != 1) {
                throw std::string{"ERROR: intcast requires exactly one argument"};
            }
            this->opstack.emplace_back(std::stoi(arg_stage.at(0).to_string()));
        } else {
            throw std::string{"ICE: undefined function type"};
        }
    } else if (instr.operation == Operation::FieldLoad) {
        Record& r = this->get_unary_op().get_record();
        std::string& field_name = this->ctx->names_.at(instr.operand0.value());
        if (r.fields.find(field_name) == r.fields.end()) {
            this->opstack.emplace_back(None{});
        } else {
            this->opstack.push_back(r.fields.at(field_name));
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::FieldStore) {
        auto [record_val, val] = this->get_binary_ops();
        std::string& field_name = this->ctx->names_.at(instr.operand0.value());
        Record& r = record_val.get_record();
        r.fields.insert_or_assign(field_name, val);
        this->iptr += 1;
    } else if (instr.operation == Operation::IndexLoad) {
        auto [record_val, index] = this->get_binary_ops();
        std::string index_string = index.to_string();
        Record& r = record_val.get_record();
        if (r.fields.find(index_string) == r.fields.end()) {
            this->opstack.emplace_back(None{});
        } else {
            this->opstack.push_back(r.fields.at(index_string));
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::IndexStore) {
        auto value = this->get_unary_op();
        auto [record_val, index] = this->get_binary_ops();
        Record& r = record_val.get_record();
        r.fields.insert_or_assign(index.to_string(), value);
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
            size_t old_base = this->opstack.at(this->base_index - 1).get_usize();
            size_t old_iptr = this->opstack.at(this->base_index - 2).get_usize();
            size_t old_num_locals = this->opstack.at(this->base_index - 3).get_usize();
            struct Function* old_ctx = this->opstack.at(this->base_index - 4).get_fnptr();
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
        if (val.get_bool()) {
            this->iptr += instr.operand0.value();
        } else {
            this->iptr += 1;
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
};  // namespace VM