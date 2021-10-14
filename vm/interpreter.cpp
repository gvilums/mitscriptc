#include <iostream>
#include <cassert>
#include <cstdio>
#include <fstream>

#include "bc-parser.h"
#include "bc-lexer.h"
#include "antlr4-runtime.h"
#include "MITScript.h"
#include "AST.h"
#include "parsercode.cpp"
#include "compiler.h"
#include "vm.h"

int main(int argc, const char *argv[])
{
  if (argc != 3)
  {
    std::cout <<"Usage: mitscript <type> <filename>\n";
    return 1;
  }

  std::string flag{argv[1]};
  
  Compiler compiler;
  struct Function* fn;
  if (flag == "-s") {
    std::ifstream file;
    file.open(argv[2]);
    if (!file.is_open())
    { 
        std::cout <<"Failed to open file: " <<argv[2] <<"\n";
        return 1;
    }
    antlr4::ANTLRInputStream input(file);
    MITScript lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    tokens.fill();

    AST::Program* program = Program(tokens);

    program->accept(compiler);
    fn = compiler.get_function();
  } else if (flag == "-b") {
	  std::FILE* file = std::fopen(argv[2], "r");
	  if (!file) {
		  std::cout << "Failed to open file: " << argv[2] << std::endl;
		  return 1;
	  }
	  void* scanner;
	bclex_init(&scanner);
	bcset_in(file, scanner);
    
    if(bcparse(scanner, fn) == 1){
      cout<<"Parsing failed"<<endl;
      return 1;
    }
  } else {
	  std::cout << "invalid flag" << std::endl;
	  return 1;
  }


  try {
	  VM vm(fn);
	  vm.exec();
  } catch (std::string s) {
	  std::cout << s << std::endl;
  }
  
  return 0;
}
