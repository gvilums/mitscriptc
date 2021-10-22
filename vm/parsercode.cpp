#include "AST.h"
#include "MITScript.h"
#include "antlr4-runtime.h"
#include "parsercode.h"

#define check(x)                              \
    {                                         \
        token = tokens.get(tokens.index());   \
        if (token->getType() != MITScript::x) \
            return NULL;                      \
        tokens.consume();                     \
    }


AST::Program* parse(std::ifstream& file) {
    antlr4::ANTLRInputStream input(file);
    MITScript lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    tokens.fill();

    return Program(tokens);
}

AST::Program* Program(antlr4::CommonTokenStream& tokens) {
    AST::Program* Prog = new AST::Program;
    antlr4::Token* token = tokens.get(tokens.index());
    while (token->getType() != MITScript::EOF) {
        AST::Statement* Stat = Statement(tokens);
        if (!Stat)
            return NULL;
        Prog->addChild(Stat);
        token = tokens.get(tokens.index());
    }
    return Prog;
}

AST::Statement* Statement(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Statement* stat;
    switch (token->getType()) {
        case MITScript::GLOBAL:
            stat = (AST::Statement*)Global(tokens);
            break;
        case MITScript::IF:
            stat = (AST::Statement*)IfStatement(tokens);
            break;
        case MITScript::WHILE:
            stat = (AST::Statement*)WhileLoop(tokens);
            break;
        case MITScript::RETURN:
            stat = (AST::Statement*)Return(tokens);
            break;
        default:
            AST::Expression* Lhs = LHS(tokens);
            if (!Lhs)
                return NULL;

            token = tokens.get(tokens.index());

            if (token->getType() == MITScript::BROPEN)
                stat = (AST::Statement*)CallStatement(tokens, Lhs);
            else
                stat = (AST::Statement*)Assignment(tokens, Lhs);
            break;
    }
    if (!stat)
        return NULL;
    return stat;
}

AST::Block* Block(antlr4::CommonTokenStream& tokens) {
    AST::Block* Blk = new AST::Block;
    antlr4::Token* token = tokens.get(tokens.index());
    check(CBROPEN);
    token = tokens.get(tokens.index());
    while (token->getType() != MITScript::CBRCLOSE) {
        AST::Statement* Stat = Statement(tokens);
        if (!Stat)
            return NULL;
        Blk->addChild(Stat);
        token = tokens.get(tokens.index());
    }
    tokens.consume();
    return Blk;
}

AST::Assignment* Assignment(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Assignment* Assg = new AST::Assignment;

    check(ASSIGN);

    AST::Expression* Expr = Expression(tokens);
    if (!Expr)
        return NULL;

    Assg->addLhs(Lhs);
    Assg->addExpr(Expr);
    check(SEMICOLON);
    return Assg;
}

AST::Statement* CallStatement(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Call* Cl = Call(tokens, Lhs);
    Cl->setStatement();

    if (!Cl)
        return NULL;
    check(SEMICOLON);
    return (AST::Statement*)Cl;
}

AST::Global* Global(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Global* Glob = new AST::Global;
    check(GLOBAL);

    token = tokens.get(tokens.index());
    if (token->getType() != MITScript::NAME)
        return NULL;
    Glob->addName(token->getText());
    tokens.consume();

    check(SEMICOLON);
    return Glob;
}

AST::IfStatement* IfStatement(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::IfStatement* IfStat = new AST::IfStatement;
    check(IF);
    check(BROPEN);
    AST::Expression* Expr = Expression(tokens);
    if (!Expr)
        return NULL;
    check(BRCLOSE);
    IfStat->addExpr(Expr);

    AST::Block* Block1 = Block(tokens);
    if (!Block1)
        return NULL;
    IfStat->addChild(Block1);
    token = tokens.get(tokens.index());
    if (token->getType() != MITScript::ELSE)
        return IfStat;

    check(ELSE);
    AST::Block* Block2 = Block(tokens);
    if (!Block2)
        return NULL;
    IfStat->addChild(Block2);
    return IfStat;
}

AST::WhileLoop* WhileLoop(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::WhileLoop* WhileL = new AST::WhileLoop;
    check(WHILE);
    check(BROPEN);
    AST::Expression* Expr = Expression(tokens);
    if (!Expr)
        return NULL;
    check(BRCLOSE);
    WhileL->addExpr(Expr);

    AST::Block* Block1 = Block(tokens);
    if (!Block1)
        return NULL;
    WhileL->addChild(Block1);

    return WhileL;
}

AST::Return* Return(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    check(RETURN);
    AST::Expression* Expr = Expression(tokens);
    if (!Expr)
        return NULL;
    token = tokens.get(tokens.index());
    check(SEMICOLON)
        AST::Return* ReturnSt = new AST::Return;
    ReturnSt->addExpr(Expr);
    return ReturnSt;
}

AST::Expression* Expression(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Expression* expr;
    switch (token->getType()) {
        case MITScript::FUNCTION:
            expr = (AST::Expression*)Function(tokens);
            break;
        case MITScript::CBROPEN:
            expr = (AST::Expression*)Record(tokens);
            break;
        default:
            expr = Boolean(tokens);
            break;
    }
    if (!expr)
        return NULL;
    return expr;
}

AST::FunctionDeclaration* Function(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    check(FUNCTION);
    check(BROPEN);
    AST::FunctionDeclaration* FunDec = new AST::FunctionDeclaration();
    token = tokens.get(tokens.index());
    if (token->getType() == MITScript::NAME) {
        FunDec->addArg(token->getText());
        tokens.consume();
        token = tokens.get(tokens.index());
        while (token->getType() == MITScript::COMMA) {
            check(COMMA);

            token = tokens.get(tokens.index());
            if (token->getType() != MITScript::NAME)
                return NULL;
            FunDec->addArg(token->getText());
            tokens.consume();

            token = tokens.get(tokens.index());
        }
    }
    check(BRCLOSE);
    AST::Block* Blk = Block(tokens);
    if (!Blk)
        return NULL;
    FunDec->addBody(Blk);
    return FunDec;
}

AST::Expression* Boolean(antlr4::CommonTokenStream& tokens) {
    AST::Expression* Conj = Conjunction(tokens);
    AST::Expression* Temp;
    if (!Conj)
        return NULL;

    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::OR) {
        check(OR);
        Temp = Conj;
        Conj = Conjunction(tokens);
        if (!Conj)
            return NULL;

        AST::BinaryExpression* BinExp = new AST::BinaryExpression();
        BinExp->addChild(Temp);
        BinExp->addChild(Conj);
        BinExp->addOp("|");

        Conj = (AST::Expression*)BinExp;
        token = tokens.get(tokens.index());
    }

    return Conj;
}

AST::Expression* Conjunction(antlr4::CommonTokenStream& tokens) {
    AST::Expression* BUnit = BoolUnit(tokens);
    AST::Expression* Temp;
    if (!BUnit)
        return NULL;

    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::AND) {
        check(AND);
        Temp = BUnit;
        BUnit = BoolUnit(tokens);
        if (!BUnit)
            return NULL;

        AST::BinaryExpression* BinExp = new AST::BinaryExpression();
        BinExp->addChild(Temp);
        BinExp->addChild(BUnit);
        BinExp->addOp("&");

        BUnit = (AST::Expression*)BinExp;
        token = tokens.get(tokens.index());
    }

    return BUnit;
}

AST::Expression* BoolUnit(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());

    bool nt = false;
    if (token->getType() == MITScript::NOT) {
        check(NOT);
        nt = true;
    }

    AST::Expression* Pred = Predicate(tokens);
    if (!Pred)
        return NULL;
    if (!nt)
        return Pred;

    AST::UnaryExpression* UnExpr = new AST::UnaryExpression();
    UnExpr->addChild(Pred);
    UnExpr->addOp("!");

    return (AST::Expression*)UnExpr;
}

AST::Expression* Predicate(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    AST::Expression* Arith = Arithmetic(tokens);

    if (!Arith)
        return NULL;

    token = tokens.get(tokens.index());

    if (token->getType() != MITScript::REL)
        return Arith;
    string op = token->getText();
    tokens.consume();

    AST::Expression* Arith2 = Arithmetic(tokens);
    if (!Arith2)
        return NULL;

    AST::BinaryExpression* BinExp = new AST::BinaryExpression();
    BinExp->addChild(Arith);
    BinExp->addChild(Arith2);
    BinExp->addOp(op);
    return (AST::Expression*)BinExp;
}

AST::Expression* Arithmetic(antlr4::CommonTokenStream& tokens) {
    AST::Expression* Prod = Product(tokens);
    AST::Expression* Temp;
    if (!Prod)
        return NULL;

    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::PLUS || token->getType() == MITScript::MINUS) {
        string op = token->getText();
        tokens.consume();

        Temp = Prod;
        Prod = Product(tokens);
        if (!Prod)
            return NULL;

        AST::BinaryExpression* BinExp = new AST::BinaryExpression();
        BinExp->addChild(Temp);
        BinExp->addChild(Prod);
        BinExp->addOp(op);

        Prod = (AST::Expression*)BinExp;
        token = tokens.get(tokens.index());
    }

    return Prod;
}

AST::Expression* Product(antlr4::CommonTokenStream& tokens) {
    AST::Expression* Un = Unit(tokens);
    AST::Expression* Temp;
    if (!Un)
        return NULL;

    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::DIV || token->getType() == MITScript::MUL) {
        string op = token->getText();
        tokens.consume();

        Temp = Un;
        Un = Unit(tokens);
        if (!Un)
            return NULL;

        AST::BinaryExpression* BinExp = new AST::BinaryExpression();
        BinExp->addChild(Temp);
        BinExp->addChild(Un);
        BinExp->addOp(op);

        Un = (AST::Expression*)BinExp;
        token = tokens.get(tokens.index());
    }

    return Un;
}

AST::Expression* Unit(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());

    bool minus = false;
    if (token->getType() == MITScript::MINUS) {
        check(MINUS);
        minus = true;
    }

    AST::Expression* Expr;
    token = tokens.get(tokens.index());

    AST::BoolConstant* Con;
    AST::NoneConstant* Con1;
    AST::IntegerConstant* Con2;
    AST::StringConstant* Con3;

    switch (token->getType()) {
        case MITScript::BOOLCONST:
            Con = new AST::BoolConstant();
            Con->addVal(token->getText());
            tokens.consume();
            Expr = (AST::Expression*)Con;
            break;
        case MITScript::NONECONST:
            Con1 = new AST::NoneConstant();
            tokens.consume();
            Expr = (AST::Expression*)Con1;
            break;
        case MITScript::INT:
            Con2 = new AST::IntegerConstant();
            Con2->addVal(token->getText());
            tokens.consume();
            Expr = (AST::Expression*)Con2;
            break;
        case MITScript::STRCONST:
            Con3 = new AST::StringConstant();
            Con3->addVal(token->getText());
            tokens.consume();
            Expr = (AST::Expression*)Con3;
            break;
        case MITScript::BROPEN:
            check(BROPEN);
            Expr = Boolean(tokens);
            if (!Expr)
                return NULL;
            check(BRCLOSE);
            break;
        default:
            AST::Expression* Lhs = LHS(tokens);
            if (!Lhs)
                return NULL;

            token = tokens.get(tokens.index());
            if (token->getType() != MITScript::BROPEN) {
                Expr = Lhs;
                break;
            }

            Expr = (AST::Expression*)Call(tokens, Lhs);
            break;
    }

    if (!Expr)
        return NULL;
    if (!minus)
        return Expr;

    AST::UnaryExpression* UnExpr = new AST::UnaryExpression();
    UnExpr->addChild(Expr);
    UnExpr->addOp("-");

    return (AST::Expression*)UnExpr;
}

AST::Expression* LHS(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    token = tokens.get(tokens.index());
    if (token->getType() != MITScript::NAME)
        return NULL;
    AST::StringConstant* Con = new AST::StringConstant();
    Con->addVal(token->getText());
    AST::Expression* BaseExpression = (AST::Expression*)Con;
    tokens.consume();

    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::SQBROPEN || token->getType() == MITScript::POINT) {
        if (token->getType() == MITScript::SQBROPEN) {
            check(SQBROPEN);
            AST::Expression* Index = Expression(tokens);
            if (!Index)
                return NULL;
            AST::IndexExpression* IExpr = new AST::IndexExpression();
            IExpr->addIndex(Index);
            IExpr->addBaseexpr(BaseExpression);
            BaseExpression = (AST::Expression*)IExpr;
            check(SQBRCLOSE);
        }
        if (token->getType() == MITScript::POINT) {
            check(POINT);
            token = tokens.get(tokens.index());
            if (token->getType() != MITScript::NAME)
                return NULL;
            AST::FieldDereference* Fdef = new AST::FieldDereference();
            Fdef->addBaseexpr(BaseExpression);
            Fdef->addField(token->getText());
            tokens.consume();
            BaseExpression = (AST::Expression*)Fdef;
        }
        token = tokens.get(tokens.index());
    }
    return BaseExpression;
}

AST::Record* Record(antlr4::CommonTokenStream& tokens) {
    antlr4::Token* token = tokens.get(tokens.index());
    check(CBROPEN);

    AST::Record* Rec = new AST::Record();
    token = tokens.get(tokens.index());
    while (token->getType() == MITScript::NAME) {
        string name = token->getText();
        tokens.consume();
        check(COLON);
        AST::Expression* Expr = Expression(tokens);
        if (!Expr)
            return NULL;
        check(SEMICOLON);
        Rec->addMap(name, Expr);
        token = tokens.get(tokens.index());
    }
    check(CBRCLOSE);
    return Rec;
}

AST::Call* Call(antlr4::CommonTokenStream& tokens, AST::Expression* Lhs) {
    antlr4::Token* token = tokens.get(tokens.index());

    AST::Call* Cl = new AST::Call();
    Cl->addExpr(Lhs);

    check(BROPEN);

    token = tokens.get(tokens.index());
    if (token->getType() != MITScript::BRCLOSE) {
        AST::Expression* Arg = Expression(tokens);

        if (!Arg)
            return NULL;
        Cl->addArg(Arg);
        token = tokens.get(tokens.index());
        while (token->getType() == MITScript::COMMA) {
            check(COMMA);
            Arg = Expression(tokens);
            if (!Arg)
                return NULL;
            Cl->addArg(Arg);
            token = tokens.get(tokens.index());
        }
    }
    check(BRCLOSE);
    return Cl;
}
