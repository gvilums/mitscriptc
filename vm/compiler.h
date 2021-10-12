#pragma once

#include "../interpreter/AST.h"
#include "types.h"

using namespace std;

class Compiler : public Visitor {
public:
	void visit(AST::Program& expr){}
	void visit(AST::Block& expr){}
	void visit(AST::Global& expr){}
	void visit(AST::Return& expr){}
	void visit(AST::Assignment& expr){}
	void visit(AST::IfStatement& expr){}
	void visit(AST::WhileLoop& expr){}
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
