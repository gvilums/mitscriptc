#pragma once

#include <algorithm>
#include <set>
#include "AST.h"
#include "Assigns.h"
#include "FreeVariables.h"
#include "ir.h"
#include "irprinter.h"

using namespace std;

class Compiler : public Visitor {
   private:
    bool global_scope_;
  
    IR::Program* program_;
    IR::Function* fun_;
    IR::BasicBlock block_;
    IR::Operand opr_;
    bool is_opr_;
    int reg_cnt_;
    int ret_reg_;
    
    std::map<std::string, int> local_vars_;
    std::set<std::string> local_reference_vars_;
    std::map<std::string, int> free_vars_;
    std::map<std::string, int> names_;
    
    std::set<std::string> globals_; 
    std::set<std::string> ref_;
    
    int imm_cnt_;
    size_t names_cnt_;
    std::map<string, int> str_const_;
    std::map<int, int> int_const_;
    
   public:
    Compiler();
    IR::Program* get_program();
    void visit(AST::Program& expr);
    void visit(AST::Block& expr);
    void visit(AST::Global& expr);
    void visit(AST::Return& expr);
    void visit(AST::Assignment& expr);
    void visit(AST::IfStatement& expr);
    void visit(AST::WhileLoop& expr);
    void visit(AST::FunctionDeclaration& expr);
    void visit(AST::BinaryExpression& expr);
    void visit(AST::UnaryExpression& expr);
    void visit(AST::Call& expr);
    void visit(AST::FieldDereference& expr);
    void visit(AST::IndexExpression& expr);
    void visit(AST::Record& expr);
    void visit(AST::IntegerConstant& expr);
    void visit(AST::StringConstant& expr);
    void visit(AST::BoolConstant& expr);
    void visit(AST::NoneConstant& expr);
};
