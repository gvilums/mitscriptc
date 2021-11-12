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
    
    std::map<std::string, int> local_vars_;
    std::map<std::string, int> local_reference_vars_;
    std::map<std::string, int> free_vars_;
    
    std::set<std::string> globals_; 
    std::set<std::string> ref_;
    
    int imm_cnt_;
    std::map<string, int> str_const_;
    std::map<int, int> int_const_;
    
   public:
    Compiler() {
        program_ = new IR::Program; 
        
		IR::Function print_ = {{{{}, {{IR::Operation::PRINT}}, {}, {}, false, 0}}, 0, {}};
		IR::Function input_ = {{{{}, {{IR::Operation::INPUT}}, {}, {}, false, 0}}, 0, {}};
		IR::Function intcast_ = {{{{}, {{IR::Operation::INTCAST}}, {}, {}, false, 0}}, 0, {}};
		
		program_->functions.push_back(print_);
		program_->functions.push_back(input_);
		program_->functions.push_back(intcast_);
		fun_ = new IR::Function(); // this is the global scope
		block_ = new IR::BasicBlock();
		
		// program_->immediates.push_back(); // NONE
		// program_->immediates.push_back(); // TRUE
		// program_->immediates.push_back(); // FALSE
		// program_->immediates.push_back(); // 0
	
		reg_cnt_ = 0;
		imm_cnt_ = 4;
		int_const_[0] = 3;
        
        global_scope_ = true;
        is_opr_ = false;
    }

    IR::Program* get_program() {
        return program_;
    }

    void visit(AST::Program& expr) {
        for (auto c : expr.children)
            c->accept(*((Visitor*)this));
  		
        fun_->blocks.push_back(*block_);
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
        block_->instructions.push_back({IR::Operation::RETURN, IR::Operand(), {IR::Operand::OpType::VIRT_REG, ret_reg_}});
    }

    void visit(AST::Assignment& expr) {
        /*if (expr.Lhs->isStringConstant()) {
            string s = ((AST::StringConstant*)expr.Lhs)->getVal();

            if (global_scope_ && !count(rfun_->names_.begin(), rfun_->names_.end(), s)) {
                rfun_->names_.push_back(s);
                globals_.insert(s);
            }

            if (globals_.count(s)) {
                int idx = find(rfun_->names_.begin(), rfun_->names_.end(), s) - rfun_->names_.begin();
                expr.Expr->accept(*((Visitor*)this));
                rfun_->instructions.push_back(Instruction(Operation::StoreGlobal, idx));
            } else if (count(rfun_->local_reference_vars_.begin(), rfun_->local_reference_vars_.end(), s)) {
                int idx = find(
                              rfun_->local_reference_vars_.begin(),
                              rfun_->local_reference_vars_.end(), s) -
                          rfun_->local_reference_vars_.begin();

                rfun_->instructions.push_back(Instruction(Operation::PushReference, idx));
                expr.Expr->accept(*((Visitor*)this));
                rfun_->instructions.push_back(Instruction(Operation::StoreReference, std::nullopt));
            } else {
                int idx = find(rfun_->local_vars_.begin(), rfun_->local_vars_.end(), s) - rfun_->local_vars_.begin();
                expr.Expr->accept(*((Visitor*)this));
                rfun_->instructions.push_back(Instruction(Operation::StoreLocal, idx));
            }
        } else if (expr.Lhs->isFieldDereference()) {
            AST::FieldDereference* exp = ((AST::FieldDereference*)expr.Lhs);

            exp->baseexpr->accept(*((Visitor*)this));
            expr.Expr->accept(*((Visitor*)this));
            rfun_->instructions.push_back(Instruction(Operation::FieldStore, rfun_->names_.size()));
            rfun_->names_.push_back(exp->field);
        } else if (expr.Lhs->isIndexExpression()) {
            AST::IndexExpression* exp = ((AST::IndexExpression*)expr.Lhs);

            exp->baseexpr->accept(*((Visitor*)this));
            exp->index->accept(*((Visitor*)this));
            expr.Expr->accept(*((Visitor*)this));
            rfun_->instructions.push_back(Instruction(Operation::IndexStore, std::nullopt));
        }*/
    }

    void visit(AST::IfStatement& expr) {
        /*expr.Expr->accept(*((Visitor*)this)); 
        expr.children[0]->accept(*((Visitor*)this));
        if (expr.children.size() > 1) {
        }*/
    }

    void visit(AST::WhileLoop& expr) { // could contain empty blocks
    	/*fun_->blocks.push_back(*block_);
    	size_t header_idx = fun_->blocks.size();
    	fun_->blocks.back().successors.push_back(header_idx);
    	delete block_;
    	
    	block_ = new IR::BasicBlock();
    	block_->predecessors.push_back(header_idx - 1);
    	block_->is_loop_header = true;
    	expr.Expr->accept(*((Visitor*)this));
    	fun_->blocks.push_back(*block_);
    	delete block_;
    	
        expr.children[0]->accept(*((Visitor*)this));
        
    	fun_->blocks[header_idx].final_loop_block = fun_->blocks.size();
    	fun_->
        fun_->blocks.push_back(*block_);  
    	delete block_;
    	
    	block_ = new IR::BasicBlock();*/
    }

    void visit(AST::FunctionDeclaration& expr) {
    	IR::Function* tfun = fun_;
    	IR:: BasicBlock* tblock = block_;
    	bool tscope = global_scope_;
    	std::set<string> tglobals = globals_;
		std::set<string> tref = ref_;
		int treg_cnt = reg_cnt_;
		
		std::map<std::string, int> tlocal_vars = local_vars_;
    	std::map<std::string, int> tlocal_reference_vars = local_reference_vars_;
    	std::map<std::string, int> tfree_vars = free_vars_;
    	
    	local_vars_ = std::map<std::string, int>();
     	local_reference_vars_ = std::map<std::string, int>();
     	free_vars_ = std::map<std::string, int>();
     	
		fun_ = new IR::Function;
		block_ = new IR::BasicBlock;
		reg_cnt_ = 0;
		global_scope_ = false;
		fun_->parameter_count = expr.arguments.size();
		
		for (auto s : tlocal_reference_vars)
        	ref_.insert(s.first);
        
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
            local_vars_[s] = 1;
            if (count(nb_var.begin(), nb_var.end(), s))
                local_reference_vars_[s] = - 1; 
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
        
        for (auto s : ass_var) {
            if (count(glob_var.begin(), glob_var.end(), s))
                continue;
            local_vars_[s] = - 1;
            if (count(nb_var.begin(), nb_var.end(), s))
                local_reference_vars_[s] = - 1; 
            if (globals_.count(s))
                globals_.erase(s);
            if (ref_.count(s))
            	ref_.erase(s);
        }
        
        for (auto s : free_var) 
            if (ref_.count(s))
                free_vars_[s] = - 1;
        
        tblock->instructions.push_back({IR::Operation::ALLOC_CLOSURE, {IR::Operand::OpType::VIRT_REG, ret_reg_}, {IR::Operand::OpType::LOGICAL, free_vars_.size()}});
        
        ret_reg_ = reg_cnt_;
        reg_cnt_++;
        
        int idx = 0;
        for (auto s : free_vars_){
        	free_vars_[s.first] = s.second;
        	
        	if (tlocal_reference_vars.count(s.first)){
        		tblock->instructions.push_back({IR::Operation::ALLOC_CLOSURE, {IR::Operand::OpType::VIRT_REG, ret_reg_}, {IR::Operand::OpType::LOGICAL, free_vars_.size()}});
        	}
        	else if (tfree_vars.count(s.first)) {
        		
        	}
        }
           	
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
	
        /*
        
        tfun->instructionsALLOC_CLOSURE
        
        ALLOC_CLOSURE
        tfun->instructions.push_back(Instruction(Operation::LoadFunc, tfun->functions_.size() - 1));
        // std::reverse(rfun_->free_vars_.begin(), rfun_->free_vars_.end());
        for (auto s : rfun_->free_vars_) {
            auto it = find(tfun->local_reference_vars_.begin(), tfun->local_reference_vars_.end(), s);
            if (it != tfun->local_reference_vars_.end()) {
                tfun->instructions.push_back(Instruction(
                    Operation::PushReference, int(it - tfun->local_reference_vars_.begin())));
                continue;
            }

            it = find(tfun->free_vars_.begin(), tfun->free_vars_.end(), s);
            tfun->instructions.push_back(Instruction(
                Operation::PushReference,
                int(it - tfun->free_vars_.begin()) + tfun->local_reference_vars_.size()));
        }
        tfun->instructions.push_back(Instruction(Operation::AllocClosure, rfun_->free_vars_.size()));

        expr.block->accept(*((Visitor*)this));

        rfun_->instructions.push_back(Instruction(Operation::LoadConst, rfun_->constants_.size()));
        rfun_->constants_.push_back(Constant{none_});
        rfun_->instructions.push_back(Instruction(Operation::Return, std::nullopt));*/
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
        ret_reg_ = reg_cnt_;
        reg_cnt_++;
    }

    void visit(AST::Call& expr) {
        /*expr.expr->accept(*((Visitor*)this));
        for (auto c : expr.arguments)
            c->accept(*((Visitor*)this));
        rfun_->instructions.push_back(Instruction(Operation::Call, expr.arguments.size()));*/
    }

    void visit(AST::FieldDereference& expr) {
        /*expr.baseexpr->accept(*((Visitor*)this));
        rfun_->instructions.push_back(Instruction(Operation::FieldLoad, rfun_->names_.size()));
        rfun_->names_.push_back(expr.field);*/
    }

    void visit(AST::IndexExpression& expr) {
        /*expr.baseexpr->accept(*((Visitor*)this));
        expr.index->accept(*((Visitor*)this));
        rfun_->instructions.push_back(Instruction(Operation::IndexLoad, std::nullopt));*/
    }

    void visit(AST::Record& expr) {
        /*rfun_->instructions.push_back(Instruction(Operation::AllocRecord, std::nullopt));

        for (auto p : expr.dict) {
            rfun_->instructions.push_back(Instruction(Operation::Dup, std::nullopt));
            p.second->accept(*((Visitor*)this));

            rfun_->instructions.push_back(Instruction(Operation::FieldStore, rfun_->names_.size()));
            rfun_->names_.push_back(p.first);
        }*/
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
        string val = expr.getVal();
        if (val[0] == '"') {
        	is_opr_ = true;
        	string str = val.substr(1, val.size() - 2);
        	
        	size_t idx;
			if (!str_const_.count(val)) {
				// program_->immediates.push_back(); // str
				str_const_[val] = imm_cnt_++;
			}
			idx = str_const_[val];
    	
    		opr_ = {IR::Operand::OpType::IMMEDIATE, idx};   
        } /*else {
        	
        
            if (global_scope_ && !count(rfun_->names_.begin(), rfun_->names_.end(), val)) {
                rfun_->names_.push_back(val);
                globals_.insert(val);
            }

            if (globals_.count(val)) {
                auto it = find(rfun_->names_.begin(), rfun_->names_.end(), val);
                rfun_->instructions.push_back(Instruction(Operation::LoadGlobal, int(it - rfun_->names_.begin())));
                return;
            }

            auto it = find(rfun_->free_vars_.begin(), rfun_->free_vars_.end(), val);
            if (it != rfun_->free_vars_.end()) {
                rfun_->instructions.push_back(Instruction(
                    Operation::PushReference,
                    int(it - rfun_->free_vars_.begin()) + rfun_->local_reference_vars_.size()));
                rfun_->instructions.push_back(Instruction(Operation::LoadReference, std::nullopt));
                return;
            }

            it = find(rfun_->local_reference_vars_.begin(), rfun_->local_reference_vars_.end(), val);
            if (it != rfun_->local_reference_vars_.end()) {
                rfun_->instructions.push_back(Instruction(
                    Operation::PushReference, int(it - rfun_->local_reference_vars_.begin())));
                rfun_->instructions.push_back(Instruction(Operation::LoadReference, std::nullopt));
                return;
            }

            it = find(rfun_->local_vars_.begin(), rfun_->local_vars_.end(), val);
            if (it != rfun_->local_vars_.end()) {
                rfun_->instructions.push_back(Instruction(Operation::LoadLocal, int(it - rfun_->local_vars_.begin())));
                return;
            }

            rfun_->instructions.push_back(Instruction(Operation::LoadGlobal, rfun_->names_.size()));
            rfun_->names_.push_back(val);
            globals_.insert(val);
        }*/
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
