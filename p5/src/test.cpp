#include "irprinter.cpp"
#include "ir.h"

int main(){
	IR::Instruction OP1 = {IR::Operation::ADD};
	IR::Instruction OP2 = {IR::Operation::SUB};
	IR::Instruction OP3 = {IR::Operation::PRINT};
	IR::Instruction OP4 = {IR::Operation::LOAD_ARG};
		
	IR::BasicBlock bb1 = {{}, {OP1, OP2, OP3}, {}, 0, 0};
	IR::BasicBlock bb2 = {{}, {OP4}, {}, 0, 0};
	
	IR::Function fun = {{bb1, bb2}, {}};
	
	IR::Program prog = {{fun}, {}}; 
	
	std::cout << prog << std::endl;
}
