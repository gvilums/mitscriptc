
// Generated from grammar/MITScript.g by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"


namespace lexer {


class  MITScript : public antlr4::Lexer {
public:
  enum {
    INT = 1, MUL = 2, DIV = 3, PLUS = 4, MINUS = 5, COMMENT = 6, WHITESPACE = 7, 
    BROPEN = 8, BRCLOSE = 9, SQBROPEN = 10, SQBRCLOSE = 11, CBROPEN = 12, 
    CBRCLOSE = 13, ASSIGN = 14, REL = 15, AND = 16, OR = 17, NOT = 18, SEMICOLON = 19, 
    COLON = 20, COMMA = 21, POINT = 22, IF = 23, ELSE = 24, WHILE = 25, 
    RETURN = 26, GLOBAL = 27, FUNCTION = 28, BOOLCONST = 29, NONECONST = 30, 
    NAME = 31, STRCONST = 32, ERROR = 33
  };

  explicit MITScript(antlr4::CharStream *input);
  ~MITScript();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

}  // namespace lexer
