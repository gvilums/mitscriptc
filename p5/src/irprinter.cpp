#include "ir.h"

#include <iostream>

std::ostream& operator<<(std::ostream& os, const IR::Function& prog){
	os << "\t\t\t" << std::endl;
	
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


