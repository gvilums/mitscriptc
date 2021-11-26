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

Executable::Executable(IR::Program&& program) : ctx_ptr(program.ctx_ptr) {
    using namespace asmjit;
    MyErrorHandler handler;
    FileLogger logger(stdout);

    CodeHolder code;
    code.init(jit_rt.environment());
    code.setErrorHandler(&handler);
//    code.setLogger(&logger);

    CodeGenerator generator{std::move(program), &code};

    Error err = this->jit_rt.add(&this->function, &code);
    if (err) {
        std::cout << DebugUtils::errorAsString(err) << std::endl;
    }
}

void CodeGenerator::process_function(size_t func_index) {
//    std::cout << "------- function " << func_index << "---------" << std::endl;
    using namespace asmjit;
    assembler.bind(function_labels[func_index]);
    const IR::Function& func = this->program.functions[func_index];
    assert(!func.blocks.empty());
    // traverse blocks of function
    auto order = get_block_dfs_order(func);
    assembler.push(x86::rbp);
    assembler.mov(x86::rbp, x86::rsp);

    // reserve stack slots. To ensure 16-byte alignment at function call time, align stack to 16 bytes + 8
    // TODO CHECK THIS
    if (func.stack_slots % 2 == 0) {
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
//    std::cout << "------- block " << block_index << "---------" << std::endl;
    using namespace asmjit;
    const IR::BasicBlock block = func.blocks[block_index];
    assembler.bind(block_labels[block_index]);
    for (const auto& instr : block.instructions) {
        if (instr.op == IR::Operation::ADD) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::value_add));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ADD_INT) {
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.add(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::SUB) {
            assembler.shr(x86::r10, 4);
            assembler.shr(x86::r11, 4);
            assembler.sub(x86::r10d, x86::r11d);
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::MUL) {
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.imul(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::DIV) {
            assembler.shr(x86::rax, 4);
            assembler.shr(x86::r10, 4);
            assembler.cdq();
            assembler.idiv(x86::r10d);
            assembler.shl(x86::rax, 4);
            assembler.or_(x86::rax, Imm(static_cast<size_t>(runtime::ValueType::Int)));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::EQ) {
            assembler.call(Imm(runtime::value_eq));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::GT) {
            assembler.shr(x86::r10, Imm(4));
            assembler.shr(x86::r11, Imm(4));
            assembler.cmp(x86::r10d, x86::r11d);
            assembler.setg(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::GEQ) {
            assembler.shr(x86::r10, Imm(4));
            assembler.shr(x86::r11, Imm(4));
            assembler.cmp(x86::r10d, x86::r11d);
            assembler.setge(x86::r10b);
            assembler.and_(x86::r10, Imm(0b1));
            assembler.shl(x86::r10, 4);
            assembler.or_(x86::r10, Imm(static_cast<size_t>(runtime::ValueType::Bool)));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::AND) {
            assembler.and_(x86::r10, x86::r11);
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::OR) {
            assembler.or_(x86::r10, x86::r11);
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::NOT) {
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
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::r10, x86::Mem(x86::r10, 0));
            store(instr.out, x86::r10);
        } else if (instr.op == IR::Operation::REF_STORE) {
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, 0), x86::r11);
        } else if (instr.op == IR::Operation::REC_LOAD_NAME) {
            assembler.call(Imm(runtime::extern_rec_load_name));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_LOAD_INDX) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::extern_rec_load_index));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::REC_STORE_NAME) {
            assembler.call(Imm(runtime::extern_rec_store_name));
        } else if (instr.op == IR::Operation::REC_STORE_INDX) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::extern_rec_store_index));
        } else if (instr.op == IR::Operation::ALLOC_REF) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::extern_alloc_ref));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_REC) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::extern_alloc_record));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::ALLOC_CLOSURE) {
            int32_t fn_id = instr.args[0].index;
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
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
            assembler.and_(x86::r10, Imm(~0b1111));
            assembler.mov(x86::Mem(x86::r10, offset), x86::r11);
        } else if (instr.op == IR::Operation::INIT_CALL) {
            current_args = instr.args[0].index;
            int32_t stack_args = std::max(current_args - 6, 0);
            // save current function
            assembler.push(x86::rbx);
            // if number of stack arguments is even, stack has to be pushed to preserve alignment:
            // (even) [rbx] [NULL] (odd) [rip]
            // notice that [rbx] messes up alignment
            if (stack_args % 2 == 0) {
                assembler.push(Imm(0));
            }
            if (stack_args > 0) {
                assembler.sub(x86::rsp, Imm(8 * stack_args));
            }
        } else if (instr.op == IR::Operation::SET_ARG) {
            int32_t arg_index = instr.args[0].index;
            // passed in register
            if (arg_index < 6) {
                assembler.mov(to_reg(arg_index), x86::r10);
            } else {  // passed on stack
                assembler.mov(x86::Mem(x86::rsp, 8 * (arg_index - 6)), x86::r10);
            }
        } else if (instr.op == IR::Operation::EXEC_CALL) {
            load(x86::rbx, instr.args[0]);
            assembler.and_(x86::rbx, Imm(~0b1111));

            // validate number of arguments
            assembler.mov(x86::r10, x86::Mem(x86::rbx, 8));
            assembler.cmp(x86::r10, Imm(current_args));
            assembler.jne(rt_exception_label);

            assembler.call(x86::Mem(x86::rbx, 0));
            if (instr.out.type != IR::Operand::NONE) {
                store(instr.out, x86::rax);
            }

            int32_t stack_delta = std::max(current_args - 6, 0);
            if (stack_delta % 2 == 0) {
                stack_delta += 1;
            }
            assembler.add(x86::rsp, Imm(8 * stack_delta));
            assembler.pop(x86::rbx);
        } else if (instr.op == IR::Operation::MOV) {
            x86::Gp target;
            if (instr.out.type == IR::Operand::NONE) {
                continue;
            }
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
            int32_t offset = 8 * instr.args[0].index;
            assembler.mov(x86::r11, Imm(program.ctx_ptr->globals));
            assembler.mov(x86::r10, x86::qword_ptr(x86::r11, offset));
            store(instr.out, x86::r10);
            assembler.cmp(x86::r10, Imm(0b10000));
            assembler.je(uninit_var_label);
        } else if (instr.op == IR::Operation::STORE_GLOBAL) {
            int32_t offset = 8 * instr.args[0].index;
            assembler.mov(x86::r11, Imm(program.ctx_ptr->globals));
            assembler.mov(x86::ptr_64(x86::r11, offset), x86::r10);
        } else if (instr.op == IR::Operation::ASSERT_BOOL) {
            assembler.and_(x86::r10, Imm(0b1111));
            assembler.cmp(x86::r10, Imm(runtime::BOOL_TAG));
            assembler.jne(illegal_cast_label);
        } else if (instr.op == IR::Operation::ASSERT_INT) {
            assembler.and_(x86::r10, Imm(0b1111));
            assembler.cmp(x86::r10, Imm(runtime::INT_TAG));
            assembler.jne(illegal_cast_label);
        } else if (instr.op == IR::Operation::ASSERT_STRING) {
            // TODO implement or maybe remove, think about this
        } else if (instr.op == IR::Operation::ASSERT_RECORD) {
            assembler.and_(x86::r10, Imm(0b1111));
            assembler.cmp(x86::r10, Imm(runtime::RECORD_TAG));
            assembler.jne(illegal_cast_label);
        } else if (instr.op == IR::Operation::ASSERT_CLOSURE) {
            assembler.and_(x86::r10, Imm(0b1111));
            assembler.cmp(x86::r10, Imm(runtime::CLOSURE_TAG));
            assembler.jne(illegal_cast_label);
        } else if (instr.op == IR::Operation::ASSERT_NONZERO) {
            assembler.shr(x86::r10, Imm(4));
            assembler.cmp(x86::r10, Imm(0));
            assembler.je(illegal_arith_label);
        } else if (instr.op == IR::Operation::PRINT) {
            assembler.call(Imm(runtime::extern_print));
        } else if (instr.op == IR::Operation::INPUT) {
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.call(Imm(runtime::extern_input));
            store(instr.out, x86::rax);
        } else if (instr.op == IR::Operation::INTCAST) {
            assembler.call(Imm(runtime::extern_intcast));
            assembler.cmp(x86::rax, 0b10000);
            assembler.je(illegal_cast_label);
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
            assembler.mov(x86::rsp, x86::rbp);
            assembler.pop(x86::rbp);
            assembler.ret();
        } else if (instr.op == IR::Operation::GC) {
            Label skip_gc_label = assembler.newLabel();
            assembler.mov(x86::r10, Imm(&program.ctx_ptr->current_alloc));
            assembler.mov(x86::r10, x86::ptr_64(x86::r10));
            assembler.mov(x86::r11, Imm(program.ctx_ptr->gc_threshold));
            assembler.cmp(x86::r10, x86::r11);
            assembler.jl(skip_gc_label);
            std::bitset<IR::MACHINE_REG_COUNT> live_regs(instr.args[0].index);
            int num_live = 0;
            for (int i = 0; i < IR::MACHINE_REG_COUNT; ++i) {
                if (live_regs.test(i)) {
                    assembler.push(to_reg(i));
                    num_live += 1;
                }
            }
            if (num_live % 2 != 0) {
                assembler.push(0);
            }
            // call tracer to perform gc
            assembler.mov(x86::rdi, Imm(program.ctx_ptr));
            assembler.mov(x86::rsi, x86::rbp);
            assembler.mov(x86::rdx, x86::rsp);
            assembler.call(Imm(runtime::trace_collect));
            if (num_live % 2 != 0) {
                assembler.add(x86::rsp, Imm(8));
            }
            for (int i = IR::MACHINE_REG_COUNT - 1; i >= 0; --i) {
                if (live_regs.test(i)) {
                    assembler.pop(to_reg(i));
                }
            }
            assembler.bind(skip_gc_label);
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
//            std::cerr << "storing to none, no instruction emitted" << std::endl;
            break;
        default:
            assert(false && "invalid operand");
    }
}

void Executable::run() {
    int exit_code = this->function();
    if (exit_code != 0) {
        throw RuntimeException(exit_code);
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

    program.ctx_ptr->init_globals(program.num_globals);
    assembler.addValidationOptions(asmjit::BaseEmitter::kValidationOptionAssembler);

//    function_address_base_label = assembler.newLabel();
//    function_labels.resize(program.functions.size());
//    for (auto& label : function_labels) {
//        label = assembler.newLabel();
//    }
    init_labels();
    generate_prelude();

//    assembler.jmp(function_labels.back());

    for (size_t i = 0; i < program.functions.size(); ++i) {
        process_function(i);
    }

    assembler.bind(function_address_base_label);
    for (const auto& label : function_labels) {
        assembler.embedLabel(label);
    }
    program.ctx_ptr = nullptr;
}

void CodeGenerator::generate_prelude() {
    using namespace asmjit;
    auto reg_restore_label = assembler.newLabel();

    save_volatile();
    // default call procedure
    assembler.call(function_labels.back());
    assembler.mov(x86::rax, Imm(0));
    assembler.jmp(reg_restore_label);

    assembler.bind(uninit_var_label);
    assembler.mov(x86::rax, Imm(1));
    assembler.jmp(reg_restore_label);

    assembler.bind(illegal_cast_label);
    assembler.mov(x86::rax, Imm(2));
    assembler.jmp(reg_restore_label);

    assembler.bind(illegal_arith_label);
    assembler.mov(x86::rax, Imm(3));
    assembler.jmp(reg_restore_label);

    assembler.bind(rt_exception_label);
    assembler.mov(x86::rax, Imm(4));
    assembler.jmp(reg_restore_label);

    assembler.bind(reg_restore_label);
    restore_volatile();
    assembler.ret();
}

void CodeGenerator::save_volatile() {
    using namespace asmjit;
    assembler.push(x86::rbx);
    assembler.push(x86::rsp);
    assembler.push(x86::rbp);
    assembler.push(x86::r12);
    assembler.push(x86::r13);
    assembler.push(x86::r14);
    assembler.push(x86::r15);

    assembler.mov(x86::r10, Imm(&program.ctx_ptr->saved_rsp));
    assembler.mov(x86::ptr_64(x86::r10), x86::rsp);
}

void CodeGenerator::restore_volatile() {
    using namespace asmjit;
    assembler.mov(x86::r10, Imm(&program.ctx_ptr->saved_rsp));
    assembler.mov(x86::rsp, x86::ptr_64(x86::r10));

    assembler.pop(x86::r15);
    assembler.pop(x86::r14);
    assembler.pop(x86::r13);
    assembler.pop(x86::r12);
    assembler.pop(x86::rbp);
    assembler.pop(x86::rsp);
    assembler.pop(x86::rbx);
}

void CodeGenerator::init_labels() {
    function_labels.resize(program.functions.size());
    for (auto& label : function_labels) {
        label = assembler.newLabel();
    }
    function_address_base_label = assembler.newLabel();
    uninit_var_label = assembler.newLabel();
    illegal_cast_label = assembler.newLabel();
    illegal_arith_label = assembler.newLabel();
    rt_exception_label = assembler.newLabel();
}

std::ostream& operator<<(std::ostream& os, const RuntimeException& exception) {
    switch (exception.type) {
        case 1:
            os << "UninitializedVariableException";
            break;
        case 2:
            os << "IllegalCastException";
            break;
        case 3:
            os << "IllegalArithmeticException";
            break;
        case 4:
            os << "RuntimeException";
            break;
        default:
            os << "INVALID EXCEPTION STATE";
            break;
    }
    return os;
}
RuntimeException::RuntimeException(int kind) : type(kind) {}

};  // namespace codegen