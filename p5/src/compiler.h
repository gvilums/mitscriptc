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
    int reg_cnt_;
    int ret_reg_;
    
    std::map<std::string, int> globals_; 
    std::set<std::string> ref_;

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
	
		reg_cnt_ = 0;
        
        global_scope_ = true;
        is_opr_ = false;
    }

    IR::Program* get_program() {
    	program_->functions.push_back(*fun_);
    	delete fun_;
    	// program_->num_globals(globals_.size());
        return program_;
    }

    void visit(AST::Program& expr) {
        for (auto c : expr.children) {
            c->accept(*((Visitor*)this));
        }
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
        /*expr.Expr->accept(*((Visitor*)this));  // do we have to clear the stack ?

        rfun_->instructions.push_back(Instruction(Operation::If, 2));
        int lo_idx = rfun_->instructions.size();
        rfun_->instructions.push_back(Instruction(Operation::Goto, std::nullopt));  // dummy value

        expr.children[0]->accept(*((Visitor*)this));

        if (expr.children.size() > 1)
            rfun_->instructions.push_back(Instruction(Operation::Goto, std::nullopt));  // dummy value

        int hi_idx0 = rfun_->instructions.size();
        rfun_->instructions[lo_idx] = Instruction(Operation::Goto, hi_idx0 - lo_idx);

        if (expr.children.size() > 1) {
            expr.children[1]->accept(*((Visitor*)this));

            int hi_idx1 = rfun_->instructions.size();
            rfun_->instructions[hi_idx0 - 1] = Instruction(Operation::Goto, hi_idx1 - (hi_idx0 - 1));
        }*/
    }

    void visit(AST::WhileLoop& expr) {
        /*int lo_idx0 = rfun_->instructions.size();

        expr.Expr->accept(*((Visitor*)this));  // do we have to clear the stack ?

        rfun_->instructions.push_back(Instruction(Operation::If, 2));
        int lo_idx1 = rfun_->instructions.size();
        rfun_->instructions.push_back(Instruction(Operation::Goto, std::nullopt));  // dummy value

        expr.children[0]->accept(*((Visitor*)this));

        int hi_idx = rfun_->instructions.size();
        rfun_->instructions.push_back(Instruction(Operation::Goto, lo_idx0 - hi_idx));
        rfun_->instructions[lo_idx1] = Instruction(Operation::Goto, hi_idx - lo_idx1 + 1);*/
    }

    void visit(AST::FunctionDeclaration& expr) {
        /*struct Function* tfun = rfun_; // save current function into temporal variable
        bool tscope = global_scope_; // save global_scope_ flag
        std::set<string> tglobals = globals_; // save current global variables
		std::set<string> tref = ref_;
		
        rfun_ = new struct Function; // create new function
        global_scope_ = false; // if we declare new function we are not anymore in the global scope
        rfun_->parameter_count_ = expr.arguments.size(); // initialize fields of new function
        tfun->functions_.push_back(rfun_);
        
        for (auto s : tfun->local_reference_vars_)
        	ref_.insert(s);
  
        FreeVariables freeVar; // use visitor class to resolve all variable types, see FreeVariables.h for further explanations
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

        for (auto s : cur_var) {
            if (count(glob_var.begin(), glob_var.end(), s))
                continue;
            if (count(expr.arguments.begin(), expr.arguments.end(), s))
                continue;
            if (count(ass_var.begin(), ass_var.end(), s))
                continue;

            if (globals_.count(s))
                rfun_->names_.push_back(s);
        }

        for (auto s : free_var) {
            if (count(glob_var.begin(), glob_var.end(), s))
                continue;
            if (count(expr.arguments.begin(), expr.arguments.end(), s))
                continue;
            if (count(ass_var.begin(), ass_var.end(), s))
                continue;
            if (count(rfun_->free_vars_.begin(), rfun_->free_vars_.end(), s))
                continue;

            if (!globals_.count(s) && ref_.count(s))
                rfun_->free_vars_.push_back(s);
        }

        for (auto s : expr.arguments) {
            // if (count(glob_var.begin(), glob_var.end(), s)) continue;
            rfun_->local_vars_.push_back(s);
            if (count(nb_var.begin(), nb_var.end(), s)) {
                rfun_->local_reference_vars_.push_back(s);
            }
            if (globals_.count(s))
                globals_.erase(s);
            if (ref_.count(s))
            	ref_.erase(s);
           
        }

        for (auto s : ass_var) {
            if (count(glob_var.begin(), glob_var.end(), s))
                continue;
            if (count(rfun_->local_vars_.begin(), rfun_->local_vars_.end(), s))
                continue;
            rfun_->local_vars_.push_back(s);
            if (count(nb_var.begin(), nb_var.end(), s))
                rfun_->local_reference_vars_.push_back(s);
            if (globals_.count(s))
                globals_.erase(s);
            if (ref_.count(s))
            	ref_.erase(s);
        }

        for (auto s : glob_var) {
            globals_.insert(s);
            if (!count(rfun_->names_.begin(), rfun_->names_.end(), s))
                rfun_->names_.push_back(s);
            if (ref_.count(s))
            	ref_.erase(s);
        }

        // finding local decl

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
        rfun_->instructions.push_back(Instruction(Operation::Return, std::nullopt));

        rfun_ = tfun;
        global_scope_ = tscope;
        globals_ = tglobals;
        ref_ = tref;*/
    }

    void visit(AST::BinaryExpression& expr) {
    	/*IR::Operand opr1, opr2;
        expr.children[0]->accept(*((Visitor*)this));
        if (is_opr_)
        	opr1 = is_opr;
        else 
        	opr1 = {IR::Operand::OpType::VIRT_REG, ret_reg_};
        expr.children[1]->accept(*((Visitor*)this));
        if (is_opr_)
        	opr2 = is_opr;
        else 
        	opr2 = {IR::Operand::OpType::VIRT_REG, ret_reg_};*/
        
        /*if (expr.op == "<") {
            rfun_->instructions.push_back(Instruction(Operation::Geq, std::nullopt));
            rfun_->instructions.push_back(Instruction(Operation::Not, std::nullopt));
            return;
        }

        if (expr.op == "<=") {
            rfun_->instructions.push_back(Instruction(Operation::Gt, std::nullopt));
            rfun_->instructions.push_back(Instruction(Operation::Not, std::nullopt));
            return;
        }*/

        /*IR::Operation op;
        if (expr.op == "+")
            op = IR::Operation::ADD;
        else if (expr.op == "-")
            op = IR::Operation::S;
        else if (expr.op == "/")
            op = IR::Operation::Div;
        else if (expr.op == "*")
            op = IR::Operation::Mul;
        else if (expr.op == ">")
            op = IR::Operation::Gt;
        else if (expr.op == ">=")
            op = IR::Operation::Geq;
        else if (expr.op == "==")
            op = IR::Operation::Eq;
        else if (expr.op == "&")
            op = IR::Operation::And;
        else if (expr.op == "|")
            op = IR::Operation::Or;

        block_->instructions.push_back({op, {IR::Operand::OpType::VIRT_REG, reg_cnt_}, opr1, opr2});
        ret_reg_ = reg_cnt_;
        reg_cnt_++;*/
    }

    void visit(AST::UnaryExpression& expr) {
        /*expr.children[0]->accept(*((Visitor*)this));

        Operation op;
        if (expr.op == "!")
            op = Operation::Not;
        else if (expr.op == "-")
            op = Operation::Neg;

        rfun_->instructions.push_back(Instruction(op, std::nullopt));*/
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
        /*rfun_->instructions.push_back(Instruction(Operation::LoadConst, rfun_->constants_.size()));
        rfun_->constants_.push_back(expr.getVal());*/
    }

    void visit(AST::StringConstant& expr) {
        /*string val = expr.getVal();
        if (val[0] == '"') {  // in this case we are dealing with string constants
            rfun_->instructions.push_back(Instruction(Operation::LoadConst, rfun_->constants_.size()));
            rfun_->constants_.push_back(val.substr(1, val.size() - 2));
        } else {
        	
        
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
        /*rfun_->instructions.push_back(Instruction(Operation::LoadConst, rfun_->constants_.size()));
        rfun_->constants_.push_back(expr.getVal());*/
    }

    void visit(AST::NoneConstant& expr) {
        /*rfun_->instructions.push_back(Instruction(Operation::LoadConst, rfun_->constants_.size()));
        rfun_->constants_.push_back(Constant{none_});*/
    }
};
