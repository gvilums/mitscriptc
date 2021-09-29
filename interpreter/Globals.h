#pragma once

#include <vector>
#include <string>

using namespace std;

class Globals : public Visitor {
	vector<string> globals;
public:
	void visit(AST::Program& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	void visit(AST::Block& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	void visit(AST::Global& expr){
		globals.push_back(expr.name);
	}
	void visit(AST::Return& expr){}
	void visit(AST::Assignment& expr){}
	void visit(AST::IfStatement& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	void visit(AST::WhileLoop& expr){
		for(auto c : expr.children)
			c->accept(*((Visitor*) this));
	}
	vector<string> getGlobals(){
		return globals;
	}
	
	void visit(AST::BinaryExpression& expr){}
	void visit(AST::UnaryExpression& expr){}
	void visit(AST::FunctionDeclaration& expr){}
	void visit(AST::Call& expr){}
	void visit(AST::FieldDereference& expr){}
	void visit(AST::IndexExpression& expr){}
	void visit(AST::Record& expr){}
	void visit(AST::IntegerConstant& expr){}
	void visit(AST::StringConstant& expr){}
	void visit(AST::BoolConstant& expr){}
	void visit(AST::NoneConstant& expr){}
};
