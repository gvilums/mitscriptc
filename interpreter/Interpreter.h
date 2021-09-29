#pragma once

#include <string>
#include <iostream>
#include <stack>
#include "Value.h"
#include "Frame.h"
#include "AST.h"
#include "Exceptions.h"
#include "Globals.h"
#include "Assigns.h"

using namespace std;

class Interpreter : public Visitor {
	private:
		bool return_;
		Value* rval_;
		std::stack<Frame*> stack_;
		Frame* global_;
		None* none_;
		Function* print_;
		Function* input_;
		Function* intcast_;
	public:
	Interpreter(){
		return_ = false;
		global_ = new Frame();
		none_ = new None();
		stack_.push(global_);
		
		print_ = new Function({"s"}, nullptr, nullptr);
		input_ = new Function({}, nullptr, nullptr);
		intcast_ = new Function({"s"}, nullptr, nullptr);
		
		global_->insert("print", print_);
		global_->insert("input", input_);
		global_->insert("intcast", intcast_);
	}
	
	string getVal(){
		return rval_->toString();
	}
	
	void print(AST::Expression* expr){
		expr->accept(*((Visitor*) this));
		cout << rval_->toString() << endl;
	}
	
	void input(){
		string str;
		getline(cin, str);
		rval_ = new String(str);
	}	
	
	void intcast(AST::Expression* expr){
		expr->accept(*((Visitor*) this));
		if (!rval_->isString()){
			IllegalCastException e;
			throw e;
		}
		
		String* str = (String*) rval_;
		for(auto c : str->getValue())
			if (!isdigit(c)){
				IllegalCastException e;
				throw e;
			}
		
		rval_ = new Integer(stoi(str->getValue()));
	}
	
	void visit(AST::Call& expr){
		expr.expr->accept(*((Visitor*) this));
		
		if (!rval_->isFunction()){
			IllegalCastException e;
			throw e;
		}
		
		Function* fun = (Function*) rval_;
		
		vector<string> arg_names = fun->getArguments();
		vector<Value*> val;
		if (expr.arguments.size() != arg_names.size()){
			RuntimeException e;
			throw e;
		}
		
		if (fun == print_) 
			print(expr.arguments[0]);
		else if (fun == input_)
			input();
		else if (fun == intcast_)
			intcast(expr.arguments[0]);
		else {
			Frame* frame = new Frame(fun->getFrame(), global_);
			for(int i = 0; i < expr.arguments.size(); i++){
				expr.arguments[i]->accept(*((Visitor*) this));
				val.push_back(rval_);
			}
			
			Globals globals;
			Assigns assigns;
			fun->getBody()->accept(globals);
			fun->getBody()->accept(assigns);
			
			frame->addGlobalVariables(globals.getGlobals());

			for (auto x : assigns.getVariables()){
				frame->insert(x, none_);
			}
			for(int i = 0; i < expr.arguments.size(); i++){
				frame->insert(arg_names[i], val[i]);
			}
			
			stack_.push(frame);
			fun->getBody()->accept(*((Visitor*) this));
			stack_.pop();
			
			if(return_)
				return_ = false;
			else
				rval_ = none_;
		}
	}
	
	void visit(AST::Global& expr){}
	
	void visit(AST::Return& expr){
		expr.Expr->accept(*((Visitor*) this));
		return_ = true;
	}
	
	void visit(AST::Assignment& expr){
		if (expr.Lhs->isStringConstant()){
			expr.Expr->accept(*((Visitor*) this));
			Value* val = rval_;
			string x = ((AST::StringConstant*) expr.Lhs)->getVal();
			
			Frame* frame = stack_.top()->lookupWrite(x);
			frame->insert(x, val);
		}
		else if (expr.Lhs->isFieldDereference()){
			AST::FieldDereference* fd = (AST::FieldDereference*) expr.Lhs;
			fd->baseexpr->accept(*((Visitor*) this));
			Value* record = rval_;
			
			if (!record->isRecord()){
				IllegalCastException e;
				throw e;
			}
			
			expr.Expr->accept(*((Visitor*) this));
			Value* val = rval_;
			
			((Record*) record)->insert(fd->field, val);
		}
		else if (expr.Lhs->isIndexExpression()){
			AST::IndexExpression* ie = (AST::IndexExpression*) expr.Lhs;
			ie->baseexpr->accept(*((Visitor*) this));
			Value* record = rval_;
				
			if (!record->isRecord()){
				IllegalCastException e;
				throw e;
			}
			
			ie->index->accept(*((Visitor*) this));
			Value* index = rval_;
			
			expr.Expr->accept(*((Visitor*) this));
			Value* val = rval_;
			
			((Record*) record)->insert(index->toString(), val);
		}
		else {
			RuntimeException e;
			throw e;
		}
	}
	
	void visit(AST::Program& expr){
		for(auto c : expr.children) {
			c->accept(*((Visitor*) this));
			if (return_)
				return;
		}
	}
	
	void visit(AST::Block& expr){
		for(auto c : expr.children) {
			c->accept(*((Visitor*) this));
			if (return_)
				return;
		}
	}
	
	void visit(AST::IfStatement& expr){
		expr.Expr->accept(*((Visitor*) this));
		Value* cond = rval_;

		if (!cond->isBoolean()){
			IllegalCastException e;
			throw e;
		}
		
		if (((Boolean*) cond)->getValue())
			expr.children[0]->accept(*((Visitor*) this));
		else if(expr.children.size() > 1)
			expr.children[1]->accept(*((Visitor*) this));
	}
	
	void visit(AST::WhileLoop& expr){
		while(true){
			if (return_)
				break;
			expr.Expr->accept(*((Visitor*) this));
			Value* cond = rval_;
			

			if (!cond->isBoolean()){
				IllegalCastException e;
				throw e;
			}
					
			if (((Boolean*) cond)->getValue())
				expr.children[0]->accept(*((Visitor*) this));
			else
				break;
			}
	}
	
	void visit(AST::BinaryExpression& expr){
		if (expr.op == "-" || expr.op == "/" || expr.op == "*"){
			expr.children[0]->accept(*((Visitor*) this));
			if (!rval_->isInteger()){
				IllegalCastException e;
				throw e;
			}
			Integer *t1;
			t1 = (Integer*) rval_;
			
			expr.children[1]->accept(*((Visitor*) this));
			if (!rval_->isInteger()){
				IllegalCastException e;
				throw e;
			}
			Integer *t2;
			t2 = (Integer*) rval_;
			
			if (expr.op == "/" && t2->getValue() == 0){
				IllegalArithmeticException e;
				throw e;
			}
			
			int newval;
			if (expr.op == "-") newval = t1->getValue() - t2->getValue();
			if (expr.op == "*") newval = t1->getValue() * t2->getValue();
			if (expr.op == "/") newval = t1->getValue() / t2->getValue();
			rval_ = new Integer(newval);
		}
		else if (expr.op == "+"){
			expr.children[0]->accept(*((Visitor*) this));
			Value *t1 = rval_;
			expr.children[1]->accept(*((Visitor*) this));
			Value *t2 = rval_;
			
			if (t1->isString() || t2->isString()){
				rval_ = new String(t1->toString() + t2->toString());
			}
			else if(t1->isInteger() && t2->isInteger())
				rval_ = new Integer(((Integer*) t1)->getValue() + ((Integer*) t2)->getValue());
			else{
				IllegalCastException e;
				throw e;
			}
		}
		else if (expr.op == "<" || expr.op == "<=" || expr.op == ">" || expr.op == ">="){
			expr.children[0]->accept(*((Visitor*) this));
			if (!rval_->isInteger()){
				IllegalCastException e;
				throw e;
			}
			Integer *t1;
			t1 = (Integer*) rval_;
			
			expr.children[1]->accept(*((Visitor*) this));
			if (!rval_->isInteger()){
				IllegalCastException e;
				throw e;
			}
			Integer *t2;
			t2 = (Integer*) rval_;
			
			bool newval;
			if (expr.op == "<") newval = t1->getValue() < t2->getValue();
			if (expr.op == "<=") newval = t1->getValue() <= t2->getValue();
			if (expr.op == ">") newval = t1->getValue() > t2->getValue();
			if (expr.op == ">=") newval = t1->getValue() >= t2->getValue();
			
			rval_ = new Boolean(newval);
		}
		else if (expr.op == "=="){
			expr.children[0]->accept(*((Visitor*) this));
			Value *t1 = rval_;
			expr.children[1]->accept(*((Visitor*) this));
			Value *t2 = rval_;
			
			if (!t1->sameClass(t2))
				rval_ = new Boolean(false);
			else 
				rval_ = new Boolean(t1->isEqual(t2));
		}
		else if (expr.op == "|" || expr.op == "&"){
			expr.children[0]->accept(*((Visitor*) this));
			if (!rval_->isBoolean()){
				IllegalCastException e;
				throw e;
			}
			Boolean *t1;
			t1 = (Boolean*) rval_;
			
			expr.children[1]->accept(*((Visitor*) this));
			if (!rval_->isBoolean()){
				IllegalCastException e;
				throw e;
			}
			Boolean *t2;
			t2 = (Boolean*) rval_;
			
			bool newval;
			if (expr.op == "&") newval = t1->getValue() && t2->getValue();
			if (expr.op == "|") newval = t1->getValue() || t2->getValue();
			
			rval_ = new Boolean(newval);
		}
	}
	
	void visit(AST::UnaryExpression& expr){
		if (expr.op == "!"){
			expr.children[0]->accept(*((Visitor*) this));
			if (!rval_->isBoolean()){
				IllegalCastException e;
				throw e;
			}
			rval_ = new Boolean(!((Boolean*) rval_)->getValue());
		}
		else if (expr.op == "-"){
			expr.children[0]->accept(*((Visitor*) this));
			if (!rval_->isInteger()){
				IllegalCastException e;
				throw e;
			}
			rval_ = new Integer(-((Integer*) rval_)->getValue());
		}
	}
	
	void visit(AST::IndexExpression& expr){
		expr.baseexpr->accept(*((Visitor*) this));
		Value* record = rval_;
		
		expr.index->accept(*((Visitor*) this));
		Value* index = rval_;
		
		if (!record->isRecord()){
			IllegalCastException e;
			throw e;
		}
		
		if (((Record*) record)->count(index->toString()))
			rval_ = ((Record*) record)->get(index->toString());
		else
			rval_ = (Value*) none_;
	}
	
	void visit(AST::FieldDereference& expr){
		expr.baseexpr->accept(*((Visitor*) this));
		Value* record = rval_;
		
		if (!record->isRecord()){
			IllegalCastException e;
			throw e;
		}
		
		if (((Record*) record)->count(expr.field))
			rval_ = ((Record*) record)->get(expr.field);
		else
			rval_ = (Value*) none_;
	}
	
	void visit(AST::Record& expr){
		Record* record = new Record();
		for(auto p : expr.dict){
			p.second->accept(*((Visitor*) this));
			record->insert(p.first, rval_);
		}
		rval_ = (Value*) record;
	}
	
	void visit(AST::IntegerConstant& expr){
		rval_ = new Integer(expr.getVal());
	}
	
	void visit(AST::StringConstant& expr){
		string val = expr.getVal();
		if (val[0] == '"') 
			rval_ = new String(val.substr(1, val.size() - 2));
		else {
			Frame* frame = stack_.top()->lookupRead(val);
			
			if (!frame){
				UninitializedVariableException e;
				throw e;
			}
			
			rval_ = frame->get(val);
		}
	}
	
	void visit(AST::BoolConstant& expr){
		rval_ = new Boolean(expr.getVal());
	}
	
	void visit(AST::NoneConstant& expr){
		rval_ = none_; // or new None?
	}
	
	void visit(AST::FunctionDeclaration& expr){
		rval_ = new Function(expr.arguments, expr.block, stack_.top());
	}
};
