#include <iostream>
#include <cassert>

#include "antlr4-runtime.h"
#include "MITScript.h"
#include "AST.h"
#include "Interpreter.h"
#include "parsercode.cpp"
#include <exception>

int main(int argc, const char *argv[])
{
  // We will now have the interpreter take the input
  // program from a file
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

  // Create lexer
  antlr4::ANTLRInputStream input(file);
  MITScript lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  tokens.fill();

  AST::Program* program = Program(tokens);
  if(!program){
  	std::cout << "Parsing failed\n";
  	return 1;
  }
  	
  Interpreter interpreter;
  
  try {
  	program->accept(interpreter);
  } catch(exception& e){
  	cout << e.what() << endl;
  	return 1;
  }
 
  
  return 0;
}
