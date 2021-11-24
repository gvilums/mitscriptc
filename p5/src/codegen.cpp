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

Executable::Executable(IR::Program&& program) : ctx_ptr(program.rt) {
    using namespace asmjit;
    MyErrorHandler handler;
    FileLogger logger(stdout);

    CodeHolder code;
    code.init(jit_rt.environment());
    code.setErrorHandler(&handler);
    code.setLogger(&logger);

    CodeGenerator generator{std::move(program), &code};

//    x86::Assembler assembler(&code);
//    assembler.addValidationOptions(BaseEmitter::kValidationOptionAssembler);

//    this->program.rt->init_globals(program.num_globals);


//    CodeGenState cg_state;
//    this->state = &cg_state;
//    cg_state.function_labels.resize(program.functions.size());
//    for (auto& label : cg_state.function_labels) {
//        label = assembler.newLabel();
//    }

    // start at global function
//    assembler.jmp(cg_state.function_labels.back());

//    cg_state.function_address_base_label = assembler.newLabel();
//    assembler.bind(cg_state.function_address_base_label);
//    for (const auto& label : cg_state.function_labels) {
//        assembler.embedLabel(label);
//    }

//    for (size_t i = 0; i < program.functions.size(); ++i) {
//        process_function(assembler, cg_state, i);
//    }
//    this->state = nullptr;
    Error err = this->jit_rt.add(&this->function, &code);
    if (err) {
        std::cout << DebugUtils::errorAsString(err) << std::endl;
    }
}

void CodeGenerator::process_function(size_t func_index) {
    std::cout << "------- function " << func_index << "---------" << std::endl;
    using namespace asmjit;
    assembler.bind(function_labels[func_index]);
    const IR::Function& func = this->program.functions[func_index];
    assert(!func.blocks.empty());
    // traverse blocks of function
    auto order = get_block_dfs_order(func);
    assembler.push(x86::rbp);
    assembler.mov(x86::rbp, x86::rsp);

    // reserve stack slots. To ensure 16-byte alignment at function call time, align stack to 16 bytes + 8
    if (func.stack_slots % 2 == 1) {
        assembler.sub(x86::rsp, 8 * func.stack_slots);
    } else {
        assembler.sub(x86::rsp, 8 * (func.stack_slots + 1));
    }

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

void CodeGenerator::process_block(
                               const IR::Function& func,
                               size_t block_index,
                               std::vector<asmjit::Label>& block_labels) {
    std::cout << "------- block " << block_index << "---------" << std::endl;
    using namespace asmjit;
    const IR::BasicBlock block = func.blocks[block_index];
    assembler.bind(block_labels[block_index]);
    for (const auto& instr : block.instructions) {
        if (instr.op == IR::Operation::ADD) {
            load(x86::r10, instr.args[0]);
            load(x86::rdx, instr.args[1]);
            assembler.mov(x86::rsi, x86::r10);
            assembler.mov(x86::rdi, Imm(program.rt));
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
            load(x86::r10, instr.args[0]);
            load(x86::rsi, instr.args[1]);
            assembler.mov(x86::rdi, x86::r10);
            assembler.call(Imm(runtime::value_eq));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::GT) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            assembler.setg(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::GEQ) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            assembler.cmp(x86::r10, x86::r11);
            assembler.setge(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
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
            size_t arg_id = instr.args[0].index;
            if (arg_id > 6) {
                int32_t offset = 8 * (instr.args[0].index - 4);
                assembler.mov(x86::r10, x86::ptr_64(x86::rbp, offset));
                store(instr.out, x86::r10);
            } else {
                store(instr.out, to_reg(arg_id));
            }
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
            load(x86::r10, instr.args[0]);
            load(x86::rsi, instr.args[1]);
            assembler.mov(x86::rdi, x86::r10);
            assembler.call(Imm(runtime::extern_rec_load_name));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_LOAD_INDX) {
            load(x86::r10, instr.args[0]);
            load(x86::rdx, instr.args[1]);
            assembler.mov(x86::rsi, x86::r10);
            assembler.mov(x86::rdi, Imm(program.rt));
            assembler.call(Imm(runtime::extern_rec_load_index));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_STORE_NAME) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            load(x86::rdx, instr.args[2]);
            assembler.mov(x86::rdi, x86::r10);
            assembler.mov(x86::rsi, x86::r11);
            assembler.call(Imm(runtime::extern_rec_store_name));
        } else if (instr.op == IR::Operation::REC_STORE_INDX) {
            load(x86::r10, instr.args[0]);
            load(x86::r11, instr.args[1]);
            load(x86::rcx, instr.args[2]);
            assembler.mov(x86::rsi, x86::r10);
            assembler.mov(x86::rdx, x86::r11);
            assembler.mov(x86::rdi, Imm(program.rt));
            assembler.call(Imm(runtime::extern_rec_store_index));
        } else if (instr.op == IR::Operation::ALLOC_REF) {
            assembler.mov(x86::rdi, Imm(program.rt));
            assembler.call(Imm(runtime::extern_alloc_ref));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_REC) {
            assembler.mov(x86::rdi, Imm(program.rt));
            assembler.call(Imm(runtime::extern_alloc_record));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_CLOSURE) {
            int32_t fn_id = instr.args[0].index;
            assembler.mov(x86::rdi, Imm(program.rt));
            // arg2 is number of free vars
            assembler.mov(x86::rsi, instr.args[2].index);
            assembler.call(Imm(runtime::extern_alloc_closure));
            // load function address
            assembler.mov(x86::r11, x86::rax);
            assembler.and_(x86::r11, Imm(~0b1111));
            assembler.mov(x86::r10, x86::Mem(function_address_base_label, 8 * fn_id));
            assembler.mov(x86::Mem(x86::r11, 0), x86::r10);
            assembler.mov(x86::qword_ptr(x86::r11, 8), Imm(instr.args[1].index));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SET_CAPTURE) {
            int32_t offset = 24 + 8 * instr.args[0].index;
            load(x86::r10, instr.args[1]);
            load(x86::r11, instr.args[2]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, offset), x86::r11);
        } else if (instr.op == IR::Operation::INIT_CALL) {
            size_t num_args = instr.args[0].index;
            if (num_args > 6) {
                current_stack_args = num_args - 6;
            } else {
                current_stack_args = 0;
            }

            // save current function
            assembler.push(x86::rbx);
            // if number of stack arguments is even, stack has to be pushed to preserve alignment:
            // (even) [rbx] [NULL] (odd) [rip]
            // notice that [rbx] messes up alignment
            if (current_stack_args % 2 == 0) {
                assembler.push(Imm(0));
            }
            if (current_stack_args > 0) {
                assembler.sub(x86::rsp, Imm(8 * current_stack_args));
            }
        } else if (instr.op == IR::Operation::SET_ARG) {
            int32_t arg_index = instr.args[0].index;
            // passed in register
            if (arg_index < 6) {
                load(to_reg(arg_index), instr.args[1]);
            } else {  // passed on stack
                load(x86::r10, instr.args[1]);
                assembler.mov(x86::Mem(x86::rsp, 8 * arg_index), x86::r10);
            }
        } else if (instr.op == IR::Operation::EXEC_CALL) {
            // TODO function argument count validation
            load(x86::r10, instr.args[0]);
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::rbx, x86::r10);
            assembler.call(x86::Mem(x86::r10, 0));
            if (instr.out.type != IR::Operand::NONE) {
                store(instr.out, x86::rax);
            }

            int32_t stack_delta = current_stack_args;
            if (current_stack_args % 2 == 0) {
                stack_delta += 1;
            }
            assembler.add(x86::rsp, Imm(8 * stack_delta));
//            if (state.current_stack_args > 0) {
//                if (state.current_stack_args % 2 == 1) {
//                    assembler.add(x86::rsp, Imm(8 * (state.current_stack_args + 1)));
//                } else {
//                    assembler.add(x86::rsp, Imm(8 * state.current_stack_args));
//                }
//            }
            assembler.pop(x86::rbx);
        } else if (instr.op == IR::Operation::MOV) {
            x86::Gp target;
            if (instr.out.type == IR::Operand::MACHINE_REG) {
                target = to_reg(instr.out.index);
            } else {
                target = x86::r10;
            }
            load(target, instr.args[0]);
            if (instr.out.type != IR::Operand::MACHINE_REG) {
                store(instr.out, target);
            }
        } else if (instr.op == IR::Operation::LOAD_GLOBAL) {
            // TODO check for uninit and terminate
            int32_t offset = 8 * instr.args[0].index;
            assembler.mov(x86::r11, Imm(program.rt->globals));
            assembler.mov(x86::r10, x86::qword_ptr(x86::r11, offset));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::STORE_GLOBAL) {
            int32_t offset = 8 * instr.args[0].index;
            load(x86::r10, instr.args[1]);
            assembler.mov(x86::r11, Imm(program.rt->globals));
            assembler.mov(x86::ptr_64(x86::r11, offset), x86::r10);
        } else if (instr.op == IR::Operation::ASSERT_BOOL) {
        } else if (instr.op == IR::Operation::ASSERT_INT) {
        } else if (instr.op == IR::Operation::ASSERT_STRING) {
        } else if (instr.op == IR::Operation::ASSERT_RECORD) {
        } else if (instr.op == IR::Operation::ASSERT_CLOSURE) {
        } else if (instr.op == IR::Operation::ASSERT_NONZERO) {
        } else if (instr.op == IR::Operation::PRINT) {
            load(x86::rdi, instr.args[0]);
            assembler.call(Imm(runtime::extern_print));
        } else if (instr.op == IR::Operation::INPUT) {
            assembler.mov(x86::rdi, Imm(program.rt));
            assembler.call(Imm(runtime::extern_input));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::INTCAST) {
            load(x86::rdi, instr.args[0]);
            assembler.call(Imm(runtime::extern_intcast));
            // TODO check for invalid
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::SWAP) {
            if (instr.args[0].type != IR::Operand::MACHINE_REG ||
                instr.args[1].type != IR::Operand::MACHINE_REG) {
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
        case IR::Operand::IMMEDIATE: {
            assembler.mov(reg, Imm(program.immediates[op.index]));
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

void CodeGenerator::store(const IR::Operand& op, const asmjit::x86::Gp& reg) {
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

CodeGenerator::CodeGenerator(IR::Program&& program1, asmjit::CodeHolder* code_holder)
    : program(std::move(program1)), assembler(code_holder) {

    program.rt->init_globals(program.num_globals);
    assembler.addValidationOptions(asmjit::BaseEmitter::kValidationOptionAssembler);

    function_address_base_label = assembler.newLabel();
    function_labels.resize(program.functions.size());
    for (auto& label : function_labels) {
        label = assembler.newLabel();
    }

    assembler.jmp(function_labels.back());
    for (size_t i = 0; i < program.functions.size(); ++i) {
        process_function(i);
    }

    assembler.bind(function_address_base_label);
    for (const auto& label : function_labels) {
        assembler.embedLabel(label);
    }
}
};  // namespace codegen