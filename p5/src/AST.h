#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Visitor.h"

#define Assert(cond, msg)                   \
    if (!(cond)) {                          \
        std::cerr << msg << endl;           \
        throw SystemException("Bad stuff"); \
    }

using namespace std;

class Visitor;

namespace AST {

class AST_node {
   public:
    virtual void accept(Visitor& v) = 0;
    virtual ~AST_node() {}
};

class Statement : public AST_node {
   public:
    virtual string tostring() {
        return "statment";
    }
    
    virtual ~Statement() override {}
    
};

class Program : public AST_node {
   public:
    vector<Statement*> children;
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    string tostring() {
        string res = "program";
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    void addChild(Statement* child) {
        children.push_back(child);
    }
    virtual ~Program() override {
    	for (auto& c : children) {
    		// c->~Statement();
    		delete c;
    	}
    }
};

class Block : Statement {
   public:
    vector<Statement*> children;
    string tostring() override {
        string res = "block";
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    void addChild(Statement* child) {
        children.push_back(child);
    }
    virtual ~Block() override {
    	for (auto& c : children) {
    		// c->~Statement();
    		delete c;
    	}
    }
};

class Expression : public AST_node {
   public:
    virtual string tostring() {
        return "expression";
    }
    virtual bool isFieldDereference() {
        return false;
    }
    virtual bool isIndexExpression() {
        return false;
    }
    virtual bool isStringConstant() {
        return false;
    }
    virtual ~Expression() override {};
};

class BinaryExpression : Expression {
   public:
    vector<Expression*> children;
    string op;
    string tostring() override {
        string res = "binaryexpression" + op;
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    void addChild(Expression* child) {
        children.push_back(child);
    }
    void addOp(string o) {
        op = o;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~BinaryExpression() override {
    	for (auto& c : children) {
    		// c->~Expression();
    		delete c;
    	}
    }
};

class UnaryExpression : Expression {
   public:
    vector<Expression*> children;
    string op;
    string tostring() override {
        string res = "unaryexpression" + op;
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    void addChild(Expression* child) {
        children.push_back(child);
    }
    void addOp(string o) {
        op = o;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~UnaryExpression() override {
    	for (auto& c : children) {
    		// c->~Expression();
    		delete c;
    	}
    }
};

class FunctionDeclaration : Expression {
   public:
    vector<string> arguments;
    AST::Block* block;
    string tostring() override {
        string res = "function(";
        for (auto c : arguments)
            res = res + c + ",";
        res = res + ")[" + block->tostring() + "]";
        return res;
    }
    void addArg(string child) {
        arguments.push_back(child);
    }
    void addBody(AST::Block* blk) {
        block = blk;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~FunctionDeclaration() override {
    	// block->~Block();
    	delete block;
    }
};

class Call : Expression, Statement {
   public:
    vector<Expression*> arguments;
    Expression* expr;
    bool isStatement = false;
    string tostring() override {
        string res = "call " + expr->tostring() + " with (";
        for (auto c : arguments)
            res = res + c->tostring() + ",";
        res = res + ")";
        return res;
    }
    void addArg(Expression* child) {
        arguments.push_back(child);
    }
    void addExpr(Expression* exp) {
        expr = exp;
    }
    void setStatement() {
        isStatement = true;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~Call() override {
    	for (auto& c : arguments) {
    		// c->~Expression();
    		delete c;
    	}
    	// expr->~Expression();
    	delete expr;
    }
};

class FieldDereference : Expression {
   public:
    Expression* baseexpr;
    string field;
    void addBaseexpr(Expression* be) {
        baseexpr = be;
    }
    void addField(string f) {
        field = f;
    }
    virtual string tostring() override {
        return baseexpr->tostring() + "." + field;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    bool isFieldDereference() override {
        return true;
    }
    virtual ~FieldDereference() override {
    	delete baseexpr;
    }
};

class IndexExpression : Expression {
   public:
    Expression* baseexpr;
    Expression* index;
    void addBaseexpr(Expression* be) {
        baseexpr = be;
    }
    void addIndex(Expression* idx) {
        index = idx;
    }
    virtual string tostring() override {
        return baseexpr->tostring() + "[" + index->tostring() + "]";
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    bool isIndexExpression() override {
        return true;
    }
    virtual ~IndexExpression() override {
    	delete baseexpr;
    	delete index;
    }
};

class Record : Expression {
   public:
    vector<pair<string, Expression*>> dict;

    void addMap(string str, Expression* expr) {
        dict.push_back({str, expr});
    }
    string tostring() override {
        string res = "{";
        for (auto p : dict)
            res = res + p.first + "->" + p.second->tostring() + ",";
        res = res + "}";
        return res;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~Record() override {
    	for (auto& p : dict) {
    		delete p.second;
    	}
    }
};

class IntegerConstant : Expression {
   public:
    string val;  //enough ?
    void addVal(string v) {
        val = v;
    }
    string tostring() override {
        return val;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    int getVal() {
        return stoi(val);
    }
    virtual ~IntegerConstant() override {}
};

class StringConstant : Expression {
   public:
    string val;
    void addVal(string v) {
        val = v;
    }
    virtual string tostring() override {
        return val;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    string getVal() {
        return val;
    }
    bool isStringConstant() override {
        return true;
    }
    virtual ~StringConstant() override {}
};

class BoolConstant : Expression {
   public:
    string val;
    void addVal(string v) {
        val = v;
    }
    string tostring() override {
        return val;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    bool getVal() {
        return val == "true";
    }
    virtual ~BoolConstant() override {}
};

class NoneConstant : Expression {
   public:
    string tostring() override {
        return "None";
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~NoneConstant() override {}
};

class Global : Statement {
   public:
    string name;
    string tostring() override {
        return "global";
    }
    void addName(string n) {
        name = n;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~Global() override {}
};

class Return : Statement {
   public:
    AST::Expression* Expr;
    string tostring() override {
        return "return " + Expr->tostring();
    }
    void addExpr(AST::Expression* expr) {
        Expr = expr;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~Return() override {
    	delete Expr;
    }
};

class Assignment : Statement {
   public:
    AST::Expression* Lhs;
    AST::Expression* Expr;
    string tostring() override {
        return "assignment " + Lhs->tostring() + " = " + Expr->tostring();
    }
    void addLhs(AST::Expression* lhs) {
        Lhs = lhs;
    }
    void addExpr(AST::Expression* expr) {
        Expr = expr;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~Assignment() override {
    	delete Lhs;
    	delete Expr;
    }
};

class IfStatement : Statement {
   public:
    vector<Block*> children;
    Expression* Expr;
    string tostring() override {
        string res = "if " + Expr->tostring() + " statement";
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    void addChild(Block* child) {
        children.push_back(child);
    }
    void addExpr(AST::Expression* expr) {
        Expr = expr;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~IfStatement() override {
    	delete Expr;
    	for (auto& c : children) {
    		delete c;
    	}
    }
};

class WhileLoop : Statement {
   public:
    vector<Block*> children;
    Expression* Expr;
    string tostring() override {
        string res = "while " + Expr->tostring() + " do";
        for (auto c : children)
            res = res + "[" + c->tostring() + "]";
        return res;
    }
    void addChild(Block* child) {
        children.push_back(child);
    }
    void addExpr(AST::Expression* expr) {
        Expr = expr;
    }
    virtual void accept(Visitor& v) override {
        v.visit(*this);
    }
    virtual ~WhileLoop() override {
    	delete Expr;
    	for (auto& c : children) {
    		delete c;
    	}
    	
    }
};

}
