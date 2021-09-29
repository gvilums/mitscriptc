#include <iostream>
#include <cassert>

#include "antlr4-runtime.h"
#include "parsercode.cpp"

int main(int argc, const char* argv[]) 
{
  // Create lexer
  antlr4::ANTLRInputStream input(std::cin);
  MITScript lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  // Load all tokens within the file to a buffer
  tokens.fill();
  
  /*antlr4::Token* token = tokens.get(tokens.index());
  while(token->getType() != MITScript::EOF){
  	std::cout << token->getText() << std::endl;
  	tokens.consume();
  	token = tokens.get(tokens.index());
  }
  return 0;*/
  
  AST::Program* output = Program(tokens);
  if(!output)
  	return 1;
  
  PrettyPrinter printer;
  output->accept(printer);
  
  std::cout << printer.getResult() << std::endl;
  return 0;
}
