#include <iostream>
#include <cassert>
#include <fstream>

#include "antlr4-runtime.h"
#include "MITScript.h"
#include "AST.h"
#include "parsercode.cpp"
#include "compiler.h"
#include "prettyprinter.h"

int main(int argc, const char *argv[])
{
  if (argc != 2)
  {
    std::cout <<"Usage: mitscript <filename>\n";
    return 1;
  }

  std::ifstream file;
  file.open(argv[1]);
	
  if (!file.is_open())
  { 
      std::cout <<"Failed to open file: " <<argv[1] <<"\n";
      return 1;
  }

  antlr4::ANTLRInputStream input(file);
  MITScript lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  tokens.fill();

  AST::Program* program = Program(tokens);
  
  Compiler compiler;
  program->accept(compiler);
  struct Function* fun = compiler.get_function();
  
  PrettyPrinter printer;
  printer.print(*fun, std::cout);
  
  return 0;
}
