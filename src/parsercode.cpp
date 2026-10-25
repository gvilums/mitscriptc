#include "parsercode.h"

#include "lexer.h"
#include "utils.h"

#include <iterator>
#include <string>

static AST::Program* Program(lex::Stream& tokens);
static AST::Statement* Statement(lex::Stream& tokens);
static AST::Global* Global(lex::Stream& tokens);
static AST::Assignment* Assignment(lex::Stream& tokens, AST::Expression* Lhs);
static AST::Statement* CallStatement(lex::Stream& tokens, AST::Expression* Lhs);
static AST::Block* Block(lex::Stream& tokens);
static AST::IfStatement* IfStatement(lex::Stream& tokens);
static AST::WhileLoop* WhileLoop(lex::Stream& tokens);
static AST::Return* Return(lex::Stream& tokens);
static AST::Expression* Expression(lex::Stream& tokens);
static AST::FunctionDeclaration* Function(lex::Stream& tokens);
static AST::Expression* Boolean(lex::Stream& tokens);
static AST::Expression* Conjunction(lex::Stream& tokens);
static AST::Expression* BoolUnit(lex::Stream& tokens);
static AST::Expression* Predicate(lex::Stream& tokens);
static AST::Expression* Arithmetic(lex::Stream& tokens);
static AST::Expression* Product(lex::Stream& tokens);
static AST::Expression* Unit(lex::Stream& tokens);
static AST::Expression* LHS(lex::Stream& tokens);
static AST::Call* Call(lex::Stream& tokens, AST::Expression* Lhs);
static AST::Record* Record(lex::Stream& tokens);


#define check(x)                         \
    {                                    \
        token = &tokens.peek();          \
        if (token->kind != lex::Kind::x) \
            return NULL;                 \
        tokens.consume();                \
    }


AST::Program* parse(std::ifstream& file) {
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    lex::Stream tokens(std::move(source));
    return Program(tokens);
}

static AST::Program* Program(lex::Stream& tokens) {
    AST::Program* Prog = new AST::Program;
    const lex::Token* token = &tokens.peek();
    while (token->kind != lex::Kind::END) {
        AST::Statement* Stat = Statement(tokens);
        if (!Stat)
            return NULL;
        Prog->addChild(Stat);
        token = &tokens.peek();
    }
    return Prog;
}

static AST::Statement* Statement(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    AST::Statement* stat;
    switch (token->kind) {
        case lex::Kind::GLOBAL:
            stat = (AST::Statement*)Global(tokens);
            break;
        case lex::Kind::IF:
            stat = (AST::Statement*)IfStatement(tokens);
            break;
        case lex::Kind::WHILE:
            stat = (AST::Statement*)WhileLoop(tokens);
            break;
        case lex::Kind::RETURN:
            stat = (AST::Statement*)Return(tokens);
            break;
        default:
            AST::Expression* Lhs = LHS(tokens);
            if (!Lhs)
                return NULL;

            token = &tokens.peek();

            if (token->kind == lex::Kind::BROPEN)
                stat = (AST::Statement*)CallStatement(tokens, Lhs);
            else
                stat = (AST::Statement*)Assignment(tokens, Lhs);
            break;
    }
    if (!stat)
        return NULL;
    return stat;
}

static AST::Block* Block(lex::Stream& tokens) {
    AST::Block* Blk = new AST::Block;
    const lex::Token* token = &tokens.peek();
    check(CBROPEN);
    token = &tokens.peek();
    while (token->kind != lex::Kind::CBRCLOSE) {
        AST::Statement* Stat = Statement(tokens);
        if (!Stat)
            return NULL;
        Blk->addChild(Stat);
        token = &tokens.peek();
    }
    tokens.consume();
    return Blk;
}

static AST::Assignment* Assignment(lex::Stream& tokens, AST::Expression* Lhs) {
    const lex::Token* token = &tokens.peek();
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

static AST::Statement* CallStatement(lex::Stream& tokens, AST::Expression* Lhs) {
    const lex::Token* token = &tokens.peek();
    AST::Call* Cl = Call(tokens, Lhs);

    if (!Cl)
        return NULL;
    Cl->setStatement();
    check(SEMICOLON);
    return (AST::Statement*)Cl;
}

static AST::Global* Global(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    AST::Global* Glob = new AST::Global;
    check(GLOBAL);

    token = &tokens.peek();
    if (token->kind != lex::Kind::NAME)
        return NULL;
    Glob->addName(token->text);
    tokens.consume();

    check(SEMICOLON);
    return Glob;
}

static AST::IfStatement* IfStatement(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
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
    token = &tokens.peek();
    if (token->kind != lex::Kind::ELSE)
        return IfStat;

    check(ELSE);
    AST::Block* Block2 = Block(tokens);
    if (!Block2)
        return NULL;
    IfStat->addChild(Block2);
    return IfStat;
}

static AST::WhileLoop* WhileLoop(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
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

static AST::Return* Return(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    check(RETURN);
    AST::Expression* Expr = Expression(tokens);
    if (!Expr)
        return NULL;
    token = &tokens.peek();
    check(SEMICOLON)
        AST::Return* ReturnSt = new AST::Return;
    ReturnSt->addExpr(Expr);
    return ReturnSt;
}

static AST::Expression* Expression(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    AST::Expression* expr;
    switch (token->kind) {
        case lex::Kind::FUNCTION:
            expr = (AST::Expression*)Function(tokens);
            break;
        case lex::Kind::CBROPEN:
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

static AST::FunctionDeclaration* Function(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    check(FUNCTION);
    check(BROPEN);
    AST::FunctionDeclaration* FunDec = new AST::FunctionDeclaration();
    token = &tokens.peek();
    if (token->kind == lex::Kind::NAME) {
        FunDec->addArg(token->text);
        tokens.consume();
        token = &tokens.peek();
        while (token->kind == lex::Kind::COMMA) {
            check(COMMA);

            token = &tokens.peek();
            if (token->kind != lex::Kind::NAME)
                return NULL;
            FunDec->addArg(token->text);
            tokens.consume();

            token = &tokens.peek();
        }
    }
    check(BRCLOSE);
    AST::Block* Blk = Block(tokens);
    if (!Blk)
        return NULL;
    FunDec->addBody(Blk);
    return FunDec;
}

static AST::Expression* Boolean(lex::Stream& tokens) {
    AST::Expression* Conj = Conjunction(tokens);
    AST::Expression* Temp;
    if (!Conj)
        return NULL;

    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();
    while (token->kind == lex::Kind::OR) {
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
        token = &tokens.peek();
    }

    return Conj;
}

static AST::Expression* Conjunction(lex::Stream& tokens) {
    AST::Expression* BUnit = BoolUnit(tokens);
    AST::Expression* Temp;
    if (!BUnit)
        return NULL;

    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();
    while (token->kind == lex::Kind::AND) {
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
        token = &tokens.peek();
    }

    return BUnit;
}

static AST::Expression* BoolUnit(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();

    bool nt = false;
    if (token->kind == lex::Kind::NOT) {
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

static AST::Expression* Predicate(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    AST::Expression* Arith = Arithmetic(tokens);

    if (!Arith)
        return NULL;

    token = &tokens.peek();

    if (token->kind != lex::Kind::REL)
        return Arith;
    string op = token->text;
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

static AST::Expression* Arithmetic(lex::Stream& tokens) {
    AST::Expression* Prod = Product(tokens);
    AST::Expression* Temp;
    if (!Prod)
        return NULL;

    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();
    while (token->kind == lex::Kind::PLUS || token->kind == lex::Kind::MINUS) {
        string op = token->text;
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
        token = &tokens.peek();
    }

    return Prod;
}

static AST::Expression* Product(lex::Stream& tokens) {
    AST::Expression* Un = Unit(tokens);
    AST::Expression* Temp;
    if (!Un)
        return NULL;

    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();
    while (token->kind == lex::Kind::DIV || token->kind == lex::Kind::MUL) {
        string op = token->text;
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
        token = &tokens.peek();
    }

    return Un;
}

static AST::Expression* Unit(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();

    bool minus = false;
    if (token->kind == lex::Kind::MINUS) {
        check(MINUS);
        minus = true;
    }

    AST::Expression* Expr;
    token = &tokens.peek();

    AST::BoolConstant* Con;
    AST::NoneConstant* Con1;
    AST::IntegerConstant* Con2;
    AST::StringConstant* Con3;

    switch (token->kind) {
        case lex::Kind::BOOLCONST:
            Con = new AST::BoolConstant();
            Con->addVal(token->text);
            tokens.consume();
            Expr = (AST::Expression*)Con;
            break;
        case lex::Kind::NONECONST:
            Con1 = new AST::NoneConstant();
            tokens.consume();
            Expr = (AST::Expression*)Con1;
            break;
        case lex::Kind::INT:
            Con2 = new AST::IntegerConstant();
            Con2->addVal(token->text);
            tokens.consume();
            Expr = (AST::Expression*)Con2;
            break;
        case lex::Kind::STRCONST:
            Con3 = new AST::StringConstant();
            Con3->addVal(utils::escape(token->text));
            tokens.consume();
            Expr = (AST::Expression*)Con3;
            break;
        case lex::Kind::BROPEN:
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

            token = &tokens.peek();
            if (token->kind != lex::Kind::BROPEN) {
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
    else {
    	AST::UnaryExpression* UnExpr = new AST::UnaryExpression();
    	UnExpr->addChild(Expr);
    	UnExpr->addOp("-");

    	return (AST::Expression*)UnExpr;
    }
}

static AST::Expression* LHS(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    token = &tokens.peek();
    if (token->kind != lex::Kind::NAME)
        return NULL;
    AST::StringConstant* Con = new AST::StringConstant();
    Con->addVal(token->text);
    AST::Expression* BaseExpression = (AST::Expression*)Con;
    tokens.consume();

    token = &tokens.peek();
    while (token->kind == lex::Kind::SQBROPEN || token->kind == lex::Kind::POINT) {
        if (token->kind == lex::Kind::SQBROPEN) {
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
        if (token->kind == lex::Kind::POINT) {
            check(POINT);
            token = &tokens.peek();
            if (token->kind != lex::Kind::NAME)
                return NULL;
            AST::FieldDereference* Fdef = new AST::FieldDereference();
            Fdef->addBaseexpr(BaseExpression);
            Fdef->addField(token->text);
            tokens.consume();
            BaseExpression = (AST::Expression*)Fdef;
        }
        token = &tokens.peek();
    }
    return BaseExpression;
}

static AST::Record* Record(lex::Stream& tokens) {
    const lex::Token* token = &tokens.peek();
    check(CBROPEN);

    AST::Record* Rec = new AST::Record();
    token = &tokens.peek();
    while (token->kind == lex::Kind::NAME) {
        string name = token->text;
        tokens.consume();
        check(COLON);
        AST::Expression* Expr = Expression(tokens);
        if (!Expr)
            return NULL;
        check(SEMICOLON);
        Rec->addMap(name, Expr);
        token = &tokens.peek();
    }
    check(CBRCLOSE);
    return Rec;
}

static AST::Call* Call(lex::Stream& tokens, AST::Expression* Lhs) {
    const lex::Token* token = &tokens.peek();

    AST::Call* Cl = new AST::Call();
    Cl->addExpr(Lhs);
    check(BROPEN);

    token = &tokens.peek();
    if (token->kind != lex::Kind::BRCLOSE) {
        AST::Expression* Arg = Expression(tokens);
        if (!Arg)
            return NULL;
        Cl->addArg(Arg);
        token = &tokens.peek();
        while (token->kind == lex::Kind::COMMA) {
            check(COMMA);
            Arg = Expression(tokens);
            if (!Arg)
                return NULL;
            Cl->addArg(Arg);
            token = &tokens.peek();
        }
    }
    
    check(BRCLOSE);
    return Cl;
}
