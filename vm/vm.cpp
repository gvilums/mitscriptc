#include "vm.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <cstring>

#include "instructions.h"
#include "types.h"
#include "allocator.h"


namespace VM {

auto Value::from_constant(Constant c, VirtualMachine& vm) -> Value {
    if (std::holds_alternative<None>(c)) {
        return {};
    } else if (std::holds_alternative<bool>(c)) {
        return std::get<bool>(c);
    } else if (std::holds_alternative<int>(c)) {
        return std::get<int>(c);
    } else if (std::holds_alternative<std::string>(c)) {
        return vm.alloc_string(std::get<std::string>(c));
    } else {
        std::terminate();
    }
}
    
void Value::trace() const {
    if (this->tag == VALUE_PTR) {
        ((HeapObj*)((char*)this->value - sizeof(HeapObj)))->marked = true;
        this->value->trace();
    } else if (this->tag == RECORD_PTR) {
        ((HeapObj*)((char*)this->record - sizeof(HeapObj)))->marked = true;
        this->record->trace();
    } else if (this->tag == CLOSURE_PTR) {
        ((HeapObj*)((char*)this->closure - sizeof(HeapObj)))->marked = true;
        this->closure->trace();
    } else if (this->tag == STRING_PTR) {
        ((HeapObj*)((char*)this->string - sizeof(HeapObj)))->marked = true;
    }
}

void Record::trace() const {
    for (const auto& [key, val] : this->fields) {
        key.trace();
        val.trace();
    }
}

void Closure::trace() const {
    for (auto* val_ptr : this->refs) {
        ((HeapObj*)((char*)val_ptr - sizeof(HeapObj)))->marked = true;
        val_ptr->trace();
    }
}

std::ostream& operator<<(std::ostream& os, const String& str) {
    for (size_t i = 0; i < str.size; ++i) {
        os << str.data[i];
    }
    return os;
}

auto operator==(const Value& lhs, const Value& rhs) -> bool {
    if (lhs.tag != rhs.tag) {
        return false;
    }
    Value::ValueTag tag = lhs.tag;
    if (tag == Value::NONE) {
        return true;
    }
    if (tag == Value::NUM) {
        return lhs.num == rhs.num;
    }
    if (tag == Value::BOOL) {
        return lhs.boolean == rhs.boolean;
    }
    if (tag == Value::STRING_PTR) {
        size_t len = lhs.string->size;
        if (rhs.string->size != len) {
            return false;
        }
        for (size_t i = 0; i < len; ++i) {
            if (lhs.string->data[i] != rhs.string->data[i]) {
                return false;
            }
        }
        return true;
    }
    if (tag == Value::RECORD_PTR) {
        return lhs.record == rhs.record;
    }
    // TODO think about throwing error on comparison of non-program-variables
    return false;
}

auto operator>=(Value const& lhs, Value const& rhs) -> bool {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return lhs.num >= rhs.num;
    }
    throw std::string{"IllegalCastException"};
}

auto operator>(Value const& lhs, Value const& rhs) -> bool {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return lhs.num > rhs.num;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::to_string(VirtualMachine& vm) const -> String* {
    if (this->tag == NONE) {
        return vm.none_string;
    }
    if (this->tag == BOOL) {
        return this->boolean ? vm.true_string : vm.false_string;
    }
    if (this->tag == NUM) {
        return vm.alloc_string(std::to_string(this->num));
    }
    if (this->tag == STRING_PTR) {
        return this->string;
    }
    if (this->tag == CLOSURE_PTR) {
        return vm.function_string;
    }
    if (this->tag == RECORD_PTR) {
        // TODO make more efficient
        std::string out{"{"};
        std::vector<std::pair<Value, Value>> vals{this->record->fields.begin(), this->record->fields.end()};
        std::sort(vals.begin(), vals.end(),
                  [](const std::pair<Value, Value>& l, const std::pair<Value, Value>& r) { return !(l.first >= r.first); });
        for (const auto& [key, val] : vals) {
            out.append(key.to_string(vm)->to_std_string());
            out.push_back(':');
            out.append(val.to_string(vm)->to_std_string());
            out.push_back(' ');
        }
        out.push_back('}');
        return vm.alloc_string(out);
    }
    throw std::string{"RuntimeException"};
}

auto VirtualMachine::alloc_record() -> Record* {
    size_t size = sizeof(HeapObj) + sizeof(Record);
    HeapObj* ptr = (HeapObj*) ::operator new(size);
    ::new (ptr) HeapObj(this->heap_head);
    ::new (&ptr->data) Record;
    return (Record*) &ptr->data;
}

template<typename ...Args>
auto VirtualMachine::alloc_closure(Args&&... args) -> Closure* {
    HeapObj* ptr = (HeapObj*) ::operator new(sizeof(HeapObj) + sizeof(Closure));
    ::new (ptr) HeapObj(this->heap_head);
    ::new (&ptr->data) Closure{std::forward<Args>(args)...};
    return (Closure*) &ptr->data;
}

auto VirtualMachine::alloc_string(size_t len) -> String* {
    HeapObj* ptr = (HeapObj*) ::operator new(sizeof(HeapObj) + sizeof(String) + len);
    ::new (ptr) HeapObj(this->heap_head);
    ::new (&ptr->data) String{len};
    return (String*) &ptr->data;
}

auto VirtualMachine::alloc_string(const std::string& str) -> String* {
    size_t len = str.size();
    String* str_ptr = this->alloc_string(len);
    std::memcpy(&str_ptr->data, str.c_str(), len);
    return str_ptr;
}

auto VirtualMachine::alloc_value() -> Value* {
    HeapObj* ptr = (HeapObj*) ::operator new(sizeof(HeapObj) + sizeof(Value));
    ::new (ptr) HeapObj(this->heap_head);
    ::new (&ptr->data) Value;
    return (Value*) &ptr->data;
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
        HeapObj* next = this->heap_head->next;
        // std::cout << "deleting head at " << this->heap_head << std::endl;
        ::operator delete(this->heap_head);
        // delete this->heap_head;
        // this->heap_size -= (sizeof(HeapObject) );//+ Allocation::ALLOC_OVERHEAD);
        this->heap_head = next;
    }
    

    // if we just cleared the entire heap, return
    if (this->heap_head == nullptr) {
        return;
    }

    // std::cout << "live object" << std::endl;

    // unmark head as we'll skip it in the following
    this->heap_head->marked = false;
    HeapObj* prev = this->heap_head;
    HeapObj* current = this->heap_head->next;

    while (current != nullptr) {
        if (!current->marked) {
            HeapObj* next = current->next;
            // std::cout << "deletion in list at " << current << std::endl;
            ::operator delete(current);
            // this->heap_size -= sizeof(HeapObject);
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

// check if gc is necessary, and, if so, execute it
void VirtualMachine::gc_check() {
    size_t total_mem = Allocation::total_alloc + this->heap_size;
    if (total_mem * 3 > this->max_heap_size * 2) {
        this->gc_collect();
        size_t live_mem = Allocation::total_alloc + this->heap_size;
        if (live_mem > this->max_heap_size) {
            std::cerr << "WARNING: live program memory larger than limit, continuing without limit" << std::endl;
            this->max_heap_size = std::numeric_limits<size_t>::max();
        }
    }
}

// get one operand from stack
auto VirtualMachine::get_unary_op() -> Value {
    if (this->opstack.size() == this->base_index) {
        throw std::string{"InsufficientStackException"};
    }
    Value operand = this->opstack.back();
    this->opstack.pop_back();
    return operand;
}

// get two operands from stack
auto VirtualMachine::get_binary_ops() -> std::pair<Value, Value> {
    size_t stack_size = this->opstack.size();
    if (stack_size - this->base_index < 2) {
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
        this->print_closure = this->alloc_closure(Closure::PRINT, nullptr);
        this->input_closure = this->alloc_closure(Closure::INPUT, nullptr);
        this->intcast_closure = this->alloc_closure(Closure::INTCAST, nullptr);
        this->none_string = this->alloc_string("None");
        this->true_string = this->alloc_string("true");
        this->false_string = this->alloc_string("false");
        this->function_string = this->alloc_string("FUNCTION");
    }
    
VirtualMachine::VirtualMachine(struct Function* prog, size_t heap_limit) 
    : VirtualMachine{prog} {
        this->max_heap_size = heap_limit;
    }

VirtualMachine::~VirtualMachine() {
    delete this->print_closure;
    delete this->input_closure;
    delete this->intcast_closure;
}

void VirtualMachine::reset() {
    this->ctx = this->source;
    this->base_index = 0;
    this->iptr = 0;
    this->num_locals = 0;
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
    // returned from global context
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
        this->opstack.push_back(Value::from_constant(this->ctx->constants_.at(instr.operand0.value()), *this));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadGlobal) {
        String* name = this->alloc_string(this->ctx->names_.at(instr.operand0.value()));
        if (this->globals.find(name) == this->globals.end()) {
            throw std::string{"UninitializedVariableException"};
        }
        this->opstack.push_back(this->globals.at(name));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadLocal) {
        int offset = instr.operand0.value();
        if (offset < 0 || offset >= this->ctx->local_vars_.size()) {
            throw std::string{"RuntimeError"};
        }
        this->opstack.push_back(this->opstack.at(this->base_index + instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadFunc) {
        this->opstack.emplace_back(this->ctx->functions_.at(instr.operand0.value()));
        this->iptr += 1;
    } else if (instr.operation == Operation::LoadReference) {
        if (this->opstack.size() == this->base_index) {
            throw std::string{"InsufficientStackException"};
        }
        Value& val = this->opstack.back().get_val_ref();
        this->opstack.pop_back();
        this->opstack.push_back(val);
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreGlobal) {
        Value val = this->get_unary_op();
        Value name{this->alloc_string(this->ctx->names_.at(instr.operand0.value()))};
        this->globals[name] = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreLocal) {
        Value val = this->get_unary_op();
        this->opstack.at(this->base_index + instr.operand0.value()) = val;
        this->iptr += 1;
    } else if (instr.operation == Operation::StoreReference) {
        Value val = this->get_unary_op();
        if (this->opstack.size() == this->base_index) {
            throw std::string{"InsufficientStackException"};
        }
        this->get_unary_op().get_val_ref() = val;
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
        this->opstack.emplace_back(l.add(r, *this));
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
        if (this->opstack.size() - this->base_index <= m) {
            throw std::string{"InsufficientStackException"};
        }
        Closure::TrackedVec refs;
        refs.reserve(m);
        for (size_t i = this->opstack.size() - m; i < this->opstack.size(); ++i) {
            // TODO check addr operator
            refs.push_back(&this->opstack.at(i).get_val_ref());
        }
        this->opstack.resize(this->opstack.size() - m);

        struct Function* fn = this->opstack.back().get_fnptr();
        this->opstack.pop_back();

        Closure* closure;
        // check if assigning builtins
        if (fn == this->source->functions_[0]) {
            closure = this->print_closure;
        } else if (fn == this->source->functions_[1]) {
            closure = this->input_closure;
        } else if (fn == this->source->functions_[2]) {
            closure = this->intcast_closure;
        } else {
            closure = this->alloc_closure(Closure::DEFAULT, fn, std::move(refs));
        }
        this->opstack.emplace_back(closure);
        this->iptr += 1;
    } else if (instr.operation == Operation::AllocRecord) {
        auto* rec = this->alloc_record();
        this->opstack.emplace_back(rec);
        this->iptr += 1;
    } else if (instr.operation == Operation::Call) {
        int32_t n_params = instr.operand0.value();
        if (this->opstack.size() - this->base_index <= n_params) {
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
        if (c.type == Closure::DEFAULT) {
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
                Value* ptr = this->alloc_value();
                if (j != -1) {
                    *ptr = this->opstack.at(this->base_index + j);
                }
                this->opstack.emplace_back(ptr);
            }
            for (auto* ref : c.refs) {
                this->opstack.emplace_back(ref);
            }
        } else if (c.type == Closure::PRINT) {
            if (arg_stage.size() != 1) {
                throw std::string{"RuntimeException"};
            }
            std::cout << arg_stage.at(0).to_string(*this) << '\n';
        } else if (c.type == Closure::INPUT) {
            if (!arg_stage.empty()) {
                throw std::string{"RuntimeException"};
            }
            std::string input;
            std::getline(std::cin, input);
            this->opstack.emplace_back(this->alloc_string(input));
        } else if (c.type == Closure::INTCAST) {
            if (arg_stage.size() != 1) {
                throw std::string{"RuntimeException"};
            }
            std::string str{arg_stage.at(0).to_string(*this)->to_std_string()};
            this->opstack.emplace_back(std::stoi(str));
        } else {
            throw std::string{"Internal Compiler Error: undefined function type"};
        }
    } else if (instr.operation == Operation::FieldLoad) {
        Record& r = this->get_unary_op().get_record();
        Value field_name{this->alloc_string(this->ctx->names_.at(instr.operand0.value()))};
        if (r.fields.find(field_name) == r.fields.end()) {
            this->opstack.emplace_back(None{});
        } else {
            this->opstack.push_back(r.fields.at(field_name));
        }
        this->iptr += 1;
    } else if (instr.operation == Operation::FieldStore) {
        auto [record_val, val] = this->get_binary_ops();
        Value field_name{this->alloc_string(this->ctx->names_.at(instr.operand0.value()))};
        Record& r = record_val.get_record();
        r.fields.insert_or_assign(field_name, val);
        this->iptr += 1;
    } else if (instr.operation == Operation::IndexLoad) {
        auto [record_val, index] = this->get_binary_ops();
        Value index_string{index.to_string(*this)};
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
        r.fields.insert_or_assign(index.to_string(*this), value);
        this->iptr += 1;
    } else if (instr.operation == Operation::Return) {
        if (this->opstack.size() == this->base_index) {
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
        if (this->opstack.size() == this->base_index) {
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
        if (this->opstack.size() == this->base_index) {
            throw std::string{"InsufficientStackException"};
        }
        Value v = this->opstack.back();
        this->opstack.push_back(v);
        this->iptr += 1;
    } else if (instr.operation == Operation::Swap) {
        size_t stack_size = this->opstack.size();
        if (stack_size - this->base_index < 2) {
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

auto Value::get_tag() -> ValueTag {
    return this->tag;
}

auto Value::get_bool() -> bool {
    if (this->tag == BOOL) {
        return this->boolean;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_int() -> int {
    if (this->tag == NUM) {
        return this->num;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_string() -> String& {
    if (this->tag == STRING_PTR) {
        return *this->string;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_record() -> Record& {
    if (this->tag == RECORD_PTR) {
        return *this->record;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_closure() -> Closure& {
    if (this->tag == CLOSURE_PTR) {
        return *this->closure;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_val_ref() -> Value& {
    if (this->tag == VALUE_PTR) {
        return *this->value;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_fnptr() -> struct Function* {
    if (this->tag == FN_PTR) {
        return this->fnptr;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_usize() -> size_t {
    if (this->tag == USIZE) {
        return this->usize;
    }
    throw std::string{"IllegalCastException"};
}
};  // namespace VM