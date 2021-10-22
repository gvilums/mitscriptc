#pragma once

#include <string>
#include <vector>
#include "AST.h"
#include "Visitor.h"

using namespace std;

class Deallocator : public Visitor {
  public:
  	void delete_statement(AST::Statement* &st) {
  		if (dynamic_cast<AST::Call*>(st)) {
  			delete dynamic_cast<AST::Call*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Program*>(st)) {
  			delete dynamic_cast<AST::Program*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Global*>(st)) {
  			delete dynamic_cast<AST::Global*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Return*>(st)) {
  			delete dynamic_cast<AST::Return*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Assignment*>(st)) {
  			delete dynamic_cast<AST::Assignment*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::WhileLoop*>(st)) {
  			delete dynamic_cast<AST::WhileLoop*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::IfStatement*>(st)) {
  			delete dynamic_cast<AST::IfStatement*>(st);
  			return;
  		}	
  	}
  	void delete_expression(AST::Expression* &st) {
  		if (dynamic_cast<AST::BinaryExpression*>(st)) {
  			delete dynamic_cast<AST::BinaryExpression*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::UnaryExpression*>(st)) {
  			delete dynamic_cast<AST::UnaryExpression*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Call*>(st)) {
  			delete dynamic_cast<AST::Call*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::FieldDereference*>(st)) {
  			delete dynamic_cast<AST::FieldDereference*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::IndexExpression*>(st)) {
  			delete dynamic_cast<AST::IndexExpression*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::Record*>(st)) {
  			delete dynamic_cast<AST::Record*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::IntegerConstant*>(st)) {
  			delete dynamic_cast<AST::IntegerConstant*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::StringConstant*>(st)) {
  			delete dynamic_cast<AST::StringConstant*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::BoolConstant*>(st)) {
  			delete dynamic_cast<AST::BoolConstant*>(st);
  			return;
  		}
  		if (dynamic_cast<AST::NoneConstant*>(st)) {
  			delete dynamic_cast<AST::NoneConstant*>(st);
  			return;
  		}
  	}
  
    void visit(AST::Program& expr) {
        for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete_statement(c);
        }
    }
    
    void visit(AST::Block& expr) {
        for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete_statement(c);
        }
    }
    void visit(AST::Global& expr) {}
    void visit(AST::Return& expr) {
    	expr.Expr->accept(*((Visitor*)this));
    	delete_expression(expr.Expr);
    }
    
    void visit(AST::Assignment& expr) {
    	expr.Expr->accept(*((Visitor*)this));
    	delete_expression(expr.Expr);
    	expr.Lhs->accept(*((Visitor*)this));
    	delete_expression(expr.Lhs);
    }
    
    void visit(AST::IfStatement& expr) {
    	expr.Expr->accept(*((Visitor*)this));
    	delete_expression(expr.Expr);
        for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete c;
        }
        
    }
    void visit(AST::WhileLoop& expr) {
    	expr.Expr->accept(*((Visitor*)this));
    	delete_expression(expr.Expr);
        for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete c;
        }
    }
   
    void visit(AST::BinaryExpression& expr) {
    	for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete_expression(c);
        }
    }
    void visit(AST::UnaryExpression& expr) {
    	for (auto& c : expr.children) {
            c->accept(*((Visitor*)this));
            delete_expression(c);
        }
    }
    void visit(AST::FunctionDeclaration& expr) {
    	expr.block->accept(*((Visitor*)this));
    	delete expr.block;
    }
    void visit(AST::Call& expr) {
    	for (auto& c : expr.arguments) {
            c->accept(*((Visitor*)this));
             delete_expression(c);
        }
        expr.expr->accept(*((Visitor*)this));
        delete_expression(expr.expr); 
    }
    void visit(AST::FieldDereference& expr) {
    	expr.baseexpr->accept(*((Visitor*)this));
    	delete_expression(expr.baseexpr);
    }
    void visit(AST::IndexExpression& expr) {
    	expr.baseexpr->accept(*((Visitor*)this));
    	delete_expression(expr.baseexpr);
    	expr.index->accept(*((Visitor*)this));
    	delete_expression(expr.index);
    }
    void visit(AST::Record& expr) {
    	for(auto& p : expr.dict) {
    		p.second->accept(*((Visitor*)this));
    		delete_expression(p.second);
    	}
    }
    void visit(AST::IntegerConstant& expr) {}
    void visit(AST::StringConstant& expr) {}
    void visit(AST::BoolConstant& expr) {}
    void visit(AST::NoneConstant& expr) {}
};
