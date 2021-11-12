#include "ir.h"

#include <iostream>
#include <string>

std::string op_str[] = {
	"ADD", 
	"ADD_INT",
    "SUB",
    "MUL",
    "DIV",
    "EQ",
    "GT",
    "GEQ",
    "AND",
    "OR",
    "NOT",
    "LOAD_ARG",         
    "LOAD_FREE_REF",      
    "REF_LOAD",           
    "REF_STORE",        
    "REC_LOAD_NAME",
    "REC_LOAD_INDX",
    "REC_STORE_NAME",
    "REC_STORE_INDX",
    "ALLOC_REF",
    "ALLOC_REC",
    "ALLOC_CLOSURE",
    "SET_CAPTURE",       
    "SET_ARG",           
    "CALL",              
    "RETURN",
    "MOV",
    "LOAD_GLOBAL",
    "STORE_GLOBAL",
    "ASSERT_BOOL",
    "ASSERT_INT",
    "ASSERT_STRING",
    "ASSERT_RECORD",
    "ASSERT_CLOSURE",
    "ASSERT_NONZERO",
    "PRINT",
    "INPUT",
    "INTCAST"
};

std::string opr_str[] = {
	"NONE",
    "VIRT_REG",
    "IMMEDIATE",
    "LOGICAL",
    "MACHINE_REG",
    "STACK_SLOT"
};

std::ostream& operator<<(std::ostream& os, const IR::Operand& opr){
	os << "[" << opr_str[(int) opr.type] << " " << opr.index << "]";
	return os;
}

std::ostream& operator<<(std::ostream& os, const IR::Operation& op){
	os << op_str[(int) op];
	return os;
}

std::ostream& operator<<(std::ostream& os, const IR::Instruction& ins){
	os << ins.op << " " << ins.out << " " << ins.args[0] << " " << ins.args[1] << " " << ins.args[2];
	return os;
}

std::ostream& operator<<(std::ostream& os, const IR::BasicBlock& bb){
	for (auto ins : bb.instructions)
		os << "\t\t\t\t" << ins << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, const IR::Function& fun){
	for (size_t i = 0; i < fun.blocks.size(); i++){
		os << "\t\t\t" << i << " : [" << std::endl;
		os << fun.blocks[i];
		os << "\t\t\t];" << std::endl;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const IR::Program& prog){
	os << "program { " << std::endl;
	os << "\tfunction { " << std::endl;
	for (size_t i = 0; i < prog.functions.size(); i++)
		os << "\t\t" << i << " : {" << std::endl << prog.functions[i] << "\t\t}" << std::endl;
	os << "\t}" << std::endl;
	os << "\timmediates { " << std::endl;
	for (size_t i = 0; i < prog.immediates.size(); i++)
		os << "\t\t" << i << " : {" << std::endl << "}" << std::endl;
	os << "\t}" << std::endl;
	os << "}";
	return os;
}

