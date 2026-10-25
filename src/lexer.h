#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Tokens for the recursive-descent parser. Longest match wins, keywords are
// reserved, comments and whitespace are dropped, and a character that starts
// no token is ERROR.

namespace lex {

enum class Kind {
    END = 0,
    INT,
    MUL,
    DIV,
    PLUS,
    MINUS,
    BROPEN,
    BRCLOSE,
    SQBROPEN,
    SQBRCLOSE,
    CBROPEN,
    CBRCLOSE,
    ASSIGN,
    REL,
    AND,
    OR,
    NOT,
    SEMICOLON,
    COLON,
    COMMA,
    POINT,
    IF,
    ELSE,
    WHILE,
    RETURN,
    GLOBAL,
    FUNCTION,
    BOOLCONST,
    NONECONST,
    NAME,
    STRCONST,
    ERROR,
};

struct Token {
    Kind kind;
    std::string text;
};

class Stream {
   public:
    explicit Stream(std::string source);

    const Token& peek() const;
    void consume();

   private:
    std::vector<Token> tokens_;
    std::size_t index_ = 0;
};

}  // namespace lex
