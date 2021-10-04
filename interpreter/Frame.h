#pragma once

#include <map>
#include <unordered_set>
#include <string>
#include "Value.h"

// the declaration is in Value.h

Frame::Frame() : parent(nullptr), global(nullptr){}

Frame::Frame(Frame* par, Frame* glob) : parent(par), global(glob){}	

Frame* Frame::lookupWrite(string x){
	if (gvariables.count(x))
		return global;
	return this;
}

Frame* Frame::lookupRead(string x){
	if (gvariables.count(x))
		return global;
	else if (dict.count(x))
		return this;
	else if (parent)
		return parent->lookupRead(x);
	return nullptr;
}
		
void Frame::addGlobalVariables(vector<string> vec){
	for(auto v : vec)
		gvariables.insert(v);
}
		
void Frame::insert(string x, Value* v){
	dict[x] = v;
}
		
Value* Frame::get(string x){
	return dict[x];
}
		
bool Frame::count(string x){
	return dict.count(x);
}
