#include "compiler.h"
#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include "AST.h"
#include "Assigns.h"
#include "FreeVariables.h"
#include "ir.h"
#include "irprinter.h"
#include "value.h"

using namespace std;

Compiler::Compiler(size_t heap_size) {
    program_ = new IR::Program(heap_size);
    layout_map_cnt_ = 0;

    IR::Function print_ = {
        {{{},
          {{IR::Operation::LOAD_ARG,
            {IR::Operand::OpType::VIRT_REG, 0},
            {IR::Operand::OpType::LOGICAL, 0}},
           {IR::Operation::PRINT, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 0}},
           {IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::IMMEDIATE, 0}}},
          {},
          {},
          false,
          0}},
        1,
        1};
    IR::Function input_ = {
        {{{},
          {{IR::Operation::INPUT, {IR::Operand::OpType::VIRT_REG, 0}},
           {IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 0}}},
          {},
          {},
          false,
          0}},
        1,
        0};
    IR::Function intcast_ = {
        {{{},
          {{IR::Operation::LOAD_ARG, {IR::Operand::OpType::VIRT_REG, 0}},
           {IR::Operation::ASSERT_STRING, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 0}},
           {IR::Operation::INTCAST,
            {IR::Operand::OpType::VIRT_REG, 1},
            {IR::Operand::OpType::VIRT_REG, 0}},
           {IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 1}}},
          {},
          {},
          false,
          0}},
        2,
        1};

    program_->functions.push_back(print_);
    program_->functions.push_back(input_);
    program_->functions.push_back(intcast_);
    fun_ = new IR::Function();  // this is the global scope
    block_ = IR::BasicBlock();

    names_["print"] = 0;
    names_["input"] = 1;
    names_["intcast"] = 2;
    globals_.insert("print");
    globals_.insert("input");
    globals_.insert("intcast");

    program_->immediates.push_back(0);                         // NONE
    program_->immediates.push_back(runtime::to_value(true));   // TRUE
    program_->immediates.push_back(runtime::to_value(false));  // FALSE
    program_->immediates.push_back(runtime::to_value(0));      // 0

    reg_cnt_ = 0;
    imm_cnt_ = 4;
    names_cnt_ = 3;
    int_const_[0] = 3;

    global_scope_ = true;
    is_opr_ = false;

    // LOAD GLOBALS into functions !!!

    IR::Instruction a_closure;
    a_closure.op = IR::Operation::ALLOC_CLOSURE;
    a_closure.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    a_closure.args[0] = {IR::Operand::OpType::LOGICAL, 0};
    a_closure.args[1] = {IR::Operand::OpType::LOGICAL, 1};
    a_closure.args[2] = {IR::Operand::OpType::LOGICAL, 0};

    IR::Instruction store_glob;
    store_glob.op = IR::Operation::STORE_GLOBAL;
    store_glob.args[0] = {IR::Operand::OpType::LOGICAL, 0};
    store_glob.args[1] = {IR::Operand::OpType::VIRT_REG, reg_cnt_++};

    block_.instructions.push_back(a_closure);
    block_.instructions.push_back(store_glob);

    a_closure.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    a_closure.args[0] = {IR::Operand::OpType::LOGICAL, 1};
    a_closure.args[1] = {IR::Operand::OpType::LOGICAL, 0};
    store_glob.args[0] = {IR::Operand::OpType::LOGICAL, 1};
    store_glob.args[1] = {IR::Operand::OpType::VIRT_REG, reg_cnt_++};

    block_.instructions.push_back(a_closure);
    block_.instructions.push_back(store_glob);

    a_closure.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    a_closure.args[0] = {IR::Operand::OpType::LOGICAL, 2};
    a_closure.args[1] = {IR::Operand::OpType::LOGICAL, 1};
    store_glob.args[0] = {IR::Operand::OpType::LOGICAL, 2};
    store_glob.args[1] = {IR::Operand::OpType::VIRT_REG, reg_cnt_++};

    block_.instructions.push_back(a_closure);
    block_.instructions.push_back(store_glob);
}

IR::Program* Compiler::get_program() {
    return program_;
}

void Compiler::visit(AST::Program& expr) {
    for (auto c : expr.children)
        c->accept(*((Visitor*)this));

    
    block_.instructions.push_back({IR::Operation::RETURN, {}, {IR::Operand::IMMEDIATE, 0}});
    fun_->blocks.push_back(block_);
    fun_->virt_reg_count = reg_cnt_;
    program_->functions.push_back(*fun_);
    delete fun_;
    program_->num_globals = (int) names_cnt_;
}

void Compiler::visit(AST::Block& expr) {
    for (auto c : expr.children)
        c->accept(*((Visitor*)this));
}

void Compiler::visit(AST::Global& expr) {
    program_->ref_globals.insert(names_[expr.name]);
}

void Compiler::visit(AST::Return& expr) {
    expr.Expr->accept(*((Visitor*)this));
    if (!is_opr_)
        opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
    block_.instructions.push_back({IR::Operation::GC, IR::Operand(), {}});
    block_.instructions.push_back({IR::Operation::RETURN, IR::Operand(), opr_});
}

void Compiler::visit(AST::Assignment& expr) {
    if (expr.Lhs->isStringConstant()) {
        string s = ((AST::StringConstant*)expr.Lhs)->getVal();

        if (global_scope_ && !globals_.count(s)) {
            if (!names_.count(s))
                names_[s] = names_cnt_++;
            globals_.insert(s);
        }

        expr.Expr->accept(*((Visitor*)this));

        if (globals_.count(s)) {
            if (!is_opr_)
                opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};

            IR::Instruction store_glob;
            store_glob.op = IR::Operation::STORE_GLOBAL;
            store_glob.args[0] = {IR::Operand::OpType::LOGICAL, names_[s]};
            store_glob.args[1] = opr_;

            block_.instructions.push_back(store_glob);
        } else if (local_reference_vars_.count(s)) {
            if (!is_opr_)
                opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
            IR::Instruction s_ref;
            s_ref.op = IR::Operation::REF_STORE;
            s_ref.args[0] = {IR::Operand::OpType::VIRT_REG, local_vars_[s]};
            s_ref.args[1] = opr_;
            block_.instructions.push_back(s_ref);
        } else {  // check for error ?
            if (is_opr_) {
                block_.instructions.push_back(
                    {IR::Operation::MOV, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr_});
                local_vars_[s] = reg_cnt_++;
            } else
                local_vars_[s] = ret_reg_;
        }
    } else if (expr.Lhs->isFieldDereference()) {
        AST::FieldDereference* exp = ((AST::FieldDereference*)expr.Lhs);
        exp->baseexpr->accept(*((Visitor*)this));
        int rec_reg = ret_reg_;
        block_.instructions.push_back({IR::Operation::ASSERT_RECORD,
                                       IR::Operand(),
                                       {IR::Operand::OpType::VIRT_REG, rec_reg}});

        expr.Expr->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};

        int idx;
        if (!str_const_.count(exp->field)) {
            program_->immediates.push_back(runtime::to_value(program_->ctx_ptr, exp->field));
            str_const_[exp->field] = imm_cnt_++;
        }
        idx = str_const_[exp->field];

        IR::Instruction store_field;
        store_field.op = IR::Operation::REC_STORE_NAME;
        store_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
        store_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
        store_field.args[2] = opr_;
        block_.instructions.push_back(store_field);

    } else if (expr.Lhs->isIndexExpression()) {
        AST::IndexExpression* exp = ((AST::IndexExpression*)expr.Lhs);
        exp->baseexpr->accept(*((Visitor*)this));
        int rec_reg = ret_reg_;
        block_.instructions.push_back({IR::Operation::ASSERT_RECORD,
                                       IR::Operand(),
                                       {IR::Operand::OpType::VIRT_REG, rec_reg}});

        exp->index->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
        IR::Operand idx_opr = opr_;

        expr.Expr->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};

        IR::Instruction store_index;
        store_index.op = IR::Operation::REC_STORE_INDX;
        store_index.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
        store_index.args[1] = idx_opr;
        store_index.args[2] = opr_;
        block_.instructions.push_back(store_index);
    }
}

void Compiler::visit(AST::IfStatement& expr) {
    int last_idx = fun_->blocks.size(), cond_idx, last_if_idx, last_else_idx, next_idx;
    last_idx = fun_->blocks.size();
    cond_idx = last_idx + 1;
    last_else_idx = cond_idx;

    block_.successors.push_back(cond_idx);
    fun_->blocks.push_back(block_);

    block_ = IR::BasicBlock();
    block_.predecessors.push_back(last_idx);
    block_.successors.push_back(cond_idx + 1);
    expr.Expr->accept(*((Visitor*)this));

    if (!is_opr_)
        opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
    block_.instructions.push_back({IR::Operation::ASSERT_BOOL, IR::Operand(), opr_});
    block_.instructions.push_back({IR::Operation::BRANCH, IR::Operand(), opr_});

    fun_->blocks.push_back(block_);

    std::map<std::string, int> tlocal_vars1 = local_vars_, tlocal_vars2 = local_vars_;
    block_ = IR::BasicBlock();
    block_.predecessors.push_back(cond_idx);

    expr.children[0]->accept(*((Visitor*)this));

    last_if_idx = fun_->blocks.size();
    fun_->blocks.push_back(block_);
    tlocal_vars1 = local_vars_;

    if (expr.children.size() > 1) {
        local_vars_ = tlocal_vars2;

        block_ = IR::BasicBlock();
        block_.predecessors.push_back(cond_idx);
        fun_->blocks[cond_idx].successors.push_back(last_if_idx + 1);

        expr.children[1]->accept(*((Visitor*)this));
        last_else_idx = fun_->blocks.size();

        fun_->blocks.push_back(block_);
        tlocal_vars2 = local_vars_;
    }

    next_idx = fun_->blocks.size();
    block_ = IR::BasicBlock();
    block_.predecessors.push_back(last_if_idx);
    fun_->blocks[last_if_idx].successors.push_back(next_idx);

    if (expr.children.size() > 1) {
        fun_->blocks[last_else_idx].successors.push_back(next_idx);
        block_.predecessors.push_back(last_else_idx);
    } else {
        fun_->blocks[cond_idx].successors.push_back(next_idx);
        block_.predecessors.push_back(cond_idx);
    }

    for (const auto& var : tlocal_vars1) {
        if (tlocal_vars2[var.first] != var.second) {
            IR::PhiNode pn;
            pn.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
            pn.args.push_back({last_if_idx, {IR::Operand::OpType::VIRT_REG, var.second}});
            pn.args.push_back(
                {last_else_idx, {IR::Operand::OpType::VIRT_REG, tlocal_vars2[var.first]}});

            block_.phi_nodes.push_back(pn);
            local_vars_[var.first] = reg_cnt_;
            reg_cnt_++;
        }
    }
}

void Compiler::visit(AST::WhileLoop& expr) {  // could contain empty blocks
    int last_idx, header_idx, last_body_idx;

    last_idx = fun_->blocks.size();
    header_idx = last_idx + 1;
    block_.successors.push_back(header_idx);
    fun_->blocks.push_back(block_);

    block_ = IR::BasicBlock();
    block_.predecessors.push_back(last_idx);
    block_.successors.push_back(last_idx + 2);
    block_.is_loop_header = true;
    expr.Expr->accept(*((Visitor*)this));

    if (!is_opr_)
        opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
    block_.instructions.push_back({IR::Operation::ASSERT_BOOL, IR::Operand(), opr_});
    block_.instructions.push_back({IR::Operation::BRANCH, IR::Operand(), opr_});
    fun_->blocks.push_back(block_);

    std::map<std::string, int> tlocal_vars = local_vars_;
    block_ = IR::BasicBlock();
    block_.predecessors.push_back(header_idx);

    expr.children[0]->accept(*((Visitor*)this));
    last_body_idx = fun_->blocks.size();
    block_.successors.push_back(header_idx);
    fun_->blocks[header_idx].predecessors.push_back(last_body_idx);
    fun_->blocks[header_idx].final_loop_block = last_body_idx;
    block_.instructions.push_back({IR::Operation::GC, IR::Operand(), {}});
    fun_->blocks.push_back(block_);

    std::map<int, int> new_args;
    for (const auto& var : tlocal_vars) {
        if (local_vars_[var.first] != var.second) {
            IR::PhiNode pn;
            pn.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
            pn.args.push_back({last_idx, {IR::Operand::OpType::VIRT_REG, var.second}});
            pn.args.push_back(
                {last_body_idx, {IR::Operand::OpType::VIRT_REG, local_vars_[var.first]}});

            new_args[var.second] = reg_cnt_;

            fun_->blocks[header_idx].phi_nodes.push_back(pn);
            local_vars_[var.first] = reg_cnt_;
            reg_cnt_++;
        }
    }

    for (size_t i = last_body_idx; i > last_idx; i--) {
        if (i != header_idx) {
            for (auto& pn : fun_->blocks[i].phi_nodes) {
                for (auto& op : pn.args)
                    if (op.second.type == IR::Operand::OpType::VIRT_REG &&
                        new_args.count(op.second.index))
                        op.second.index = new_args[op.second.index];
            }
        }

        for (auto& ins : fun_->blocks[i].instructions) {
            for (auto& op : ins.args)
                if (op.type == IR::Operand::OpType::VIRT_REG && new_args.count(op.index))
                    op.index = new_args[op.index];
        }
    }

    block_ = IR::BasicBlock();
    fun_->blocks[header_idx].successors.push_back(last_body_idx + 1);
    block_.predecessors.push_back(header_idx);
}

void Compiler::visit(AST::FunctionDeclaration& expr) {
    IR::Function* tfun = fun_;
    IR::BasicBlock tblock = block_;
    bool tscope = global_scope_;
    std::set<string> tglobals = globals_;
    std::set<string> tref = ref_;
    int treg_cnt = reg_cnt_;
    int tret_reg = ret_reg_;

    std::map<std::string, int> tlocal_vars = std::move(local_vars_);
    std::set<std::string> tlocal_reference_vars = std::move(local_reference_vars_);
    std::map<std::string, int> tfree_vars = std::move(free_vars_);

    local_vars_ = std::map<std::string, int>();
    local_reference_vars_ = std::set<std::string>();
    free_vars_ = std::map<std::string, int>();

    fun_ = new IR::Function;
    block_ = IR::BasicBlock();
    reg_cnt_ = 0;
    global_scope_ = false;
    fun_->parameter_count = expr.arguments.size();

    for (const auto& s : tlocal_reference_vars)
        ref_.insert(s);

    FreeVariables freeVar;
    expr.block->accept(freeVar);
    vector<string> free_var = freeVar.getFreeVariables();
    vector<string> cur_var = freeVar.getCurVariables();
    vector<string> nb_var = freeVar.getNotBoundVariables();

    Assigns assigns;
    expr.block->accept(assigns);
    vector<string> ass_var = assigns.getVariables();

    Globals globals;
    expr.block->accept(globals);
    vector<string> glob_var = globals.getGlobals();

    // QUI INIZIA IL CASINO

    for (const auto& s : expr.arguments) {
        local_vars_[s] = reg_cnt_++;
        if (count(nb_var.begin(), nb_var.end(), s))
            local_reference_vars_.insert(s);
        if (globals_.count(s))
            globals_.erase(s);
        if (ref_.count(s))
            ref_.erase(s);
    }

    for (const auto& s : glob_var) {
        globals_.insert(s);
        if (!names_.contains(s))
            names_[s] = names_cnt_++;
        if (ref_.count(s))
            ref_.erase(s);
    }

    std::set<std::string> new_ass;
    for (const auto& s : ass_var) {
        if (count(glob_var.begin(), glob_var.end(), s))
            continue;
        if (count(expr.arguments.begin(), expr.arguments.end(), s))
            continue;
        if (new_ass.count(s))
            continue;

        new_ass.insert(s);
        local_vars_[s] = reg_cnt_++;
        if (count(nb_var.begin(), nb_var.end(), s))
            local_reference_vars_.insert(s);
        if (globals_.count(s))
            globals_.erase(s);
        if (ref_.count(s))
            ref_.erase(s);
    }

    for (const auto& s : free_var)
        if (ref_.count(s))
            free_vars_[s] = -1;

    IR::Operand fun_reg = {IR::Operand::OpType::VIRT_REG, treg_cnt};
    int b_idx = tblock.instructions.size();
    tblock.instructions.push_back({});

    tret_reg = treg_cnt;
    treg_cnt++;

    int args_idx = 0;
    std::vector<IR::Instruction> a_refs;
    for (const auto& s: expr.arguments) {
        IR::Instruction l_arg;
        l_arg.op = IR::Operation::LOAD_ARG;
        l_arg.out = {IR::Operand::OpType::VIRT_REG, local_vars_[s]};
        l_arg.args[0] = {IR::Operand::OpType::LOGICAL, args_idx++};
        block_.instructions.push_back(l_arg);

        if (local_reference_vars_.count(s)) {
            int ass_cnt = (int) count(ass_var.begin(), ass_var.end(), s);
            int new_reg = reg_cnt_++;
            a_refs.push_back(
                {IR::Operation::ALLOC_REF, {IR::Operand::OpType::VIRT_REG, new_reg},{IR::Operand::OpType::LOGICAL, ass_cnt}});
            IR::Instruction s_ref;
            s_ref.op = IR::Operation::REF_STORE;
            s_ref.args[0] = {IR::Operand::OpType::VIRT_REG, new_reg};
            s_ref.args[1] = {IR::Operand::OpType::VIRT_REG, local_vars_[s]};
            local_vars_[s] = new_reg;
            a_refs.push_back(s_ref);
        }
    }

    for (const auto &s : a_refs)
        block_.instructions.push_back(s);

    int idx = 0;
    vector<IR::Instruction> instr;
   
    for (const auto& s : free_vars_) {
        block_.instructions.push_back({IR::Operation::LOAD_FREE_REF,
                                       {IR::Operand::OpType::VIRT_REG, reg_cnt_},
                                       {IR::Operand::OpType::LOGICAL, idx}});
        free_vars_[s.first] = reg_cnt_++;  // could be done later

        if (tlocal_reference_vars.count(s.first)) {
            IR::Instruction set_c;
            set_c.op = IR::Operation::SET_CAPTURE;
            set_c.args[0] = {IR::Operand::OpType::LOGICAL, idx};
            set_c.args[1] = fun_reg;
            set_c.args[2] = {IR::Operand::OpType::VIRT_REG, tlocal_vars[s.first]};
            instr.push_back(set_c);
        } else if (tfree_vars.count(s.first)) {
            // tblock.instructions.push_back({IR::Operation::LOAD_FREE_REF, {IR::Operand::OpType::VIRT_REG, treg_cnt}, {IR::Operand::OpType::VIRT_REG, tfree_vars[s.first]}});

            IR::Instruction set_c;
            set_c.op = IR::Operation::SET_CAPTURE;
            set_c.args[0] = {IR::Operand::OpType::LOGICAL, idx};
            set_c.args[1] = fun_reg;
            set_c.args[2] = {IR::Operand::OpType::VIRT_REG, tfree_vars[s.first]};
            instr.push_back(set_c);
            // treg_cnt++;
        }
        idx++;
    }

    for (auto ins : instr)
        tblock.instructions.push_back(ins);

    for (const auto& var : new_ass) {
        if (local_reference_vars_.count(var)) {
            int ass_cnt = (int) count(ass_var.begin(), ass_var.end(), var);
            block_.instructions.push_back(
                {IR::Operation::ALLOC_REF, {IR::Operand::OpType::VIRT_REG, local_vars_[var]}, {IR::Operand::OpType::LOGICAL, ass_cnt}});
            IR::Instruction s_ref;
            s_ref.op = IR::Operation::REF_STORE;
            s_ref.args[0] = {IR::Operand::OpType::VIRT_REG, local_vars_[var]};
            s_ref.args[1] = {IR::Operand::OpType::IMMEDIATE, 0};
            block_.instructions.push_back(s_ref);
        } else {
            block_.instructions.push_back({IR::Operation::MOV,
                                           {IR::Operand::OpType::VIRT_REG, local_vars_[var]},
                                           {IR::Operand::OpType::IMMEDIATE, 0}});
        }
    }

    expr.block->accept(*((Visitor*)this));

    is_opr_ = false;

    IR::Instruction a_closure;
    a_closure.op = IR::Operation::ALLOC_CLOSURE;
    a_closure.out = fun_reg;
    a_closure.args[0] = {IR::Operand::OpType::LOGICAL, (int)program_->functions.size()};
    a_closure.args[1] = {IR::Operand::OpType::LOGICAL, (int)expr.arguments.size()};
    a_closure.args[2] = {IR::Operand::OpType::LOGICAL, (int)free_vars_.size()};
    tblock.instructions[b_idx] = a_closure;

    if (block_.instructions.empty() || block_.instructions.back().op != IR::Operation::RETURN) {
        block_.instructions.push_back({IR::Operation::GC, IR::Operand(), {}});
        block_.instructions.push_back({IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::IMMEDIATE, 0}});
    }

    fun_->virt_reg_count = reg_cnt_;
    fun_->blocks.push_back(block_);
    program_->functions.push_back(*fun_);
    delete fun_;

    local_vars_ = tlocal_vars;
    local_reference_vars_ = tlocal_reference_vars;
    free_vars_ = tfree_vars;
    fun_ = tfun;
    block_ = tblock;
    global_scope_ = tscope;
    globals_ = tglobals;
    ref_ = tref;
    reg_cnt_ = treg_cnt;
    ret_reg_ = tret_reg;
}

void Compiler::visit(AST::BinaryExpression& expr) {
    IR::Operand opr1, opr2;
    expr.children[0]->accept(*((Visitor*)this));
    if (is_opr_)
        opr1 = opr_;
    else
        opr1 = {IR::Operand::OpType::VIRT_REG, ret_reg_};
    expr.children[1]->accept(*((Visitor*)this));
    if (is_opr_)
        opr2 = opr_;
    else
        opr2 = {IR::Operand::OpType::VIRT_REG, ret_reg_};

    if (expr.op == "<=" || expr.op == "<") {
        IR::Operation opr_t = (expr.op == "<") ? IR::Operation::GEQ : IR::Operation::GT;
        block_.instructions.push_back(
            {opr_t, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr1, opr2});
        block_.instructions.push_back({IR::Operation::NOT,
                                       {IR::Operand::OpType::VIRT_REG, reg_cnt_ + 1},
                                       {IR::Operand::OpType::VIRT_REG, reg_cnt_}});
        ret_reg_ = reg_cnt_ + 1;
        is_opr_ = false;
        reg_cnt_ += 2;
        return;
    }

    IR::Operation op;
	bool bool_op = false, int_op = false;
    if (expr.op == "+")
        op = IR::Operation::ADD;
    else if (expr.op == "-")
        op = IR::Operation::SUB, int_op = true;
    else if (expr.op == "/")
        op = IR::Operation::DIV, int_op = true;
    else if (expr.op == "*")
        op = IR::Operation::MUL, int_op = true;
    else if (expr.op == ">")
        op = IR::Operation::GT, int_op = true;
    else if (expr.op == ">=")
        op = IR::Operation::GEQ, int_op = true;
    else if (expr.op == "==")
        op = IR::Operation::EQ;
    else if (expr.op == "&")
        op = IR::Operation::AND, bool_op = true;
    else if (expr.op == "|")
        op = IR::Operation::OR, bool_op = true;

    if (int_op) {
        block_.instructions.push_back({IR::Operation::ASSERT_INT, IR::Operand(), opr1});
        block_.instructions.push_back({IR::Operation::ASSERT_INT, IR::Operand(), opr2});
    } else if (bool_op) {
        block_.instructions.push_back({IR::Operation::ASSERT_BOOL, IR::Operand(), opr1});
        block_.instructions.push_back({IR::Operation::ASSERT_BOOL, IR::Operand(), opr2});
    }

    if (expr.op == "/")
        block_.instructions.push_back({IR::Operation::ASSERT_NONZERO, {}, opr2});
    
    IR::Instruction opr;
    opr.op = op;
    opr.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    opr.args[0] = opr1;
    opr.args[1] = opr2;
    block_.instructions.push_back(opr);
    is_opr_ = false;
    ret_reg_ = reg_cnt_;
    reg_cnt_++;
}

void Compiler::visit(AST::UnaryExpression& expr) {
    IR::Operand opr;
    expr.children[0]->accept(*((Visitor*)this));
    if (is_opr_)
        opr = opr_;
    else
        opr = {IR::Operand::OpType::VIRT_REG, ret_reg_};

    if (expr.op == "!") {
        block_.instructions.push_back({IR::Operation::ASSERT_BOOL, IR::Operand(), opr});
        block_.instructions.push_back(
            {IR::Operation::NOT, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr});
    } else {
        IR::Operand zero = {IR::Operand::OpType::IMMEDIATE, 3};
        block_.instructions.push_back({IR::Operation::ASSERT_INT, IR::Operand(), opr});
        block_.instructions.push_back(
            {IR::Operation::SUB, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, zero, opr});
    }
    is_opr_ = false;
    ret_reg_ = reg_cnt_;
    reg_cnt_++;
}

void Compiler::visit(AST::Call& expr) {
    expr.expr->accept(*((Visitor*)this));
    block_.instructions.push_back(
        {IR::Operation::ASSERT_CLOSURE, IR::Operand(), {IR::Operand::VIRT_REG, ret_reg_}});
    int fun_reg = ret_reg_;

    int arg_cnt = 0;
    std::vector<IR::Instruction> args;
    for (auto c : expr.arguments) {
        c->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
        
        IR::Instruction set_arg;
        set_arg.op = IR::Operation::SET_ARG;
        set_arg.args[0] = {IR::Operand::OpType::LOGICAL, arg_cnt++};
        set_arg.args[1] = opr_;

        args.push_back(set_arg);
        // block_.instructions.push_back(set_arg);
    }

    IR::Instruction icall;
    icall.op = IR::Operation::INIT_CALL;
    icall.args[0] = {IR::Operand::OpType::LOGICAL, arg_cnt};
    block_.instructions.push_back(icall);

    for (auto a : args)
        block_.instructions.push_back(a);

    IR::Instruction call;
    call.op = IR::Operation::EXEC_CALL;
    call.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    call.args[0] = {IR::Operand::OpType::VIRT_REG, fun_reg};
    block_.instructions.push_back(call);

    is_opr_ = false;
    ret_reg_ = reg_cnt_;
    reg_cnt_++;
}

void Compiler::visit(AST::FieldDereference& expr) {
    expr.baseexpr->accept(*((Visitor*)this));
    int rec_reg = ret_reg_;
    block_.instructions.push_back(
        {IR::Operation::ASSERT_RECORD, IR::Operand(), {IR::Operand::OpType::VIRT_REG, rec_reg}});

    int idx;
    if (!str_const_.count(expr.field)) {
        program_->immediates.push_back(runtime::to_value(program_->ctx_ptr, expr.field));
        str_const_[expr.field] = imm_cnt_++;
    }
    idx = str_const_[expr.field];

    IR::Instruction load_field;
    load_field.op = IR::Operation::REC_LOAD_NAME;
    load_field.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    load_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
    load_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
    block_.instructions.push_back(load_field);
    ret_reg_ = reg_cnt_;
    reg_cnt_++;
    is_opr_ = false;
}

void Compiler::visit(AST::IndexExpression& expr) {
    expr.baseexpr->accept(*((Visitor*)this));
    int rec_reg = ret_reg_;
    block_.instructions.push_back(
        {IR::Operation::ASSERT_RECORD, IR::Operand(), {IR::Operand::OpType::VIRT_REG, rec_reg}});

    expr.index->accept(*((Visitor*)this));
    if (!is_opr_)
        opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};

    IR::Instruction load_idx;
    load_idx.op = IR::Operation::REC_LOAD_INDX;
    load_idx.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
    load_idx.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
    load_idx.args[1] = opr_;
    block_.instructions.push_back(load_idx);
    ret_reg_ = reg_cnt_;
    reg_cnt_++;
    is_opr_ = false;
}

void Compiler::visit(AST::Record& expr) {
    int rec_reg = reg_cnt_;
    // find approriate 

    std::vector<std::string> std_fields;
    for (const auto &p : expr.dict) 
        std_fields.push_back(p.first);
    sort(std_fields.begin(), std_fields.end());
    std::vector<runtime::Value> fields;
    for (const auto &s : std_fields) {
        if (!str_const_.count(s)) {
            program_->immediates.push_back(runtime::to_value(program_->ctx_ptr, s));
            str_const_[s] = imm_cnt_++;
        }
        fields.push_back(program_->immediates[str_const_[s]]);
    }
    
    if (!layout_map_.count(std_fields)) {
        program_->struct_layouts.push_back(fields);
        layout_map_[std_fields] = layout_map_cnt_++;
    }

    IR::Instruction a_ref;
    a_ref.op = IR::Operation::ALLOC_REC;
    a_ref.out = {IR::Operand::OpType::VIRT_REG, rec_reg};
    a_ref.args[0] = {IR::Operand::OpType::LOGICAL, (int) expr.dict.size()};
    a_ref.args[1] = {IR::Operand::OpType::LOGICAL, layout_map_[std_fields]};//  find idx
    block_.instructions.push_back(a_ref);

    reg_cnt_++;

    for (auto p : expr.dict) {
        p.second->accept(*((Visitor*)this));

        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};

        int idx;
        if (!str_const_.count(p.first)) {
            program_->immediates.push_back(runtime::to_value(program_->ctx_ptr, p.first));
            str_const_[p.first] = imm_cnt_++;
        }
        idx = str_const_[p.first];

        IR::Instruction store_field;
        store_field.op = IR::Operation::REC_STORE_NAME;
        store_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
        store_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
        store_field.args[2] = opr_; 
        block_.instructions.push_back(store_field);
    }

    is_opr_ = false;
    ret_reg_ = rec_reg;
}

void Compiler::visit(AST::IntegerConstant& expr) {
    is_opr_ = true;
    int val = expr.getVal();
    int idx;
    if (!int_const_.count(val)) {
        program_->immediates.push_back(runtime::to_value(val));  // val
        int_const_[val] = imm_cnt_++;
    }
    idx = int_const_[val];

    opr_ = {IR::Operand::OpType::IMMEDIATE, idx};
}

void Compiler::visit(AST::StringConstant& expr) {
    string s = expr.getVal();
    is_opr_ = false;
    if (s[0] == '"') {
        is_opr_ = true;
        string str = s.substr(1, s.size() - 2);

        int idx;
        if (!str_const_.count(str)) {
            program_->immediates.push_back(runtime::to_value(program_->ctx_ptr, str));  // str
            str_const_[str] = imm_cnt_++;
        }
        idx = str_const_[str];

        opr_ = {IR::Operand::OpType::IMMEDIATE, idx};
    } else {
        if (global_scope_ && !globals_.count(s)) {
            if (!names_.count(s))
                names_[s] = names_cnt_++;
            globals_.insert(s);
        }

        if (globals_.count(s)) {
            block_.instructions.push_back({IR::Operation::LOAD_GLOBAL,
                                           {IR::Operand::OpType::VIRT_REG, reg_cnt_},
                                           {IR::Operand::OpType::LOGICAL, names_[s]}});
            ret_reg_ = reg_cnt_++;
            return;
        } else if (local_reference_vars_.count(s)) {
            block_.instructions.push_back({IR::Operation::REF_LOAD,
                                           {IR::Operand::OpType::VIRT_REG, reg_cnt_},
                                           {IR::Operand::OpType::VIRT_REG, local_vars_[s]}});
            ret_reg_ = reg_cnt_++;
        } else if (free_vars_.count(s)) {
            block_.instructions.push_back({IR::Operation::REF_LOAD,
                                           {IR::Operand::OpType::VIRT_REG, reg_cnt_},
                                           {IR::Operand::OpType::VIRT_REG, free_vars_[s]}});
            ret_reg_ = reg_cnt_++;
        } else {
            ret_reg_ = local_vars_[s];
        }  // should default be globals?
    }
}

void Compiler::visit(AST::BoolConstant& expr) {
    is_opr_ = true;
    int idx = expr.getVal() ? 1 : 2;
    opr_ = {IR::Operand::OpType::IMMEDIATE, idx};
}

void Compiler::visit(AST::NoneConstant& expr) {
    is_opr_ = true;
    opr_ = {IR::Operand::OpType::IMMEDIATE, 0};
}
