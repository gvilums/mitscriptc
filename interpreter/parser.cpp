#include <iostream>
#include <cassert>

#include "antlr4-runtime.h"
#include "MITScript.h"

// Helper function for reporting errors
void reportError(antlr4::Token& token)
{
  std::cout <<"Unexpected Symbol (" 
            <<token.getLine() <<"," 
	    <<token.getCharPositionInLine() <<"): "
	    <<token.getText() <<"\n";
}

bool TermPrime(antlr4::CommonTokenStream& tokens);

bool Term(antlr4::CommonTokenStream& tokens)
{
  // Get token at current index in buffer
  antlr4::Token* token =  tokens.get(tokens.index());

  if (token->getType() == MITScript::INT)
  { 
    // Move forward in token buffer
    tokens.consume();

    return TermPrime(tokens);
  }

  // Report a parse error
  reportError(*token);

  return false;
}

bool TermPrime(antlr4::CommonTokenStream&  tokens)
{ 
  antlr4::Token* token = tokens.get(tokens.index());

  switch(token->getType())
  {
    case MITScript::EOF:
    {
      return true;
    }
    case MITScript::MUL:
    case MITScript::DIV:
    {
      tokens.consume();

      antlr4::Token* nextToken = tokens.get(tokens.index());
   
      if (nextToken->getType() == MITScript::INT)
      {
        tokens.consume();

        return TermPrime(tokens);
      }

      reportError(*nextToken);

      return false;
    }
    default:
    {
      reportError(*token);
      return false;
    }
  }
  
  // We shouldn't reach here
  assert(0);

  return false;
}


int main(int argc, const char* argv[]) 
{
  // Create lexer
  antlr4::ANTLRInputStream input(std::cin);
  MITScript lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  // Load all tokens within the file to a buffer
  tokens.fill();
  
  bool result = Term(tokens);

  std::cout <<"Parse status: " <<result <<"\n";

  return (result ? 0 : 1);

  // This is a cartoon of the output pattern
  // you should instead expect for your implementation
#if 0
  // Parse the program, producing a Program AST node
  AST::Program* output = Program(tokens);

  // Create pretty print
  PrettyPrinter printer;

  // Print program
  program->accept(printer);

  // return appropriate return code
#endif

}
