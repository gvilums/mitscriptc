#pragma once

#include "../interpreter/AST.h"
#include "types.h"
#include <set>

using namespace std;

class Compiler : public Visitor {
private:
	bool global_scope_;
	struct Function* rfun_;
	std::set<std::string> ref_var_;
	std::set<std::string> globals_;
	std::set<std::string> locals_;
	
public:	
	Compiler(){
		rfun_ = (struct Function*) malloc(sizeof(struct Function));
		global_scope_ = true;
	}

	struct Function* get_function(){
		return rfun_;
	}

	void visit(AST::Program& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::Block& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::Global& expr){}
	void visit(AST::Return& expr){}
	
	void visit(AST::Assignment& expr){
		if (expr.Lhs->isStringConstant()){
			locals_.insert(((AST::StringConstant*) expr.Lhs)->getVal());
			expr.Expr->accept(*((Visitor*) this));
		}
	}
	
	void visit(AST::IfStatement& expr){
		expr.children[0]->accept(*((Visitor*) this));
		if (expr.children.size() > 1) {
			expr.children[0]->accept(*((Visitor*) this));
		}
	}
	
	void visit(AST::WhileLoop& expr){
		expr.children[0]->accept(*((Visitor*) this));
	}
	
	void visit(AST::FunctionDeclaration& expr){
		struct Function* tfun = rfun_;
		bool tscope = global_scope_;
		
		global_scope_ = false;
		rfun_ = (struct Function*) malloc(sizeof(struct Function));
		rfun_->parameter_count_ = expr.arguments.size();
		tfun->functions_.push_back(rfun_);
		
		expr.block->accept(*((Visitor*) this));
		
		rfun_ = tfun;
		global_scope_ = tscope;
		
	}
	
	void visit(AST::BinaryExpression& expr){}
	void visit(AST::UnaryExpression& expr){}
	void visit(AST::Call& expr){}
	void visit(AST::FieldDereference& expr){}
	void visit(AST::IndexExpression& expr){}
	void visit(AST::Record& expr){}
	void visit(AST::IntegerConstant& expr){}
	void visit(AST::StringConstant& expr){}
	void visit(AST::BoolConstant& expr){}
	void visit(AST::NoneConstant& expr){}
};
