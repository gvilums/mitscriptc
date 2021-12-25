#pragma once

#include "AST.h"
#include "antlr4-runtime.h"
#include "MITScript.h"

AST::Program* Program(antlr4::CommonTokenStream& tokens);
AST::Statement* Statement(antlr4::CommonTokenStream& tokens);
AST::Global* Global(antlr4::CommonTokenStream& tokens);
AST::Assignment* Assignment(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs);
AST::Statement* CallStatement(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs);
AST::Block* Block(antlr4::CommonTokenStream& tokens);
AST::IfStatement* IfStatement(antlr4::CommonTokenStream& tokens);
AST::WhileLoop* WhileLoop(antlr4::CommonTokenStream& tokens);
AST::Return* Return(antlr4::CommonTokenStream& tokens);

AST::Expression* Expression(antlr4::CommonTokenStream& tokens);
AST::FunctionDeclaration* Function(antlr4::CommonTokenStream& tokens);
AST::Expression* Boolean(antlr4::CommonTokenStream& tokens);
AST::Expression* Conjunction(antlr4::CommonTokenStream& tokens);
AST::Expression* BoolUnit(antlr4::CommonTokenStream& tokens);
AST::Expression* Predicate(antlr4::CommonTokenStream& tokens);
AST::Expression* Arithmetic(antlr4::CommonTokenStream& tokens);
AST::Expression* Product(antlr4::CommonTokenStream& tokens);
AST::Expression* Unit(antlr4::CommonTokenStream& tokens);
AST::Expression* LHS(antlr4::CommonTokenStream& tokens);
AST::Call* Call(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs);
AST::Record* Record(antlr4::CommonTokenStream& tokens);

AST::Program* parse(std::ifstream& file);