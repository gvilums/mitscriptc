#include "codegen.h"
#include <stack>
#include "value.h"
#include <cassert>

namespace codegen {

class MyErrorHandler : public asmjit::ErrorHandler {
   public:
    void handleError(asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override {
        printf("AsmJit error: %s\n", message);
    }
};

Executable::Executable(IR::Program&& program1) : program{std::move(program1)} {
    using namespace asmjit;
    MyErrorHandler handler;
    FileLogger logger(stdout);

    CodeHolder code;
    code.init(jit_rt.environment());

    x86::Assembler assembler(&code);
    assembler.addValidationOptions(BaseEmitter::kValidationOptionAssembler);

    this->program.rt->init_globals(program.num_globals);

    code.setErrorHandler(&handler);
    code.setLogger(&logger);

    CodeGenState cg_state;
    this->state = &cg_state;
    cg_state.function_labels.resize(program.functions.size());
    for (auto& label : cg_state.function_labels) {
        label = assembler.newLabel();
    }

    // start at global function
    assembler.jmp(cg_state.function_labels.back());

    cg_state.function_address_base_label = assembler.newLabel();
    assembler.bind(cg_state.function_address_base_label);
    for (const auto& label : cg_state.function_labels) {
        assembler.embedLabel(label);
    }

    cg_state.context_ptr_label = assembler.newLabel();
    assembler.bind(cg_state.context_ptr_label);
    assembler.embedUInt64(reinterpret_cast<uint64_t>(this->program.rt));

    cg_state.globals_ptr_label = assembler.newLabel();
    assembler.bind(cg_state.globals_ptr_label);
    assembler.embedUInt64(reinterpret_cast<uint64_t>(this->program.rt->globals));

    cg_state.const_pool_label = assembler.newLabel();
    assembler.bind(cg_state.const_pool_label);
    for (auto immediate : program.immediates) {
        assembler.embedUInt64(immediate);
    }

    for (size_t i = 0; i < program.functions.size(); ++i) {
        process_function(assembler, cg_state, i);
    }
    this->state = nullptr;
    Error err = this->jit_rt.add(&this->function, &code);
    if (err) {
        std::cout << DebugUtils::errorAsString(err) << std::endl;
    }
}

void Executable::process_function(asmjit::x86::Assembler& assembler,
                                  CodeGenState& state,
                                  size_t func_index) {
    std::cout << "------- function " << func_index << "---------" << std::endl;
    using namespace asmjit;
    assembler.bind(state.function_labels[func_index]);
    const IR::Function& func = this->program.functions[func_index];
    assert(!func.blocks.empty());
    // traverse blocks of function
    auto order = get_block_dfs_order(func);
    assembler.push(x86::rbp);
    assembler.mov(x86::rbp, x86::rsp);
    // reserve stack slots (note: this needs potential rework in future)
    assembler.sub(x86::rsp, 8 * func.stack_slots);

    // init vector of block labels to be accessed by id
    std::vector<Label> block_labels(func.blocks.size());
    for (auto& label : block_labels) {
        label = assembler.newLabel();
    }

    for (size_t block_index = 0; block_index < func.blocks.size(); ++block_index) {
        const IR::BasicBlock& block = func.blocks[block_index];
        process_block(assembler, state, func, block_index, block_labels);
        // process branch instruction if block has multiple successors
        size_t num_successors = block.successors.size();
        if (num_successors == 2) {
            const IR::Instruction& instr = block.instructions.back();
            load(assembler, x86::r10, instr.args[0]);
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

void Executable::process_block(asmjit::x86::Assembler& assembler,
                               CodeGenState& state,
                               const IR::Function& func,
                               size_t block_index,
                               std::vector<asmjit::Label>& block_labels) {
    std::cout << "------- block " << block_index << "---------" << std::endl;
    using namespace asmjit;
    const IR::BasicBlock block = func.blocks[block_index];
    assembler.bind(block_labels[block_index]);
    for (const auto& instr : block.instructions) {
        if (instr.op == IR::Operation::ADD) {
            load(assembler, x86::rsi, instr.args[0]);
            load(assembler, x86::rdx, instr.args[1]);
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            assembler.call(Imm(runtime::value_add));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ADD_INT) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.add(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::SUB) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.sub(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::MUL) {
            load(assembler, x86::rax, instr.args[0]);
            load(assembler, x86::r10, instr.args[1]);
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.imul(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::DIV) {
            load(assembler, x86::rax, instr.args[0]);
            load(assembler, x86::r10, instr.args[1]);
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.xor_(x86::edx, x86::edx);
            assembler.idiv(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::EQ) {
            load(assembler, x86::rdi, instr.args[0]);
            load(assembler, x86::rsi, instr.args[1]);
            assembler.call(Imm(runtime::value_eq));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::GT) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            // TODO SETcc instructions
            assembler.setg(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::GEQ) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            assembler.setge(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::AND) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.and_(x86::r10, x86::r11);
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::OR) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.or_(x86::r10, x86::r11);
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::NOT) {
            load(assembler, x86::r10, instr.args[0]);
            assembler.shr(x86::r10, 4);
            assembler.mov(x86::r11, Imm(1));
            assembler.sub(x86::r11, x86::r10);
            assembler.shl(x86::r11, 4);
            assembler.or_(x86::r11, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(assembler, instr.out, x86::r11);
        } else if (instr.op == IR::Operation::LOAD_ARG) {
            // +2 because saved rbp and rip are at rbp
            size_t arg_id = instr.args[0].index;
            if (arg_id > 6) {
                int32_t offset = 8 * (instr.args[0].index - 4);
                assembler.mov(x86::r10, x86::ptr_64(x86::rbp, offset));
                store(assembler, instr.out, x86::r10);
            } else {
                store(assembler, instr.out, to_reg(arg_id));
            }
        } else if (instr.op == IR::Operation::LOAD_FREE_REF) {
            int32_t offset = 24 + 8 * instr.args[0].index;
            assembler.mov(x86::r10, x86::Mem(x86::rbx, offset));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::REF_LOAD) {
            load(assembler, x86::r10, instr.args[0]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::r10, x86::Mem(x86::r10, 0));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::REF_STORE) {
            load(assembler, x86::r10, instr.args[0]);
            load(assembler, x86::r11, instr.args[1]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, 0), x86::r11);
        } else if (instr.op == IR::Operation::REC_LOAD_NAME) {
            load(assembler, x86::rdi, instr.args[0]);
            load(assembler, x86::rsi, instr.args[1]);
            assembler.call(Imm(runtime::extern_rec_load_name));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_LOAD_INDX) {
            load(assembler, x86::rdx, instr.args[1]);
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            load(assembler, x86::rsi, instr.args[0]);
            assembler.call(Imm(runtime::extern_rec_load_index));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_STORE_NAME) {
            load(assembler, x86::rdx, instr.args[2]);
            load(assembler, x86::rdi, instr.args[0]);
            load(assembler, x86::rsi, instr.args[1]);
            assembler.call(Imm(runtime::extern_rec_store_name));
        } else if (instr.op == IR::Operation::REC_STORE_INDX) {
            // TODO what if rdx and rcx need to be swapped?
            load(assembler, x86::rdx, instr.args[1]);
            load(assembler, x86::rcx, instr.args[2]);
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            load(assembler, x86::rsi, instr.args[0]);
            assembler.call(Imm(runtime::extern_rec_store_index));
        } else if (instr.op == IR::Operation::ALLOC_REF) {
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            assembler.call(Imm(runtime::extern_alloc_ref));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_REC) {
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            assembler.call(Imm(runtime::extern_alloc_record));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_CLOSURE) {
            int32_t fn_id = instr.args[0].index;
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            // arg2 is number of free vars
            assembler.mov(x86::rsi, instr.args[2].index);
            assembler.call(Imm(runtime::extern_alloc_closure));
            // load function address
            assembler.mov(x86::r11, x86::rax);
            assembler.and_(x86::r11, Imm(~0b1111));
            assembler.mov(x86::r10, x86::Mem(state.function_address_base_label, 8 * fn_id));
            assembler.mov(x86::Mem(x86::r11, 0), x86::r10);
            assembler.mov(x86::qword_ptr(x86::r11, 8), Imm(instr.args[1].index));
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SET_CAPTURE) {
            int32_t offset = 24 + 8 * instr.args[0].index;
            load(assembler, x86::r10, instr.args[1]);
            load(assembler, x86::r11, instr.args[2]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, offset), x86::r11);
        } else if (instr.op == IR::Operation::INIT_CALL) {
            size_t num_args = instr.args[0].index;
            if (num_args > 6) {
                state.current_stack_args = num_args - 6;
            } else {
                state.current_stack_args = 0;
            }

            // save current function
            assembler.push(x86::rbx);
            // if number of stack arguments is odd, stack has to be pushed to preserve alignment:
            // (even) [rbx] [NULL] (odd) [rip]
            if (state.current_stack_args % 2 == 1) {
                assembler.push(Imm(0));
            }
            if (state.current_stack_args > 0) {
                assembler.sub(x86::rsp, Imm(8 * state.current_stack_args));
            }
        } else if (instr.op == IR::Operation::SET_ARG) {
            int32_t arg_index = instr.args[0].index;
            // passed in register
            if (arg_index < 6) {
                load(assembler, to_reg(arg_index), instr.args[1]);
            } else {  // passed on stack
                load(assembler, x86::r10, instr.args[1]);
                assembler.mov(x86::Mem(x86::rsp, 8 * arg_index), x86::r10);
            }
        } else if (instr.op == IR::Operation::EXEC_CALL) {
            // TODO function argument count validation
            load(assembler, x86::r10, instr.args[0]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::rbx, x86::r10);
            assembler.call(x86::Mem(x86::r10, 0));
            if (instr.out.type != IR::Operand::NONE) {
                store(assembler, instr.out, x86::rax);
            }

            // restore current closure pointer
            if (state.current_stack_args > 0) {
                if (state.current_stack_args % 2 == 1) {
                    assembler.add(x86::rsp, Imm(8 * (state.current_stack_args + 1)));
                } else {
                    assembler.add(x86::rsp, Imm(8 * state.current_stack_args));
                }
            }
            assembler.pop(x86::rbx);
        } else if (instr.op == IR::Operation::MOV) {
            x86::Gp target;
            if (instr.out.type == IR::Operand::MACHINE_REG) {
                target = to_reg(instr.out.index);
            } else {
                target = x86::r10;
            }
            load(assembler, target, instr.args[0]);
            if (instr.out.type != IR::Operand::MACHINE_REG) {
                store(assembler, instr.out, target);
            }
        } else if (instr.op == IR::Operation::LOAD_GLOBAL) {
            // TODO check for uninit and terminate
            int32_t offset = 8 * instr.args[0].index;
            assembler.mov(x86::r11, x86::ptr_64(state.globals_ptr_label));
            assembler.mov(x86::r10, x86::qword_ptr(x86::r11, offset));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::STORE_GLOBAL) {
            int32_t offset = 8 * instr.args[0].index;
            load(assembler, x86::r10, instr.args[1]);
            assembler.mov(x86::r11, x86::ptr_64(state.globals_ptr_label));
            assembler.mov(x86::ptr_64(x86::r11, offset), x86::r10);
        } else if (instr.op == IR::Operation::ASSERT_BOOL) {
        } else if (instr.op == IR::Operation::ASSERT_INT) {
        } else if (instr.op == IR::Operation::ASSERT_STRING) {
        } else if (instr.op == IR::Operation::ASSERT_RECORD) {
        } else if (instr.op == IR::Operation::ASSERT_CLOSURE) {
        } else if (instr.op == IR::Operation::ASSERT_NONZERO) {
        } else if (instr.op == IR::Operation::PRINT) {
            load(assembler, x86::rdi, instr.args[0]);
            assembler.call(Imm(runtime::extern_print));
        } else if (instr.op == IR::Operation::INPUT) {
            assembler.mov(x86::rdi, x86::ptr_64(state.context_ptr_label, 0));
            assembler.call(Imm(runtime::extern_input));
            store(assembler, instr.out, x86::r10);
        } else if (instr.op == IR::Operation::INTCAST) {
            load(assembler, x86::rdi, instr.args[0]);
            assembler.call(Imm(runtime::extern_intcast));
            // TODO check for invalid
            store(assembler, instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SWAP) {
            if (instr.args[0].type != IR::Operand::MACHINE_REG ||
                instr.args[1].type != IR::Operand::MACHINE_REG) {
                assert(false && "emitted swap with stack slots");
            }
            assembler.xchg(to_reg(instr.args[0].index), to_reg(instr.args[1].index));
        } else if (instr.op == IR::Operation::BRANCH) {
            break;
        } else if (instr.op == IR::Operation::RETURN) {
            load(assembler, x86::rax, instr.args[0]);
            assembler.mov(x86::rsp, x86::rbp);
            assembler.pop(x86::rbp);
            assembler.ret();
        } else {
            assert(false);
        }
    }
}

void Executable::load(asmjit::x86::Assembler& assembler,
                      const asmjit::x86::Gp& reg,
                      const IR::Operand& op) {
    using namespace asmjit;
    switch (op.type) {
        case IR::Operand::IMMEDIATE: {
            int32_t offset = 8 * op.index;
            assembler.mov(reg, x86::ptr_64(this->state->const_pool_label, offset));
        } break;
        case IR::Operand::MACHINE_REG:
            assembler.mov(reg, to_reg(op.index));
            break;
        case IR::Operand::STACK_SLOT:
            assembler.mov(reg, to_mem(op.index));
            break;
        case IR::Operand::NONE:
            std::cerr << "loading from none, no instruction emitted" << std::endl;
            break;
        default:
            assert(false && "invalid operand");
    }
}

void Executable::store(asmjit::x86::Assembler& assembler,
                       const IR::Operand& op,
                       const asmjit::x86::Gp& reg) {
    using namespace asmjit;
    switch (op.type) {
        case IR::Operand::MACHINE_REG:
            assembler.mov(to_reg(op.index), reg);
            break;
        case IR::Operand::STACK_SLOT:
            assembler.mov(to_mem(op.index), reg);
            break;
        case IR::Operand::NONE:
            std::cerr << "storing to none, no instruction emitted" << std::endl;
            break;
        default:
            assert(false && "invalid operand");
    }
}

void Executable::run() {
    int exit_code = this->function();
    switch (exit_code) {
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
            visited[current] = true;
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
    return {asmjit::x86::rbp, 8 * (-stack_slot - 1)};
}

};  // namespace codegen