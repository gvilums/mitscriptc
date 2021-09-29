#pragma once

#include <string>
#include <map>
#include "AST.h"

using namespace std;

class Value{
	public:
		virtual bool isInteger();
		virtual bool isBoolean();
		virtual bool isString();
		virtual bool isFunction();
		virtual bool isRecord();
		virtual bool isNone();
		virtual string toString();
		virtual bool isEqual(Value* other);
		virtual bool sameClass(Value* other);
};

class Frame{
	private:
		std::map<string, Value*> dict;
		Frame* parent;
		Frame* global;
		std::unordered_set<string> gvariables;
	public:
		Frame();
		Frame(Frame* par, Frame* glob);
		Frame* lookupWrite(string x);
		Frame* lookupRead(string x);
		void addGlobalVariables(vector<string> vec);
		void insert(string x, Value* v);
		Value* get(string x);
		bool count(string x);
};


bool Value::isInteger(){
	return false;
}	
bool Value::isBoolean(){
	return false;
}
bool Value::isString(){
	return false;
}
bool Value::isFunction(){
	return false;
}
bool Value::isRecord(){
	return false;
}
bool Value::isNone(){
	return false;
}
string Value::toString(){
	return "";
}
bool Value::isEqual(Value* other){
	return false;
}
bool Value::sameClass(Value* other){
	return false;
}


class Integer : public Value{
	private:
		int value;
	public:
		Integer(){}
		Integer(int v) : value(v){}
		int getValue(){
			return value;
		}
		void setValue(int v){
			value = v;
		}
		void invert(){
			value = -value;
		}
		bool isInteger() override{
			return true;
		}
		string toString() override{
			return to_string(value);
		}
		bool isEqual(Value* other) override{
			return value == ((Integer*) other)->getValue();
		}
		bool sameClass(Value* other) override{
			return other->isInteger();
		}
};

class Boolean : public Value{
	private:
		bool value;
	public:
		Boolean(){}
		Boolean(bool v) : value(v){}
		bool getValue(){
			return value;
		}
		void setValue(bool v){
			value = v;
		}
		void invert(){
			value = !value;
		}
		bool isBoolean() override{
			return true;
		}
		string toString() override{
			return value ? "true" : "false";
		}
		bool isEqual(Value* other) override{
			return value == ((Boolean*) other)->getValue();
		}
		bool sameClass(Value* other) override{
			return other->isBoolean();
		}
};

class String : public Value{
	private:
		string value;
	public:
		String(){}
		String(string v) : value(v){}
		string getValue(){
			return value;
		}
		void setValue(string v){
			value = v;
		}
		bool isString() override{
			return true;
		}
		string toString() override{
			return value;
		}
		bool isEqual(Value* other) override{
			return value == ((String*) other)->getValue();
		}
		bool sameClass(Value* other) override{
			return other->isString();
		}
};

class Function : public Value{
	private:
		// frame to dsa
		Frame* frame;
		AST::Block* block;
		vector<string> arguments;
	public:
		Function(){}
		Function(vector<string> arg, AST::Block* stat, Frame* f) : arguments(arg), block(stat), frame(f){} 
		string toString() override{
			return "FUNCTION";
		}
		bool isFunction() override{
				return true;
			}
		bool isEqual(Value* other) override{
			if (other != this)
				return false;
			if (((Function*) other)->getBody() != this->block)
				return false;
			vector<string> arg2 = ((Function*) other)->getArguments();
			if (arguments.size() != arg2.size())
				return false;
			for(int i = 0; i < arg2.size(); i++)
				if(arguments[i] != arg2[i])
					return false;
			return true;
		}
		bool sameClass(Value* other) override{
			return other->isFunction();
		}
		AST::Block* getBody(){
			return block;
		}
		vector<string> getArguments(){
			return arguments;
		}
		Frame* getFrame(){
			return frame;
		}
};

class Record : public Value{
	private:
		map<string, Value*> dict;
	public:
		bool isRecord() override{
			return true;
		}
		bool isEqual(Value* other) override{
			return this == other;
		}
		string toString() override{
			string str = "{";
			for(auto p : dict) {
				str = str + p.first + ":" + p.second->toString() + " ";
			}
			str = str + "}";
			return str;
		}
		bool sameClass(Value* other) override{
			return other->isRecord();
		}
		void insert(string x, Value* v){
			dict[x] = v;
		}
		bool count(string x){
			return dict.count(x);
		}	
		Value* get(string x){
			return dict[x];
		}
};

class None : public Value{
	public:
		string toString() override{
			return "None";
		}
		bool isNone() override{
			return true;
		}
		bool isEqual(Value* other) override{
			return true;
		}
		bool sameClass(Value* other) override{
			return other->isNone();
		}
		
};




