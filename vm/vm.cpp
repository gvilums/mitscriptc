#include "vm.h"

#include <algorithm>
#include <iostream>
#include <limits>

#include "instructions.h"
#include "types.h"
#include "value.h"
#include "allocator.h"


namespace VM {

template<typename T>
auto VirtualMachine::alloc(T t) -> HeapObject* {
    // std::cout << "previous head: " << this->heap_head << std::endl;
    auto* obj = new HeapObject(std::move(t));
    this->heap_size += (sizeof(HeapObject));// + Allocation::ALLOC_OVERHEAD);
    // std::cout << "at address " << obj << std::endl;
    obj->next = this->heap_head;
    this->heap_head = obj;
    // std::cout << "new head: " << this->heap_head << std::endl;
    return obj;
}

void VirtualMachine::gc_collect() {
    // std::cout << "starting gc" << std::endl;
    // no current allocations
    if (this->heap_head == nullptr) {
        return;
    }

    // mark
    for (auto& val : this->opstack) {
        val.trace();
    }
    for (auto& [_, val] : this->globals) {
        val.trace();
    }
    // maybe unnecessary
    for (auto& val : this->arg_stage) {
        val.trace();
    }

    // free unmarked heads
    while (this->heap_head != nullptr && !this->heap_head->marked) {
        HeapObject* next = this->heap_head->next;
        // std::cout << "deleting head at " << this->heap_head << std::endl;
        delete this->heap_head;
        this->heap_size -= (sizeof(HeapObject) );//+ Allocation::ALLOC_OVERHEAD);
        this->heap_head = next;
    }
    

    // if we just cleared the entire heap, return
    if (this->heap_head == nullptr) {
        return;
    }

    // std::cout << "live object" << std::endl;

    // unmark head as we'll skip it in the following
    this->heap_head->marked = false;
    HeapObject* prev = this->heap_head;
    HeapObject* current = this->heap_head->next;

    while (current != nullptr) {
        if (!current->marked) {
            HeapObject* next = current->next;
            // std::cout << "deletion in list at " << current << std::endl;
            delete current;
            this->heap_size -= sizeof(HeapObject);
            current = next;
            prev->next = current;
        } else {
            // std::cout << "live object" << std::endl;
            current->marked = false;
            prev = current;
            current = current->next;
        }
    }
    // sweep
}

void VirtualMachine::gc_check() {
    size_t total_mem = Allocation::total_alloc + this->heap_size;
    if (total_mem > this->max_heap_size * 2 / 3) {
        this->gc_collect();
        size_t live_mem = Allocation::total_alloc + this->heap_size;
        if (live_mem > this->max_heap_size) {
            std::cerr << "WARNING: live program memory larger than limit, continuing without limit" << std::endl;
            this->max_heap_size = std::numeric_limits<size_t>::max();
        }
    }
}

auto VirtualMachine::get_unary_op() -> Value {
    if (this->opstack.empty()) {
        throw std::string{"InsufficientStackException"};
    }
    Value operand = this->opstack.back();
    this->opstack.pop_back();
    return operand;
}

auto VirtualMachine::get_binary_ops() -> std::pair<Value, Value> {
    size_t stack_size = this->opstack.size();
    if (stack_size < 2) {
        throw std::string{"InsufficientStackException"};
    }
    Value lop = this->opstack[stack_size - 2];
    Value rop = this->opstack[stack_size - 1];

    this->opstack.pop_back();
    this->opstack.pop_back();

    return std::pair{lop, rop};
}

VirtualMachine::VirtualMachine(struct Function* prog)
    : source(prog) {
        this->print_closure = this->alloc(Closure{FnType::PRINT, nullptr});
        this->input_closure = this->alloc(Closure{FnType::INPUT, nullptr});
        this->intcast_closure = this->alloc(Closure{FnType::INTCAST, nullptr});
    }
    
VirtualMachine::VirtualMachine(struct Function* prog, size_t heap_limit) 
    : VirtualMachine{prog} {
        this->max_heap_size = heap_limit;
    }

void VirtualMachine::reset() {
    this->ctx = this->source;
    this->base_index = 0;
    this->iptr = 0;
    this->num_locals = 0;
    
    // auto* print_closure = this->alloc(Closure{FnType::PRINT, nullptr});
    // auto* input_closure = this->alloc(Closure{FnType::INPUT, nullptr});
    // auto* intcast_closure = this->alloc(Closure{FnType::INTCAST, nullptr});

    // this->globals = {
    //     {"print", {print_closure}},
    //     {"input", {input_closure}},
    //     {"intcast", {intcast_closure}}};
}

void VirtualMachine::exec() {
    this->reset();
    while (this->step()) {
        this->gc_check();
    }
    this->opstack.clear();
    this->globals.clear();
    this->arg_stage.clear();
    this->gc_collect();
}

auto VirtualMachine::step() -> bool {
    // return from global context
    if (this->ctx == nullptr) {
        return false;
    }
    // iptr ran past instruction list
    if (this->iptr >= this->ctx->instructions.size()) {
        // ok if in global frame, just end program
        if (this->base_index == 0) {
            return false;
        }
        // error in non-global frame
        throw std::string{"RuntimeException"};
    }

    Instruction instr = this->ctx->instructions[this->iptr];

    if (instr.operation == Operation::LoadConst) {
        Value v{this->ctx->constants_.at(instr.operand0.value())};
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadGlobal) {
        TrackedString name{this->ctx->names_.at(instr.operand0.value())};
        if (this->globals.find(name) == this->globals.end()) {
            throw std::string{"UninitializedVariableException"};
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
            throw std::string{"InsufficientStackException"};
        }
        Value& val = this->opstack.back().get_val_ref();
        this->opstack.pop_back();
        this->opstack.push_back(val);
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreGlobal) {
        Value val = this->get_unary_op();
        // check if assigning builtins
        if (val.get_tag() == Value::HEAP_REF) {
            auto* ref_ptr = val.get_heap_ref();
            if (ref_ptr->tag == HeapObject::CLOSURE) {
                auto& closure = ref_ptr->get_closure();
                if (closure.fn == this->source->functions_[0]) {
                    val = Value{this->print_closure};
                } else if (closure.fn == this->source->functions_[1]) {
                    val = Value{this->input_closure};
                } else if (closure.fn == this->source->functions_[2]) {
                    val = Value{this->intcast_closure};
                }
            }
        }
        TrackedString name{this->ctx->names_.at(instr.operand0.value())};
        this->globals[name] = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreLocal) {
        Value val = this->get_unary_op();
        this->opstack.at(this->base_index + instr.operand0.value()) = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreReference) {
        Value val = this->get_unary_op();
        if (this->opstack.empty()) {
            throw std::string{"InsufficientStackException"};
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
            throw std::string{"IllegalArithmeticException"};
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
            throw std::string{"InsufficientStackException"};
        }
        Closure::TrackedVec refs;
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
            throw std::string{"InsufficientStackException"};
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
                throw std::string{"RuntimeException"};
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
            if (c.fn->local_vars_.size() < n_params) {
                throw std::string{"RuntimeException"};
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
                throw std::string{"RuntimeException"};
            }
            std::cout << arg_stage.at(0).to_string() << '\n';
        } else if (c.type == FnType::INPUT) {
            if (!arg_stage.empty()) {
                throw std::string{"RuntimeException"};
            }
            std::string input;
            std::getline(std::cin, input);
            this->opstack.emplace_back(input);
        } else if (c.type == FnType::INTCAST) {
            if (arg_stage.size() != 1) {
                throw std::string{"RuntimeException"};
            }
            this->opstack.emplace_back(std::stoi(std::string{arg_stage.at(0).to_string()}));
        } else {
            throw std::string{"Internal Compiler Error: undefined function type"};
        }
    } else if (instr.operation == Operation::FieldLoad) {
        Record& r = this->get_unary_op().get_record();
        TrackedString field_name{this->ctx->names_.at(instr.operand0.value())};
        if (r.fields.find(field_name) == r.fields.end()) {
            this->opstack.emplace_back(None{});
        } else {
            this->opstack.push_back(r.fields.at(field_name));
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::FieldStore) {
        auto [record_val, val] = this->get_binary_ops();
        TrackedString field_name{this->ctx->names_.at(instr.operand0.value())};
        Record& r = record_val.get_record();
        r.fields.insert_or_assign(field_name, val);
        this->iptr += 1;
    } else if (instr.operation == Operation::IndexLoad) {
        auto [record_val, index] = this->get_binary_ops();
        TrackedString index_string = index.to_string();
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
        if (this->opstack.empty()) {
            throw std::string{"InsufficientStackException"};
        }
        // returning from final frame
        if (this->base_index == 0) {
            this->ctx = nullptr;
            this->opstack.resize(0);
        } else {
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
            throw std::string{"InsufficientStackException"};
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
            throw std::string{"InsufficientStackException"};
        }
        Value v = this->opstack.back();
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::Swap) {
        size_t stack_size = this->opstack.size();
        if (stack_size < 2) {
            throw std::string{"InsufficientStackException"};
        }
        Value tmp = this->opstack.back();
        this->opstack.back() = this->opstack[stack_size - 2];
        this->opstack[stack_size - 2] = tmp;
        this->iptr += 1;
    } else {
        throw std::string{"Internal Compiler Error: invalid opcode"};
    }
    return true;
}
};  // namespace VM