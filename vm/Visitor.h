#pragma once

#include <memory>

using namespace std;

// You will need to a virtual visitor class with a
// visit method for each different type of expression and statement
// as defined in AST.h

namespace AST {
class Program;
class Statement;
class Block;
class BinaryExpression;
class UnaryExpression;
class FunctionDeclaration;
class Call;
class FieldDereference;
class IndexExpression;
class Record;
class IntegerConstant;
class StringConstant;
class BoolConstant;
class NoneConstant;
class Global;
class Return;
class Assignment;
class IfStatement;
class WhileLoop;
}  // namespace AST

class Visitor {
   public:
    // For each AST node, you need a virtual method of the form
    virtual void visit(AST::Program& expr) = 0;
    virtual void visit(AST::Block& expr) = 0;
    virtual void visit(AST::BinaryExpression& expr) = 0;
    virtual void visit(AST::UnaryExpression& expr) = 0;
    virtual void visit(AST::FunctionDeclaration& expr) = 0;
    virtual void visit(AST::Call& expr) = 0;
    virtual void visit(AST::FieldDereference& expr) = 0;
    virtual void visit(AST::IndexExpression& expr) = 0;
    virtual void visit(AST::Record& expr) = 0;
    virtual void visit(AST::IntegerConstant& expr) = 0;
    virtual void visit(AST::StringConstant& expr) = 0;
    virtual void visit(AST::BoolConstant& expr) = 0;
    virtual void visit(AST::NoneConstant& expr) = 0;
    virtual void visit(AST::Global& expr) = 0;
    virtual void visit(AST::Return& expr) = 0;
    virtual void visit(AST::Assignment& expr) = 0;
    virtual void visit(AST::IfStatement& expr) = 0;
    virtual void visit(AST::WhileLoop& expr) = 0;
};
