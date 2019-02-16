#include <iostream>
#include <cassert>

#include "antlr4-runtime.h"
#include "MITScript.h"

/**
 * Term -> INT TermPrime
 * TermPrime -> * INT TermPrime
 * TermPrime -> / INT TermPrime
 * TermPrime -> \epsilon
 * */

class TermPrime
{
public:

  virtual std::string to_string() = 0;

};


//  Term -> INT TermPrime
class Term
{
public:

  Term(int32_t value, TermPrime* rest)
  : value_(value),
    rest_(rest)
  {

  }

  int32_t value_;
  TermPrime* rest_;

    
  std::string to_string()
  {
    std::stringstream ss;
    ss <<"Term(" << value_ <<"," <<rest_->to_string() <<")";    
    return ss.str();
  }

};

// TermPrime -> * INT TermPrime
class TermPrimeMul : public TermPrime
{

public:

  TermPrimeMul(int32_t value, TermPrime* rest) 
  : value_(value), rest_(rest)
  {

  }
  
  std::string to_string() override
  {
    std::stringstream ss;
    ss <<"TermPrimeMul(" << value_ <<"," <<rest_->to_string() <<")";    
  
    return ss.str();
  }


  int32_t value_;
  TermPrime*  rest_;
};

// TermPrime -> / INT TermPrime
class TermPrimeDiv : public TermPrime
{
public:

  TermPrimeDiv(int32_t value, TermPrime* rest) 
  : value_(value), rest_(rest)
  {

  }

  std::string to_string() override
  {
    std::stringstream ss;
    ss <<"TermPrimeDiv(" << value_ <<"," <<rest_->to_string() <<")";    
  
    return ss.str();
  }

  int32_t value_;
  TermPrime* rest_;
};

// TermPrime -> \epsilon
class TermPrimeEps : public TermPrime
{
public:
  std::string to_string() override
  {
    return "TermPrimeEps";    
  }
};

// Helper function for reporting errors
void reportError(antlr4::Token& token)
{
  std::cout <<"Unexpected Symbol (" 
            <<token.getLine() <<"," 
	    <<token.getCharPositionInLine() <<"): "
	    <<token.getText() <<"\n";
}

TermPrime* ParseTermPrime(antlr4::CommonTokenStream& tokens);

Term* ParseTerm(antlr4::CommonTokenStream& tokens)
{
  // Get token at current index in buffer
  antlr4::Token* token =  tokens.get(tokens.index());

  if (token->getType() == MITScript::INT)
  { 
    int32_t value = std::stoi(token->getText());
    
    // Move forward in token buffer
    tokens.consume();

    auto* rest = ParseTermPrime(tokens);

    if (rest == nullptr) { return nullptr; }
    
    return new Term(value, rest);
  }

  // Report a parse error
  reportError(*token);

  return nullptr;
}

TermPrime* ParseTermPrime(antlr4::CommonTokenStream&  tokens)
{ 
  antlr4::Token* token = tokens.get(tokens.index());

  switch(token->getType())
  {
    case MITScript::MUL:
    case MITScript::DIV:
    {
      tokens.consume();

      antlr4::Token* nextToken = tokens.get(tokens.index());
   
      if (nextToken->getType() == MITScript::INT)
      {

        int32_t value = std::stoi(nextToken->getText());
    
        tokens.consume();

        auto* rest = ParseTermPrime(tokens);

        if (rest == nullptr) { return nullptr; }

        switch(token->getType())
        {
          case MITScript::MUL: return new TermPrimeMul(value, rest);
          case MITScript::DIV: return new TermPrimeDiv(value, rest);
          default:
          {
            assert (0);
          }
        }
      }

      reportError(*nextToken);

      return nullptr;
    }
    case MITScript::EOF:
    {
      return new TermPrimeEps();
    }
    default:
    {
      reportError(*token);

      return nullptr;
    }
  }
  
  // We shouldn't reach here
  assert(0);

  return nullptr;
}

int main(int argc, const char* argv[]) 
{
  // Create lexer
  antlr4::ANTLRInputStream input(std::cin);
  MITScript lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  // Load all tokens within the file to a buffer
  tokens.fill();
  
  // Print source as text
  std::cout <<"source: " <<tokens.getText() <<"\n";

  // Print the tokens
  std::cout <<"tokens: ";

  const auto& vocab = lexer.getVocabulary();
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (i != 0)
    {
      std::cout <<" ";
    }

    const auto token = tokens.get(i);
    std::cout <<vocab.getSymbolicName(token->getType());
  }

  std::cout <<"\n";

  Term* result = ParseTerm(tokens);

  bool success = result != nullptr;

  if (success)
  {
    std::cout <<"CST:    " <<result->to_string() <<"\n";
  }
  else
  {
    std::cout <<"Parse Failed\n";
  }
  
  return (success ? 0 : 1);
}
