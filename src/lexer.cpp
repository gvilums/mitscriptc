#include "lexer.h"

#include <string_view>

namespace lex {
namespace {

bool is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_ident_continue(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f';
}

Kind keyword_kind(std::string_view text) {
    if (text == "if")
        return Kind::IF;
    if (text == "else")
        return Kind::ELSE;
    if (text == "while")
        return Kind::WHILE;
    if (text == "return")
        return Kind::RETURN;
    if (text == "global")
        return Kind::GLOBAL;
    if (text == "fun")
        return Kind::FUNCTION;
    if (text == "true" || text == "false")
        return Kind::BOOLCONST;
    if (text == "None")
        return Kind::NONECONST;
    return Kind::NAME;
}

// Length of a string literal starting at src[i] == '"', including both quotes.
// Zero means the quote does not begin a valid literal, matching the old rule
// STRCONST : '"' (~('"'|'\\') | '\\n' | '\\t' | '\\"' | '\\\\')* '"';
std::size_t string_length(std::string_view src, std::size_t i) {
    std::size_t j = i + 1;
    while (j < src.size()) {
        char c = src[j];
        if (c == '"')
            return (j + 1) - i;
        if (c == '\\') {
            if (j + 1 >= src.size())
                return 0;
            char esc = src[j + 1];
            if (esc == 'n' || esc == 't' || esc == '"' || esc == '\\') {
                j += 2;
                continue;
            }
            return 0;
        }
        ++j;
    }
    return 0;
}

void push(std::vector<Token>& out, Kind kind, std::string_view text) {
    out.push_back(Token{kind, std::string(text)});
}

}  // namespace

Stream::Stream(std::string source) {
    std::string_view src(source);
    std::size_t i = 0;
    while (i < src.size()) {
        char c = src[i];
        if (is_space(c)) {
            ++i;
            continue;
        }
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            i += 2;
            while (i < src.size() && src[i] != '\r' && src[i] != '\n')
                ++i;
            continue;
        }
        if (c == '"') {
            std::size_t n = string_length(src, i);
            if (n == 0) {
                push(tokens_, Kind::ERROR, src.substr(i, 1));
                ++i;
                continue;
            }
            push(tokens_, Kind::STRCONST, src.substr(i, n));
            i += n;
            continue;
        }
        if (is_ident_start(c)) {
            std::size_t j = i + 1;
            while (j < src.size() && is_ident_continue(src[j]))
                ++j;
            std::string_view text = src.substr(i, j - i);
            push(tokens_, keyword_kind(text), text);
            i = j;
            continue;
        }
        if (is_digit(c)) {
            std::size_t j = i + 1;
            while (j < src.size() && is_digit(src[j]))
                ++j;
            push(tokens_, Kind::INT, src.substr(i, j - i));
            i = j;
            continue;
        }
        if (i + 1 < src.size()) {
            char n = src[i + 1];
            if ((c == '<' && n == '=') || (c == '>' && n == '=') || (c == '=' && n == '=')) {
                push(tokens_, Kind::REL, src.substr(i, 2));
                i += 2;
                continue;
            }
        }
        Kind kind = Kind::ERROR;
        switch (c) {
            case '*':
                kind = Kind::MUL;
                break;
            case '/':
                kind = Kind::DIV;
                break;
            case '+':
                kind = Kind::PLUS;
                break;
            case '-':
                kind = Kind::MINUS;
                break;
            case '(':
                kind = Kind::BROPEN;
                break;
            case ')':
                kind = Kind::BRCLOSE;
                break;
            case '[':
                kind = Kind::SQBROPEN;
                break;
            case ']':
                kind = Kind::SQBRCLOSE;
                break;
            case '{':
                kind = Kind::CBROPEN;
                break;
            case '}':
                kind = Kind::CBRCLOSE;
                break;
            case '=':
                kind = Kind::ASSIGN;
                break;
            case '<':
            case '>':
                kind = Kind::REL;
                break;
            case '&':
                kind = Kind::AND;
                break;
            case '|':
                kind = Kind::OR;
                break;
            case '!':
                kind = Kind::NOT;
                break;
            case ';':
                kind = Kind::SEMICOLON;
                break;
            case ':':
                kind = Kind::COLON;
                break;
            case ',':
                kind = Kind::COMMA;
                break;
            case '.':
                kind = Kind::POINT;
                break;
            default:
                break;
        }
        push(tokens_, kind, src.substr(i, 1));
        ++i;
    }
    tokens_.push_back(Token{Kind::END, {}});
}

const Token& Stream::peek() const {
    return tokens_[index_];
}

void Stream::consume() {
    if (index_ + 1 < tokens_.size())
        ++index_;
}

}  // namespace lex
