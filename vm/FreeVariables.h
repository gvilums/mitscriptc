#pragma once

#include <vector>
#include <string>
#include <set>

#include "Assigns.h"
#include "Globals.h"

using namespace std;

class FreeVariables : public Visitor {
	vector<string> variables;
	vector<string> nb_variables;
	vector<string> cur_variables;
public:
	vector<string> getFreeVariables(){
		return variables;
	}
	vector<string> getNotBoundVariables(){
		return nb_variables;
	}
	vector<string> getCurVariables(){
		return cur_variables;
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
	
	void visit(AST::Return& expr){
		expr.Expr->accept(*((Visitor*) this));
	}
	
	void visit(AST::Assignment& expr){
		expr.Expr->accept(*((Visitor*) this));
		expr.Lhs->accept(*((Visitor*) this));
	}
	
	void visit(AST::IfStatement& expr){
		expr.Expr->accept(*((Visitor*) this));
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::WhileLoop& expr){
		expr.Expr->accept(*((Visitor*) this));
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::BinaryExpression& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::UnaryExpression& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::FunctionDeclaration& expr){
		FreeVariables free_var;
		expr.block->accept(free_var);
		vector<string> fv = free_var.getFreeVariables();
		
		Globals globals;
		expr.block->accept(globals);
		vector<string> glob = globals.getGlobals();
		
		Assigns assigns;
		expr.block->accept(assigns);
		vector<string> var = assigns.getVariables();
		
		for (auto s : fv) {
			if (count(glob.begin(), glob.end(), s)) continue;	
			if (count(var.begin(), var.end(), s)) continue;
			if (count(expr.arguments.begin(), expr.arguments.end(), s)) continue;
			nb_variables.push_back(s);
			variables.push_back(s);
		}
	}
	
	void visit(AST::Call& expr){
		expr.expr->accept(*((Visitor*) this));
		for (auto c : expr.arguments)
			c->accept(*((Visitor*) this));
	}
	
	void visit(AST::FieldDereference& expr){
		expr.baseexpr->accept(*((Visitor*) this));
	}
	
	void visit(AST::IndexExpression& expr){
		expr.baseexpr->accept(*((Visitor*) this));
		expr.index->accept(*((Visitor*) this));
	}
	
	void visit(AST::Record& expr){
		for (auto p : expr.dict)
			p.second->accept(*((Visitor*) this));
	}
	
	void visit(AST::StringConstant& expr){
		if (expr.getVal()[0] != '"') {
			variables.push_back(expr.getVal());
			cur_variables.push_back(expr.getVal());
			// nb_variables.push_back(expr.getVal());
		}
	}
	
	void visit(AST::IntegerConstant& expr){}
	void visit(AST::BoolConstant& expr){}
	void visit(AST::NoneConstant& expr){}
};
