#pragma once

#include "AST.h"
#include <string>
#include <iostream>

using namespace std;

//This is where you get to define your pretty printer class, which should be
//a subtype of visitor.

class PrettyPrinter : public Visitor {
	private:
		string result = "";
		int tabs = 0;
		string currTabs(){
			string r = "";
			for(int i = 0; i < tabs; i++)
				r += "    "; // spaces vs tabs
			return r;
		}
	public:
		void visit(AST::Program& expr){
			for(auto c : expr.children)
				c->accept(*((Visitor*) this));
		}
		void visit(AST::Block& expr){
			result += "{\n";
			tabs++;
			for(auto c : expr.children)
				c->accept(*((Visitor*) this));
			tabs--;
			result += currTabs();
			result += "}";
		}
		void visit(AST::BinaryExpression& expr){
			result += "(";
			expr.children[0]->accept(*((Visitor*) this));
			result += expr.op;
			expr.children[1]->accept(*((Visitor*) this));
			result += ")";
		}
		void visit(AST::UnaryExpression& expr){
			result += expr.op;
			result += "(";
			expr.children[0]->accept(*((Visitor*) this));
			result += ")";
		}
		void visit(AST::FunctionDeclaration& expr){
			result += "fun(";
			bool loop = false;
			for(auto c : expr.arguments){
				result += c + ",";
				loop = true;
			}
			if(loop) result.pop_back();
			result += ")";
			expr.block->accept(*((Visitor*) this));
		}
		void visit(AST::Call& expr){
			if(expr.isStatement)
				result += currTabs();
			expr.expr->accept(*((Visitor*) this));
			result += "(";
			bool loop = false;
			for(auto c : expr.arguments){
				c->accept(*((Visitor*) this));
				result += ",";
				loop = true;
			}
			if(loop) result.pop_back();
			result += ")";
			if(expr.isStatement)
				result += ";\n";
			
		}
		virtual void visit(AST::FieldDereference& expr){
			expr.baseexpr->accept(*((Visitor*) this));
			result += "." + expr.field;
		}
		virtual void visit(AST::IndexExpression& expr){
			expr.baseexpr->accept(*((Visitor*) this));
			result += "[";
			expr.index->accept(*((Visitor*) this));
			result += "]";
		}
		virtual void visit(AST::Record& expr){
			result += "{";
			bool loop = false;
			for(auto p : expr.dict){
				result += p.first + " : ";
				p.second->accept(*((Visitor*) this));
				result += "; ";
				loop = true;
			}
			if(loop) result.pop_back();
			result += "}";
		}
		virtual void visit(AST::IntegerConstant& expr){
			result += expr.val;
		}
		virtual void visit(AST::StringConstant& expr){
			result += expr.val;
		}
		virtual void visit(AST::BoolConstant& expr){
			result += expr.val;
		}
		virtual void visit(AST::NoneConstant& expr){
			result += "None";
		}
		virtual void visit(AST::Global& expr){
			result += currTabs();
			result += "global ";
			result += expr.name;
			result += ";\n";
		}
		virtual void visit(AST::Return& expr){
			result += currTabs();
			result += "return ";
			expr.Expr->accept(*((Visitor*) this));
			result += ";\n";
		}
		virtual void visit(AST::Assignment& expr){
			result += currTabs();
			expr.Lhs->accept(*((Visitor*) this));
			result += " = ";
			expr.Expr->accept(*((Visitor*) this));
			result += ";\n";
		}
		virtual void visit(AST::IfStatement& expr){
			result += currTabs();
			result += "if(";
			expr.Expr->accept(*((Visitor*) this));
			result += ")";
			expr.children[0]->accept(*((Visitor*) this));
			
			if(expr.children.size() > 1){
				result += currTabs();
				result += "\n";
				result += currTabs();
				result += "else";
				expr.children[1]->accept(*((Visitor*) this));
			}
			result += "\n";
		}
		virtual void visit(AST::WhileLoop& expr){
			result += currTabs();
			result += "while(";
			expr.Expr->accept(*((Visitor*) this));
			result += ")";
			expr.children[0]->accept(*((Visitor*) this));
			result += "\n";
		}
		string getResult(){
			return result;
		}
};

