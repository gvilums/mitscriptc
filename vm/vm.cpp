#include "vm.h"
#include "instructions.h"

Value from_constant(Constant constant) {
    return std::visit([](auto x) -> Value { return x; }, constant);
}

std::string to_string(const Value& val) {
    return std::visit(overloaded {
        [](None x) { return std::string{"None"}; },
        [](bool b) { return b ? "true" : "false"; },
        [](int i) { return std::to_string(i); },
        [](std::string s) { return s; },
        [](Record r) { 
            // TODO
            std::string out{"{"};
            for (auto p : *r.internal) {
                out.append(p.first);
                out.push_back(':');
            
            }
            return std::string{"TODO"};
        },
        [](RefCell c) { return std::string{"refcell"}; },
        [](FnPtr fn) { return std::string{"FUNCTION"}; },
        [](BuiltinFn fn) { return std::string{"FUNCTION"}; },
    }, val);
}

Value val_add(const Value& lhs, const Value& rhs) {
    return std::visit(overloaded {
        [](const int& x, const int& y) -> Value { return x + y; },
        [](const std::string& l, const std::string& r) -> Value { return l + r; },
        []()
    }, lhs, rhs);
}

VM::VM(Function&& prog) : source(std::move(prog)) {

}

void VM::exec() {
    while (!this->callstack.empty()) {
        this->step();
    }
}

bool VM::step() {
    if (this->callstack.empty()) return false;
    auto& frame = this->callstack.back();
    auto& fn = *frame.ctx;
    if (frame.iptr >= fn.instructions.size()) {
        throw std::string{"instruction pointer overran instruction list"};
    }
    auto instr = fn.instructions[frame.iptr];
    auto op0 = instr.operand0.value_or(0);
    
    switch (instr.operation) {
        case Operation::LoadConst:
            frame.opstack.push_back(from_constant(fn.constants_[op0]));
            frame.iptr += 1;
            break;
        case Operation::LoadGlobal:
            break;
        case Operation::LoadLocal:
            break;
        case Operation::LoadFunc:
            break;
        case Operation::LoadReference:
            break;
        case Operation::StoreGlobal:
            break;
        case Operation::StoreLocal:
            break;
        case Operation::StoreReference:
            break;
        case Operation::PushReference:
            break;
        case Operation::Neg:
            if (frame.opstack.empty()) {
                throw std::string{"empty opstack"};
            }
            Value v = frame.opstack.back();
            if (!std::holds_alternative<int>(v)) {
                throw std::string{"operand error"};
            }
            frame.opstack.pop_back();
            frame.opstack.push_back(-std::get<int>(v));
            frame.iptr += 1;
            break;
        case Operation::Add:
            size_t stack_size = frame.opstack.size();
            if (stack_size < 2) {
                throw std::string{"stack size less than 2"};
            }
            Value lop = frame.opstack[stack_size - 2];
            Value rop = frame.opstack[stack_size - 1];
            frame.opstack.pop_back();
            frame.opstack.pop_back();
            frame.opstack.push_back()
            break;
        case Operation::Sub:
            break;
        case Operation::Mul:
            break;
        case Operation::Div:
            break;
        case Operation::Gt:
            break;
        case Operation::Geq:
            break;
        case Operation::Eq:
            break;
        case Operation::And:
            break;
        case Operation::Or:
            break;
        case Operation::Not:
            break;
        case Operation::AllocClosure:
            break;
        case Operation::AllocRecord:
            break;
        case Operation::Call:
            break;
        case Operation::FieldLoad:
            break;
        case Operation::FieldStore:
            break;
        case Operation::IndexLoad:
            break;
        case Operation::IndexStore:
            break;
        case Operation::Return:
            break;
        case Operation::Pop:
            break;
        case Operation::Goto:
            break;
        case Operation::If:
            break;
        case Operation::Dup:
            break;
        case Operation::Swap:
            break;
        default:
            throw std::string{"internal compiler error: invalid instruction"};
    }
}