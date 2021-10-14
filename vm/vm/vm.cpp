#include "vm.h"

#include <iostream>

#include "value.h"
#include "../instructions.h"
#include "../types.h"

ProgVal get_unary_operand(std::vector<StackVal>& stack) {
    if (stack.empty()) {
        throw std::string{"empty stack for unary operation"};
    }
    StackVal operand = stack.back();
    stack.pop_back();
    return std::get<ProgVal>(operand);
}

std::pair<ProgVal, ProgVal> get_bin_operands(std::vector<StackVal>& stack) {
    size_t stack_size = stack.size();
    if (stack_size < 2) {
        throw std::string{"stack holds too few operands for binary operation"};
    }
    StackVal lop = stack[stack_size - 2];
    StackVal rop = stack[stack_size - 1];

    stack.pop_back();
    stack.pop_back();

    return std::visit(
        overloaded{
            [](ProgVal x, ProgVal y) -> std::pair<ProgVal, ProgVal> { return std::pair{x, y}; },
            [](auto x, auto y) -> std::pair<ProgVal, ProgVal> { throw std::string{"stack operands for binary operation must be program values"}; }
        },
    lop, rop);
}

VM::VM(Function&& prog) : source(std::move(prog)) {
    this->callstack.push_back(StackFrame(&this->source));
    
    ProgVal* print_closure = new ProgVal;
    *print_closure = Closure{.type=FnType::PRINT, .fn=nullptr};
    ProgVal* input_closure = new ProgVal;
    *input_closure = Closure{.type=FnType::INPUT, .fn=nullptr};
    ProgVal* intcast_closure = new ProgVal;
    *intcast_closure = Closure{.type=FnType::INTCAST, .fn=nullptr};
    
    this->globals["print"] = RefCell{.ref=print_closure};
    this->globals["input"] = RefCell{.ref=input_closure};
    this->globals["intcast"] = RefCell{.ref=intcast_closure};
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
    
    if (instr.operation == Operation::LoadConst) {
        frame.opstack.push_back(value_from_constant(fn.constants_[instr.operand0.value()]));
        frame.iptr += 1;
    } else if (instr.operation == Operation::LoadGlobal) {
        frame.opstack.push_back(globals[fn.names_[instr.operand0.value()]]);
        frame.iptr += 1;
    } else if (instr.operation == Operation::LoadLocal) {
        frame.opstack.push_back(frame.locals[instr.operand0.value()]);
        frame.iptr += 1;
    } else if (instr.operation == Operation::LoadFunc) {
        frame.opstack.push_back(fn.functions_[instr.operand0.value()]);
        frame.iptr += 1;
    } else if (instr.operation == Operation::LoadReference) {
        if (frame.opstack.empty()) {
            throw std::string{"error: trying to load reference from empty stack"};
        }
        RefCell ref_cell = std::get<RefCell>(frame.opstack.back());
        frame.opstack.pop_back();
        frame.opstack.push_back(*ref_cell.ref);
        frame.iptr += 1;
    } else if (instr.operation == Operation::StoreGlobal) {
        ProgVal val = get_unary_operand(frame.opstack);
        std::string& name = fn.names_[instr.operand0.value()];
        this->globals[name] = val;
        frame.iptr += 1;
    } else if (instr.operation == Operation::StoreLocal) {
        ProgVal val = get_unary_operand(frame.opstack);
        frame.locals[instr.operand0.value()] = val;
        frame.iptr += 1;
    } else if (instr.operation == Operation::StoreReference) {
        ProgVal val = get_unary_operand(frame.opstack);
        if (frame.opstack.empty()) {
            throw std::string{"error: trying to store to reference with stack of size 1"};
        }
        RefCell ref_cell = std::get<RefCell>(frame.opstack.back());
        frame.opstack.pop_back();
        *ref_cell.ref = val;
        frame.iptr += 1;
    } else if (instr.operation == Operation::PushReference) {
        int32_t i = instr.operand0.value();
        frame.opstack.push_back(frame.refs[i]);
        frame.iptr += 1;
    } else if (instr.operation == Operation::Neg) {
        auto val = get_unary_operand(frame.opstack);
        frame.opstack.push_back(-std::get<int>(val));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Add) {
        auto ops = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::visit(overloaded{
            [](int x, int y) -> ProgVal { return x + y; },
            [](const std::string& l, const std::string& r) -> ProgVal { return l + r; },
            [](const std::string& l, auto r) -> ProgVal { return l + value_to_string(r); },
            [](auto l, const std::string& r) -> ProgVal { return value_to_string(l) + r; },
            [](auto l, auto r) -> ProgVal { throw std::string{"invalid operands for add"}; }
        }, ops.first, ops.second));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Sub) {
        auto [x, y] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<int>(x) - std::get<int>(y));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Mul) {
        auto [x, y] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<int>(x) * std::get<int>(y));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Div) {
        auto [x, y] = get_bin_operands(frame.opstack);
        int divisor = std::get<int>(y);
        if (divisor == 0) {
            throw std::string{"error: division by zero"};
        }
        frame.opstack.push_back(std::get<int>(x) / divisor);
        frame.iptr += 1;
    } else if (instr.operation == Operation::Gt) {
        auto [l, r] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<int>(l) > std::get<int>(r));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Geq) {
        auto [l, r] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<int>(l) >= std::get<int>(r));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Eq) {
        auto [l, r] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(value_eq(l, r));
        frame.iptr += 1;
    } else if (instr.operation == Operation::And) {
        auto [l, r] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<bool>(l) && std::get<bool>(r));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Or) {
        auto [l, r] = get_bin_operands(frame.opstack);
        frame.opstack.push_back(std::get<bool>(l) || std::get<bool>(r));
        frame.iptr += 1;
    } else if (instr.operation == Operation::Not) {
        auto val = get_unary_operand(frame.opstack);
        frame.opstack.push_back(!std::get<bool>(val));
        frame.iptr += 1;
    } else if (instr.operation == Operation::AllocClosure) {
        int32_t m = instr.operand0.value();
        if (frame.opstack.size() <= m) {
            throw std::string{"error: not enough stack arguments while allocating closure"};
        }
        std::vector<RefCell> refs;
        for (size_t i = 0; i < m; ++i) {
            refs.push_back(std::get<RefCell>(frame.opstack.back()));
            frame.opstack.pop_back();
        }
        Function* fn = std::get<Function*>(frame.opstack.back());
        frame.opstack.pop_back();

        ProgVal* closure = new ProgVal;
        *closure = Closure{.type=FnType::DEFAULT, .fn=fn, .refs=std::move(refs)};
        frame.opstack.push_back(RefCell{.ref=closure});
        frame.iptr += 1;
    } else if (instr.operation == Operation::AllocRecord) {
        auto* map = new std::map<std::string, ProgVal>();
        frame.opstack.push_back(Record{.internal=map});
        frame.iptr += 1;
    } else if (instr.operation == Operation::Call) {
        int32_t m = instr.operand0.value();
        if (frame.opstack.size() <= m) {
            throw std::string{"error: not enough stack arguments while calling closure"};
        }
        std::vector<ProgVal> args;
        for (size_t i = 0; i < m; ++i) {
            args.push_back(get_unary_operand(frame.opstack));
        }
        RefCell ref_cell = std::get<RefCell>(frame.opstack.back());
        frame.opstack.pop_back();
        Closure& c = std::get<Closure>(*ref_cell.ref);
        if (c.type == FnType::DEFAULT) {
            this->callstack.push_back(StackFrame(c.fn, std::move(args), c.refs));
        } else if (c.type == FnType::PRINT) {
            if (args.size() != 1) {
                throw std::string{"error: print requires exactly one argument"};
            }
            std::cout << value_to_string(args[0]) << std::endl;
        } else if (c.type == FnType::INPUT) {
            if (!args.empty()) {
                throw std::string{"error: input requires exactly zero arguments"};
            }
            std::string input;
            std::getline(std::cin, input);
            frame.opstack.push_back(input);
        } else if (c.type == FnType::INTCAST) {
            if (args.size() != 1) {
                throw std::string{"error: intcast requires exactly one argument"};
            }
            frame.opstack.push_back(std::stoi(std::get<std::string>(args[0])));
        } else {
            throw std::string{"internal vm error: undefined function type"};
        }
        frame.iptr += 1;
    } else if (instr.operation == Operation::FieldLoad) {
        auto val = get_unary_operand(frame.opstack);
        std::string& field_name = fn.names_[instr.operand0.value()];
        Record r = std::get<Record>(val);
        frame.opstack.push_back(r.internal->at(field_name));
        frame.iptr += 1;
    } else if (instr.operation == Operation::FieldStore) {
        auto [record_val, val] = get_bin_operands(frame.opstack);
        std::string& field_name = fn.names_[instr.operand0.value()];
        Record r = std::get<Record>(record_val);
        r.internal->insert_or_assign(field_name, val);
        frame.iptr += 1;
    } else if (instr.operation == Operation::IndexLoad) {
        auto [record_val, index] = get_bin_operands(frame.opstack);
        Record r = std::get<Record>(record_val);
        frame.opstack.push_back(r.internal->at(value_to_string(index)));
        frame.iptr += 1;
    } else if (instr.operation == Operation::IndexStore) {
        auto value = get_unary_operand(frame.opstack);
        auto [record_val, index] = get_bin_operands(frame.opstack);
        Record r = std::get<Record>(record_val);
        r.internal->insert_or_assign(value_to_string(index), value);
        frame.iptr += 1;
    } else if (instr.operation == Operation::Return) {
        if (frame.opstack.empty()) {
            throw std::string{"error: called return on empty stack"};
        }
        StackVal v = frame.opstack.back();
        this->callstack.pop_back();
        this->callstack.back().opstack.push_back(v);
    } else if (instr.operation == Operation::Pop) {
        if (frame.opstack.empty()) {
            throw std::string{"error: called pop on empty opstack"};
        } else {
            frame.opstack.pop_back();
        }
        frame.iptr += 1;
    } else if (instr.operation == Operation::Goto) {
        frame.iptr += instr.operand0.value();
    } else if (instr.operation == Operation::If) {
        if (frame.opstack.empty()) {
            throw std::string{"error: called if on empty stack"};
        }
        auto val = get_unary_operand(frame.opstack);
        if (std::get<bool>(val)) {
            frame.iptr += instr.operand0.value();
        } else {
            frame.iptr += 1;
        }
    } else if (instr.operation == Operation::Dup) {
        if (frame.opstack.empty()) {
            throw std::string{"error: called dup on empty stack"};
        } else {
            StackVal v = frame.opstack.back();
            frame.opstack.push_back(v);
        }
    } else if (instr.operation == Operation::Swap) {
        size_t stack_size = frame.opstack.size();
        if (stack_size < 2) {
            throw std::string{"error: swap executed on stack with less than 2 elements"};
        } else {
            StackVal tmp = frame.opstack.back();
            frame.opstack.back() = frame.opstack[stack_size - 2];
            frame.opstack[stack_size - 2] = tmp;
        }
        frame.iptr += 1;
    } else {
        throw std::string{"internal vm error: invalid opcode"};
    }
    return true;
}