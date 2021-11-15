#pragma once

#include <algorithm>
#include <set>
#include "AST.h"
#include "Assigns.h"
#include "FreeVariables.h"
#include "ir.h"

using namespace std;

class Compiler : public Visitor {
   private:
    bool global_scope_;
  
    IR::Program* program_;
    IR::Function* fun_;
    IR::BasicBlock* block_;
    IR::Operand opr_;
    bool is_opr_;
    size_t reg_cnt_;
    size_t ret_reg_;
    
    std::map<std::string, size_t> local_vars_;
    std::set<std::string> local_reference_vars_;
    std::map<std::string, size_t> free_vars_;
    std::map<std::string, size_t> names_;
    
    std::set<std::string> globals_; 
    std::set<std::string> ref_;
    
    int imm_cnt_;
    size_t names_cnt_;
    std::map<string, int> str_const_;
    std::map<int, int> int_const_;
    
   public:
    Compiler() {
        program_ = new IR::Program; 
        
		IR::Function print_ = {{{{}, {{IR::Operation::LOAD_ARG, {IR::Operand::OpType::VIRT_REG, 0}, {IR::Operand::OpType::LOGICAL, 0}}, {IR::Operation::PRINT, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 0}}, {IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::IMMEDIATE, 0}}}, {}, {}, false, 0}}, 0, 0, {}};
		IR::Function input_ = {{{{}, {{IR::Operation::INPUT, {IR::Operand::OpType::VIRT_REG, 0}},{IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 0}}}, {}, {}, false, 0}}, 0, 0, {}};
		IR::Function intcast_ = {{{{}, {{IR::Operation::LOAD_ARG, {IR::Operand::OpType::VIRT_REG, 0}, {IR::Operand::OpType::LOGICAL, 0}}, {IR::Operation::INTCAST, {IR::Operand::OpType::VIRT_REG, 1}, {IR::Operand::OpType::VIRT_REG, 0}}, {IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::VIRT_REG, 1}}}, {}, {}, false, 0}}, 0, 0, {}};
		
		program_->functions.push_back(print_);
		program_->functions.push_back(input_);
		program_->functions.push_back(intcast_);
		fun_ = new IR::Function(); // this is the global scope
		block_ = new IR::BasicBlock();
		
		names_["print"] = 0;
		names_["input"] = 1;
		names_["intcast"] = 2;
		globals_.insert("print");
		globals_.insert("input");
		globals_.insert("intcast");
		
		// program_->immediates.push_back(); // NONE
		// program_->immediates.push_back(); // TRUE
		// program_->immediates.push_back(); // FALSE
		// program_->immediates.push_back(); // 0
	
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
   		
        block_->instructions.push_back(a_closure);
        block_->instructions.push_back(store_glob);
       
   		a_closure.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
   		a_closure.args[0] = {IR::Operand::OpType::LOGICAL, 1};
   		a_closure.args[1] = {IR::Operand::OpType::LOGICAL, 0};
   		store_glob.args[0] = {IR::Operand::OpType::LOGICAL, 1};
   		store_glob.args[1] = {IR::Operand::OpType::VIRT_REG, reg_cnt_++};
   		
        block_->instructions.push_back(a_closure);
        block_->instructions.push_back(store_glob);
        
        a_closure.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
   		a_closure.args[0] = {IR::Operand::OpType::LOGICAL, 2};
   		a_closure.args[1] = {IR::Operand::OpType::LOGICAL, 1};
   		store_glob.args[0] = {IR::Operand::OpType::LOGICAL, 2};
   		store_glob.args[1] = {IR::Operand::OpType::VIRT_REG, reg_cnt_++};
   		
        block_->instructions.push_back(a_closure);
        block_->instructions.push_back(store_glob);
    }

    IR::Program* get_program() {
        return program_;
    }

    void visit(AST::Program& expr) {
        for (auto c : expr.children)
            c->accept(*((Visitor*)this));
  		
        fun_->blocks.push_back(*block_);
        fun_->virt_reg_count = reg_cnt_;
    	program_->functions.push_back(*fun_);
    	delete block_;
    	delete fun_;
    	program_->num_globals = globals_.size();
    }

    void visit(AST::Block& expr) {
        for (auto c : expr.children)
            c->accept(*((Visitor*)this));
    }

    void visit(AST::Global& expr) {}

    void visit(AST::Return& expr) { 
        expr.Expr->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};  
        block_->instructions.push_back({IR::Operation::RETURN, IR::Operand(), opr_});
    }

    void visit(AST::Assignment& expr) {
        if (expr.Lhs->isStringConstant()) {
            string s = ((AST::StringConstant*)expr.Lhs)->getVal();
			expr.Expr->accept(*((Visitor*)this));
					
            if (global_scope_ && !globals_.count(s)) {
                names_[s] = names_cnt_++;
                globals_.insert(s);
            }
            
            if (globals_.count(s)) {
            	if (!is_opr_)
            		opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
            	
            	IR::Instruction store_glob;
   				store_glob.op = IR::Operation::STORE_GLOBAL;
   				store_glob.args[0] = {IR::Operand::OpType::LOGICAL, names_[s]};
   				store_glob.args[1] = opr_;
            	
            	block_->instructions.push_back(store_glob);
            }
			else if (local_reference_vars_.count(s)) {
				if (!is_opr_)
            		opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
				IR::Instruction s_ref;
   				s_ref.op = IR::Operation::REF_STORE;
   				s_ref.args[0] = {IR::Operand::OpType::VIRT_REG, local_vars_[s]};
   				s_ref.args[1] = opr_;
				block_->instructions.push_back(s_ref);
			}
			else { // check for error ? 
				local_vars_[s] = reg_cnt_++;
				if (is_opr_) {
					block_->instructions.push_back({IR::Operation::MOV, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr_});
					local_vars_[s] = reg_cnt_++;
				}
				else 
					local_vars_[s] = ret_reg_;
			}
		}
        else if (expr.Lhs->isFieldDereference()) {
            AST::FieldDereference* exp = ((AST::FieldDereference*)expr.Lhs);
            exp->baseexpr->accept(*((Visitor*)this));
            size_t rec_reg = ret_reg_;
            
            expr.Expr->accept(*((Visitor*)this));
            if (!is_opr_)
            	opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
           
            size_t idx;
			if (!str_const_.count(exp->field)) {
				// program_->immediates.push_back(); // expr->field
				str_const_[exp->field] = imm_cnt_++;
			}
			idx = str_const_[exp->field];
			
			IR::Instruction store_field;
	   		store_field.op = IR::Operation::REC_STORE_NAME;
	   		store_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
	   		store_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
	   		store_field.args[2] = opr_;
		    block_->instructions.push_back(store_field);
		    
        } else if (expr.Lhs->isIndexExpression()) {
            AST::IndexExpression* exp = ((AST::IndexExpression*)expr.Lhs);
            exp->baseexpr->accept(*((Visitor*)this));
            size_t rec_reg = ret_reg_;
            
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
		    block_->instructions.push_back(store_index);
        }
    }

    void visit(AST::IfStatement& expr) {
    	IR::BasicBlock* block0 = new IR::BasicBlock;
    	IR::BasicBlock* block1 = new IR::BasicBlock;
    	IR::BasicBlock* block2 = new IR::BasicBlock;
    	IR::BasicBlock* block3 = new IR::BasicBlock;
    	
    	size_t last_idx = fun_->blocks.size(), block1_idx, block2_idx, next_idx;
    	block2_idx = last_idx;
    	fun_->blocks.push_back(*block_);
    	delete block_;
    	
    	block_ = block0;
    	fun_->blocks[last_idx].successors.push_back(last_idx + 1);
    	block0->predecessors.push_back(last_idx);
        expr.Expr->accept(*((Visitor*)this));
        fun_->blocks.push_back(*block_);
        delete block_;
        
        std::map<std::string, size_t> tlocal_vars1 = local_vars_, tlocal_vars2 = local_vars_;
        block_ = block1;
        expr.children[0]->accept(*((Visitor*)this));
        block1_idx = fun_->blocks.size();
        block1->predecessors.push_back(last_idx + 1);
        block3->predecessors.push_back(block1_idx);
        fun_->blocks[last_idx + 1].successors.push_back(block1_idx);
    	fun_->blocks.push_back(*block_);
    	tlocal_vars1 = local_vars_;
    	delete block_;
		
        if (expr.children.size() > 1) {
        	local_vars_ = tlocal_vars2;
        	
        	block_ = block2;
        	expr.children[1]->accept(*((Visitor*)this));
        	block2_idx = fun_->blocks.size();
        	block2->predecessors.push_back(last_idx + 1);
        	block3->predecessors.push_back(block2_idx);
        	fun_->blocks[last_idx + 1].successors.push_back(block2_idx);
    		fun_->blocks.push_back(*block_);
    		tlocal_vars2 = local_vars_;
    		delete block_;	
        }
        
        block_ = block3;
        next_idx = fun_->blocks.size();
        fun_->blocks[block1_idx].successors.push_back(next_idx);
        if (expr.children.size() > 1)
        	fun_->blocks[block2_idx].successors.push_back(next_idx);
        
        for (auto var : tlocal_vars1) {
        	if (tlocal_vars2[var.first] != var.second) {
        		IR::PhiNode pn;
        		pn.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};        		
        		pn.args.push_back({block1_idx, {IR::Operand::OpType::VIRT_REG, var.second}});
        		pn.args.push_back({block2_idx, {IR::Operand::OpType::VIRT_REG, tlocal_vars2[var.first]}});
        	
        		block_->phi_nodes.push_back(pn);
        		local_vars_[var.first] = reg_cnt_;
        		reg_cnt_++;
        	}
        } 
    }

    void visit(AST::WhileLoop& expr) { // could contain empty blocks
    	IR::BasicBlock* block0 = new IR::BasicBlock;
    	IR::BasicBlock* block1 = new IR::BasicBlock;
    	IR::BasicBlock* block2 = new IR::BasicBlock;
    	
    	size_t last_idx = fun_->blocks.size(), block1_idx, block2_idx;
    	fun_->blocks.push_back(*block_);
    	delete block_;
    	
    	block_ = block0;
    	fun_->blocks[last_idx].successors.push_back(last_idx + 1);
    	block0->predecessors.push_back(last_idx);
        expr.Expr->accept(*((Visitor*)this));
        fun_->blocks.push_back(*block_);
        delete block_;
        
        std::map<std::string, size_t> tlocal_vars = local_vars_;
        block_ = block1;
        expr.children[0]->accept(*((Visitor*)this));
        block1_idx = fun_->blocks.size();
        block1->predecessors.push_back(last_idx + 1);
        fun_->blocks[last_idx + 1].successors.push_back(block1_idx);
        block1->successors.push_back(last_idx + 1);
        fun_->blocks[last_idx + 1].predecessors.push_back(block1_idx);
    	fun_->blocks.push_back(*block_);
    	delete block_;
    	
    	for (auto var : tlocal_vars) {
    		if (local_vars_[var.first] != var.second) {
    			IR::PhiNode pn;
        		pn.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};        		
        		pn.args.push_back({last_idx, {IR::Operand::OpType::VIRT_REG, var.second}});
        		pn.args.push_back({block1_idx, {IR::Operand::OpType::VIRT_REG, local_vars_[var.first]}});
        		
        		fun_->blocks[last_idx + 1].phi_nodes.push_back(pn);
        		local_vars_[var.first] = reg_cnt_;
        		reg_cnt_++;
    		}
    	}
    	
    	block_ = block2;
        block2_idx = fun_->blocks.size();
    	fun_->blocks[last_idx + 1].successors.push_back(block2_idx);
    	block2->predecessors.push_back(last_idx + 1);
    }

    void visit(AST::FunctionDeclaration& expr) {
    	IR::Function* tfun = fun_;
    	IR:: BasicBlock* tblock = block_;
    	bool tscope = global_scope_;
    	std::set<string> tglobals = globals_;
		std::set<string> tref = ref_;
		size_t treg_cnt = reg_cnt_;
		size_t tret_reg = ret_reg_;
		
		std::map<std::string, size_t> tlocal_vars = local_vars_;
    	std::set<std::string> tlocal_reference_vars = local_reference_vars_;
    	std::map<std::string, size_t> tfree_vars = free_vars_;
    	
    	local_vars_ = std::map<std::string, size_t>();
     	local_reference_vars_ = std::set<std::string>();
     	free_vars_ = std::map<std::string, size_t>();
     	
		fun_ = new IR::Function;
		block_ = new IR::BasicBlock;
		reg_cnt_ = 0;
		global_scope_ = false;
		fun_->parameter_count = expr.arguments.size();
		
		for (auto s : tlocal_reference_vars)
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
        
        for (auto s : expr.arguments) {
            local_vars_[s] = reg_cnt_++;
            if (count(nb_var.begin(), nb_var.end(), s))
                local_reference_vars_.insert(s); 
            if (globals_.count(s))
                globals_.erase(s);
            if (ref_.count(s))
            	ref_.erase(s); 
        }

        for (auto s : glob_var) {
            globals_.insert(s);
            if (ref_.count(s))
            	ref_.erase(s);
        }
        
        std::vector<size_t> new_ass;
        for (auto s : ass_var) {
            if (count(glob_var.begin(), glob_var.end(), s))
                continue;
            new_ass.push_back(reg_cnt_);
            local_vars_[s] = reg_cnt_++;
            if (count(nb_var.begin(), nb_var.end(), s))
                local_reference_vars_.insert(s); 
            if (globals_.count(s))
                globals_.erase(s);
            if (ref_.count(s))
            	ref_.erase(s);
        }
        
        for (auto s : free_var) 
            if (ref_.count(s))
                free_vars_[s] = - 1;
        
        IR::Operand fun_reg = {IR::Operand::OpType::VIRT_REG, treg_cnt};
        size_t b_idx = tblock->instructions.size();
        tblock->instructions.push_back({});
        
        tret_reg = treg_cnt;
        treg_cnt++;
        
        size_t args_idx = 0;
        for (auto s : expr.arguments)
        	block_->instructions.push_back({IR::Operation::LOAD_ARG, {IR::Operand::OpType::VIRT_REG, local_vars_[s]}, {IR::Operand::OpType::LOGICAL, args_idx++}});
         
        size_t idx = 0;
        vector<IR::Instruction> instr;
        for (auto s : free_vars_){
        	block_->instructions.push_back({IR::Operation::LOAD_FREE_REF, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, {IR::Operand::OpType::LOGICAL, idx}});
        	free_vars_[s.first] = reg_cnt_++; // could be done later
        	
        	if (tlocal_reference_vars.count(s.first)){
        		instr.push_back({IR::Operation::SET_CAPTURE, {IR::Operand::OpType::LOGICAL, idx}, fun_reg, {IR::Operand::OpType::VIRT_REG, tlocal_vars[s.first]}});
        	}
        	else if (tfree_vars.count(s.first)) {	
        		tblock->instructions.push_back({IR::Operation::LOAD_FREE_REF, {IR::Operand::OpType::VIRT_REG, treg_cnt}, {IR::Operand::OpType::VIRT_REG, tfree_vars[s.first]}});
        		instr.push_back({IR::Operation::SET_CAPTURE, {IR::Operand::OpType::LOGICAL, idx}, fun_reg, {IR::Operand::OpType::VIRT_REG, reg_cnt_}});
        		treg_cnt++; 		
        	}
        	idx++;
        }
        
        for (auto ins : instr)
        	tblock->instructions.push_back(ins);
        
       	for (auto reg : new_ass)
       		block_->instructions.push_back({IR::Operation::MOV, {IR::Operand::OpType::VIRT_REG, reg}, {IR::Operand::OpType::IMMEDIATE, 0}});
        
        
        expr.block->accept(*((Visitor*)this));
   		
   		is_opr_ = false;
   		
   		IR::Instruction a_closure;
   		a_closure.op = IR::Operation::ALLOC_CLOSURE;
   		a_closure.out = fun_reg;
   		a_closure.args[0] = {IR::Operand::OpType::LOGICAL, program_->functions.size()};
   		a_closure.args[1] = {IR::Operand::OpType::LOGICAL, expr.arguments.size()};
   		a_closure.args[2] = {IR::Operand::OpType::LOGICAL, free_vars_.size()};
   		tblock->instructions[b_idx] = a_closure;
   		
        block_->instructions.push_back({IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::IMMEDIATE, 0}});   
        fun_->virt_reg_count = reg_cnt_;
        fun_->blocks.push_back(*block_);
		program_->functions.push_back(*fun_);
		delete fun_;
		delete block_;
	
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

    void visit(AST::BinaryExpression& expr) {
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
            block_->instructions.push_back({opr_t, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr1, opr2});
        	block_->instructions.push_back({IR::Operation::NOT, {IR::Operand::OpType::VIRT_REG, reg_cnt_ + 1}, {IR::Operand::OpType::VIRT_REG, reg_cnt_}});
        	ret_reg_ = reg_cnt_ + 1;
        	reg_cnt_ += 2;
            return;
        }

        IR::Operation op;
        if (expr.op == "+")
            op = IR::Operation::ADD;
        else if (expr.op == "-")
            op = IR::Operation::SUB;
        else if (expr.op == "/")
            op = IR::Operation::DIV;
        else if (expr.op == "*")
            op = IR::Operation::MUL;
        else if (expr.op == ">")
            op = IR::Operation::GT;
        else if (expr.op == ">=")
            op = IR::Operation::GEQ;
        else if (expr.op == "==")
            op = IR::Operation::EQ;
        else if (expr.op == "&")
            op = IR::Operation::AND;
        else if (expr.op == "|")
            op = IR::Operation::OR;

        block_->instructions.push_back({op, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr1, opr2});
        is_opr_ = false;
        ret_reg_ = reg_cnt_;
        reg_cnt_++;
    }

    void visit(AST::UnaryExpression& expr) {
        IR::Operand opr;
        expr.children[0]->accept(*((Visitor*)this));
        if (is_opr_)
        	opr = opr_;
        else 
        	opr = {IR::Operand::OpType::VIRT_REG, ret_reg_};

        if (expr.op == "!") {
        	block_->instructions.push_back({IR::Operation::NOT, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr});
        }
        else {
        	IR::Operand zero = {IR::Operand::OpType::IMMEDIATE, 3};
        	block_->instructions.push_back({IR::Operation::SUB, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, zero, opr});
        }
        is_opr_ = false;
        ret_reg_ = reg_cnt_;
        reg_cnt_++;
    }

    void visit(AST::Call& expr) {
        expr.expr->accept(*((Visitor*)this));
        size_t fun_reg = ret_reg_;
        
        size_t arg_cnt = 0;
        for (auto c : expr.arguments) {
            c->accept(*((Visitor*)this));
            if (!is_opr_)
            	opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
            
            IR::Instruction set_arg;
   			set_arg.op = IR::Operation::SET_ARG;
   			set_arg.args[0] = {IR::Operand::OpType::LOGICAL, arg_cnt++};
   			set_arg.args[1] = opr_;
            
            block_->instructions.push_back(set_arg);
        }
        
        IR::Instruction call;
   		call.op = IR::Operation::CALL;
   		call.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
   		call.args[0] = {IR::Operand::OpType::LOGICAL, arg_cnt};
   		call.args[1] = {IR::Operand::OpType::VIRT_REG, fun_reg};
   		
        block_->instructions.push_back(call);
        is_opr_ = false;
        ret_reg_ = reg_cnt_;
        reg_cnt_++;
    }

    void visit(AST::FieldDereference& expr) {
    	expr.baseexpr->accept(*((Visitor*)this));
    	size_t rec_reg = ret_reg_;
    	
    	size_t idx;
		if (!str_const_.count(expr.field)) {
			// program_->immediates.push_back(); // expr.field
			str_const_[expr.field] = imm_cnt_++;
		}
		idx = str_const_[expr.field];
    	
    	IR::Instruction load_field;
   		load_field.op = IR::Operation::REC_LOAD_NAME;
   		load_field.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
   		load_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
   		load_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
        block_->instructions.push_back(load_field);
        reg_cnt_++;
        is_opr_ = false;
    }

    void visit(AST::IndexExpression& expr) {
        expr.baseexpr->accept(*((Visitor*)this));
        size_t rec_reg = ret_reg_;
        
        expr.index->accept(*((Visitor*)this));
        if (!is_opr_)
            opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
         
        IR::Instruction load_idx;
   		load_idx.op = IR::Operation::REC_LOAD_INDX;
   		load_idx.out = {IR::Operand::OpType::VIRT_REG, reg_cnt_};
   		load_idx.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
   		load_idx.args[1] = opr_;
        block_->instructions.push_back(load_idx);
        reg_cnt_++;
        is_opr_ = false;
    }

    void visit(AST::Record& expr) {
    	size_t rec_reg = reg_cnt_;
    	block_->instructions.push_back({IR::Operation::ALLOC_REC, {IR::Operand::OpType::VIRT_REG, rec_reg}});
    	reg_cnt_++;
    	
        for (auto p : expr.dict) {
            p.second->accept(*((Visitor*)this));
            
            if (!is_opr_)
            	opr_ = {IR::Operand::OpType::VIRT_REG, ret_reg_};
            
            size_t idx;
			if (!str_const_.count(p.first)) {
				// program_->immediates.push_back(); // expr.field
				str_const_[p.first] = imm_cnt_++;
			}
			idx = str_const_[p.first];
			
			IR::Instruction store_field;
	   		store_field.op = IR::Operation::REC_STORE_NAME;
	   		store_field.args[0] = {IR::Operand::OpType::VIRT_REG, rec_reg};
	   		store_field.args[1] = {IR::Operand::OpType::IMMEDIATE, idx};
	   		store_field.args[2] = opr_;
		    block_->instructions.push_back(store_field);
        }
    }

    void visit(AST::IntegerConstant& expr) {
    	is_opr_ = true;
    	int val = expr.getVal();
    	size_t idx;
    	if (!int_const_.count(val)) {
    		// program_->immediates.push_back(); // val
    		int_const_[val] = imm_cnt_++;
    	}
    	idx = int_const_[val];
    	
    	opr_ = {IR::Operand::OpType::IMMEDIATE, idx};
    }

    void visit(AST::StringConstant& expr) {
        string s = expr.getVal();
        is_opr_ = false;
        if (s[0] == '"') {
        	is_opr_ = true;
        	string str = s.substr(1, s.size() - 2);
        	
        	size_t idx;
			if (!str_const_.count(str)) {
				// program_->immediates.push_back(); // str
				str_const_[str] = imm_cnt_++;
			}
			idx = str_const_[str];
    	
    		opr_ = {IR::Operand::OpType::IMMEDIATE, idx};   
        } else {
        	if (global_scope_ && !globals_.count(s)) {
                names_[s] = names_cnt_++;
                globals_.insert(s);
            }
        	
            if (globals_.count(s)) {
            	block_->instructions.push_back({IR::Operation::LOAD_GLOBAL, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, {IR::Operand::OpType::LOGICAL, names_[s]}});
            	ret_reg_ = reg_cnt_++;
                return;
            }
            else if (local_reference_vars_.count(s)) {
            	block_->instructions.push_back({IR::Operation::REF_LOAD, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, {IR::Operand::OpType::VIRT_REG, local_vars_[s]}});
            	ret_reg_ = reg_cnt_++;
            }
            else if (free_vars_.count(s)) {
            	block_->instructions.push_back({IR::Operation::REF_LOAD, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, {IR::Operand::OpType::VIRT_REG, free_vars_[s]}});
            	ret_reg_ = reg_cnt_++;
            }
            else {
            	is_opr_ = true;
            	opr_ = {IR::Operand::OpType::VIRT_REG, local_vars_[s]};
            } // should default be globals? 
        }
    }

    void visit(AST::BoolConstant& expr) {
    	is_opr_ = true;
    	size_t idx = expr.getVal() ? 1 : 2;
    	opr_ = {IR::Operand::OpType::IMMEDIATE, idx};
    }

    void visit(AST::NoneConstant& expr) {
        is_opr_ = true;
    	opr_ = {IR::Operand::OpType::IMMEDIATE, 0};
    }
};
