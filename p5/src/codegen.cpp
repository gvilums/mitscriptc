#include "value.h"
#include "codegen.h"
#include <stack>

namespace codegen {

CodeGenerator::CodeGenerator(IR::Program&& program1)
    : program{std::move(program1)}, assembler(&this->code), rt{program1.rt} {
    assembler.addValidationOptions(asmjit::BaseEmitter::kValidationOptionAssembler);
    this->code.init(this->jit_rt.environment());
    this->function_labels.resize(this->program.functions.size());
    for (auto& label : this->function_labels) {
        label = assembler.newLabel();
    }
    this->function_address_label = assembler.newLabel();
    assembler.bind(this->function_address_label);
    for (const auto& label : this->function_labels) {
        assembler.embedLabel(label);
    }

    for (size_t i = 0; i < program.functions.size(); ++i) {
        process_function(i);
    }

    this->jit_rt.add(&this->function, &this->code);
}

void CodeGenerator::process_function(size_t func_index) {
    using namespace asmjit;
    this->assembler.bind(this->function_labels[func_index]);
    const IR::Function& func = this->program.functions[func_index];
    assert(!func.blocks.empty());
    // traverse blocks of function
    auto order = get_block_dfs_order(func);
    this->assembler.push(x86::rbp);
    this->assembler.mov(x86::rbp, x86::rsp);
    // reserve stack slots (note: this needs potential rework in future)
    this->assembler.sub(x86::rsp, 8 * func.stack_slots);

    // init vector of block labels to be accessed by id
    std::vector<Label> block_labels(func.blocks.size());
    for (auto& label : block_labels) {
        label = assembler.newLabel();
    }

    for (size_t block_index = 0; block_index < func.blocks.size(); ++block_index) {
        const IR::BasicBlock& block = func.blocks[block_index];
        process_block(func, block_index, block_labels);
        // process branch instruction if block has multiple successors
        size_t num_successors = block.successors.size();
        if (num_successors == 2) {
            const IR::Instruction& instr = block.instructions.back();
            load(x86::r10, instr.args[0]);
            assembler.shr(x86::r10, 4);
            assembler.test(x86::r10, x86::r10);
            assembler.jz(block_labels[block.successors.back()]);
            if (block.successors.front() != block_index + 1) {
                assembler.jmp(block_labels[block.successors.front()]);
            }
        } else if (num_successors == 1) {
            // jump to successor needed
            size_t successor_index = block.successors.back();
            if (block_index + 1 != successor_index) {
                assembler.jmp(block_labels[successor_index]);
            }
        } else if (num_successors == 0) {
            // do nothing, we will have returned by now
        } else {
            assert(false);
        }
    }

}

void CodeGenerator::process_block(const IR::Function& func,
                                  size_t block_index,
                                  std::vector<asmjit::Label>& block_labels) {
    using namespace asmjit;
    const IR::BasicBlock block = func.blocks[block_index];
    this->assembler.bind(block_labels[block_index]);
    for (const auto& instr : block.instructions) {
        if (instr.op == IR::Operation::ADD) {
            assembler.mov(x86::rdi, Imm(this->rt));
            load(x86::rsi, instr.args[0]);
            load(x86::rdx, instr.args[1]);
            assembler.call(Imm(runtime::value_add));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ADD_INT) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.add(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::SUB) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.sub(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::MUL) {
            load(x86::rax, instr.args[0]);
            load(x86::r10, instr.args[1]);
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.imul(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::DIV) {
            load(x86::rax, instr.args[0]);
            load(x86::r10, instr.args[1]);
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.xor_(x86::edx, x86::edx);
            assembler.idiv(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::EQ) {
            load(x86::rdi, instr.args[0]);
            load(x86::rsi, instr.args[1]);
            assembler.call(Imm(runtime::value_eq));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::GT) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            // TODO SETcc instructions
            assembler.setg(x86::r10);
            assembler.shr(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::GEQ) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            assembler.setge(x86::r10);
            assembler.shr(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::AND) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.and_(x86::r10, x86::r11);
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::OR) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.or_(x86::r10, x86::r11);
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::NOT) {
            load(x86::r10, instr.args[0]);
            assembler.shr(x86::r10, 4);
            assembler.mov(x86::r11, Imm(1));
            assembler.sub(x86::r11, x86::r10);
            assembler.shl(x86::r11, 4);
            assembler.or_(x86::r11, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r11);
        } else if (instr.op == IR::Operation::LOAD_ARG) {
            // +2 because saved rbp and rip are at rbp
            int32_t offset = 8 * (instr.args[0].index + 2);
            assembler.mov(x86::r10, x86::Mem(x86::rbp, offset));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::LOAD_FREE_REF) {
            int32_t offset = 24 + 8 * instr.args[0].index;
            assembler.mov(x86::r10, x86::Mem(x86::rbx, offset));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::REF_LOAD) {
            load(x86::r10, instr.args[0]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::r10, x86::Mem(x86::r10, 0));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::REF_STORE) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, 0), x86::r11);
        } else if (instr.op == IR::Operation::REC_LOAD_NAME) {
            load(x86::rdi, instr.args[0]);
            load(x86::rsi, instr.args[1]);
            assembler.call(Imm(runtime::extern_rec_load_name));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_LOAD_INDX) {
            assembler.mov(x86::rdi, Imm(this->rt));
            load(x86::rsi, instr.args[0]);
            load(x86::rdx, instr.args[1]);
            assembler.call(Imm(runtime::extern_rec_load_index));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_STORE_NAME) {
            load(x86::rdi, instr.args[0]);
            load(x86::rsi, instr.args[1]);
            load(x86::rdx, instr.args[2]);
            assembler.call(Imm(runtime::extern_rec_store_name));
        } else if (instr.op == IR::Operation::REC_STORE_INDX) {
            assembler.mov(x86::rdi, Imm(this->rt));
            load(x86::rsi, instr.args[0]);
            load(x86::rdx, instr.args[1]);
            load(x86::rcx, instr.args[2]);
            assembler.call(Imm(runtime::extern_rec_store_index));
        } else if (instr.op == IR::Operation::ALLOC_REF) {
            assembler.mov(x86::rdi, Imm(this->rt));
            assembler.call(Imm(runtime::extern_alloc_ref));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_REC) {
            assembler.mov(x86::rdi, Imm(this->rt));
            assembler.call(Imm(runtime::extern_alloc_record));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_CLOSURE) {
            int32_t fn_id = instr.args[0].index;
            assembler.mov(x86::rdi, this->rt);
            assembler.mov(x86::rsi, instr.args[1].index);
            assembler.call(Imm(runtime::extern_alloc_closure));
            // load function address
            assembler.mov(x86::r10, x86::Mem(this->function_address_label, 8 * fn_id));
            assembler.mov(x86::Mem(x86::rax, 0), x86::r10);
            assembler.mov(x86::Mem(x86::rax, 8), Imm(this->program.functions[fn_id].parameter_count));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SET_CAPTURE) {
            int32_t offset = 24 + 8 * instr.args[0].index;
            load(x86::r10, instr.args[1]);
            load(x86::r11, instr.args[2]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, offset), x86::r11);
        } else if (instr.op == IR::Operation::INIT_CALL) {
            size_t num_args = instr.args[0].index;
            this->current_call_args = num_args;
            // save current function
            assembler.push(x86::rbx);
            // if number of arguments is odd, stack has to be pushed to preserve alignment:
            // (even) [rbx] [NULL] (odd) [rip]
            if (num_args % 2 == 1) {
                assembler.push(Imm(0));
            }
            assembler.sub(x86::rsp, Imm(8 * num_args));
        } else if (instr.op == IR::Operation::SET_ARG) {
            int32_t arg_index = instr.args[0].index;
            // passed in register
            if (arg_index < 6) {
                load(to_reg(arg_index), instr.args[1]);
            } else { // passed on stack
                load(x86::r10, instr.args[1]);
                assembler.mov(x86::Mem(x86::rsp, 8 * arg_index), x86::r10);
            }
        } else if (instr.op == IR::Operation::EXEC_CALL) {
            // TODO function argument count validation

            load(x86::r10, instr.args[0]);
            assembler.mov(x86::rbx, x86::Mem(x86::r10, 0));
            assembler.call(x86::Mem(x86::r10, 0));
            store(instr.out, x86::rax);

            // restore current closure pointer
            if (this->current_call_args % 2 == 1) {
                assembler.sub(x86::rsp, Imm(8 * (this->current_call_args + 1)));
            } else {
                assembler.sub(x86::rsp, Imm(8 * this->current_call_args));
            }
            assembler.pop(x86::rbx);
        } else if (instr.op == IR::Operation::MOV) {
            x86::Gp target;
            if (instr.out.type == IR::Operand::MACHINE_REG) {
                target = to_reg(instr.out.index);
            } else {
                target = x86::r10;
            }
            load(target, instr.args[0]);
            if (instr.out.type == IR::Operand::MACHINE_REG) {
                store(instr.out, target);
            }
        } else if (instr.op == IR::Operation::LOAD_GLOBAL) {
            // TODO check for uninit and terminate
            int32_t offset = 8 * instr.args[0].index;
            assembler.mov(x86::r10, x86::Mem(reinterpret_cast<uint64_t>(this->rt->globals), offset));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::STORE_GLOBAL) {
            int32_t offset = 8 * instr.args[0].index;
            load(x86::r10, instr.args[1]);
            assembler.mov(x86::Mem(reinterpret_cast<uint64_t>(this->rt->globals), offset), x86::r10);
        } else if (instr.op == IR::Operation::ASSERT_BOOL) {

        } else if (instr.op == IR::Operation::ASSERT_INT) {

        } else if (instr.op == IR::Operation::ASSERT_STRING) {

        } else if (instr.op == IR::Operation::ASSERT_RECORD) {

        } else if (instr.op == IR::Operation::ASSERT_CLOSURE) {

        } else if (instr.op == IR::Operation::ASSERT_NONZERO) {

        } else if (instr.op == IR::Operation::PRINT) {
            assembler.mov(x86::rdi, Imm(this->rt));
            load(x86::rsi, instr.args[0]);
            assembler.call(Imm(runtime::extern_print));
        } else if (instr.op == IR::Operation::INPUT) {
            assembler.mov(x86::rdi, Imm(this->rt));
            assembler.call(Imm(runtime::extern_input));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::INTCAST) {
            load(x86::rdi, instr.args[0]);
            assembler.call(Imm(runtime::extern_intcast));
            // TODO check for invalid
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SWAP) {
            if (instr.args[0].type != IR::Operand::MACHINE_REG
                || instr.args[1].type != IR::Operand::MACHINE_REG) {
                assert(false && "emitted swap with stack slots");
            }
            assembler.xchg(to_reg(instr.args[0].index), to_reg(instr.args[1].index));
        } else if (instr.op == IR::Operation::BRANCH) {
            break;
        } else if (instr.op == IR::Operation::RETURN) {
            load(x86::rax, instr.args[0]);
            assembler.mov(x86::rsp, x86::rbp);
            assembler.pop(x86::rbp);
            assembler.ret();
        } else {
            assert(false);
        }
    }
}

void CodeGenerator::load(const asmjit::x86::Gp& reg, const IR::Operand& op) {
    using namespace asmjit;
    switch (op.type) {
        case IR::Operand::IMMEDIATE:
            assembler.mov(reg, this->program.immediates[op.index]);
            break;
        case IR::Operand::MACHINE_REG:
            assembler.mov(reg, to_reg(op.index));
            break;
        case IR::Operand::STACK_SLOT:
            assembler.mov(reg, to_mem(op.index));
            break;
        default:
            assert(false && "invalid operand");
    }
}

void CodeGenerator::store(const IR::Operand& op, const asmjit::x86::Gp& reg) {
    using namespace asmjit;
    switch (op.type) {
        case IR::Operand::MACHINE_REG:
            assembler.mov(to_reg(op.index), reg);
            break;
        case IR::Operand::STACK_SLOT:
            assembler.mov(to_mem(op.index), reg);
            break;
        default:
            assert(false && "invalid operand");
    }
}

void CodeGenerator::run() {
    int exit_code = this->function();
    switch(exit_code) {
        case 0:
            return;
        default:
            assert(false && "error handling not yet implemented");
    }
}

auto get_block_dfs_order(const IR::Function& func) -> std::vector<size_t> {
    std::vector<size_t> block_order;
    std::stack<size_t> block_stack;
    std::vector<bool> visited(func.blocks.size(), false);
    block_stack.push(0);
    while (!block_stack.empty()) {
        size_t current = block_stack.top();
        block_stack.pop();
        if (!visited[current]) {
            block_order.push_back(current);
            for (auto successor : func.blocks[current].successors) {
                block_stack.push(successor);
            }
        }
    }
    assert(block_order.size() == func.blocks.size());
    return block_order;
}

auto to_reg(size_t reg_index) -> asmjit::x86::Gp {
    using namespace asmjit::x86;
    switch (static_cast<IR::MachineReg>(reg_index)) {
        case IR::MachineReg::RDI:
            return rdi;
        case IR::MachineReg::RSI:
            return rsi;
        case IR::MachineReg::RDX:
                return rdx;
        case IR::MachineReg::RCX:
            return rcx;
        case IR::MachineReg::R8:
            return r8;
        case IR::MachineReg::R9:
            return r9;
        case IR::MachineReg::RAX:
            return rax;
        case IR::MachineReg::R11:
            return r11;
        case IR::MachineReg::R12:
            return r12;
        case IR::MachineReg::R13:
            return r13;
        case IR::MachineReg::R14:
            return r14;
        case IR::MachineReg::R15:
            return r15;
        case IR::MachineReg::RBX:
            return rbx;
        case IR::MachineReg::R10:
            return r10;
    }
    assert(false && "invalid register assignment");
}

auto to_mem(int32_t stack_slot) -> asmjit::x86::Mem {
    return {asmjit::x86::rbp, 8 * stack_slot};
}

};