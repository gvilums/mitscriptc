lexer grammar MITScript;
 
// Lexer Rules

INT : [0-9]+ ;
MUL : '*' ;
DIV : '/' ;
PLUS : '+' ;
MINUS : '-' ;
COMMENT: '//'~( '\r' | '\n' )* -> skip ;
WHITESPACE : ( '\t' | ' ' | '\r' | '\n'| '\u000C' )+ -> skip ;
BROPEN : '(' ;
BRCLOSE : ')' ;
SQBROPEN : '[' ;
SQBRCLOSE : ']' ;
CBROPEN : '{' ;
CBRCLOSE : '}' ;
ASSIGN : '=' ;
REL : ('<' | '<=' | '>' | '>=' | '==') ;  
AND : '&' ;
OR : '|' ; 
NOT : '!' ; 
SEMICOLON : ';' ;
COLON : ':' ;
COMMA : ',' ;
POINT : '.' ;
IF : 'if';
ELSE : 'else';
WHILE : 'while' ;
RETURN : 'return' ;
GLOBAL : 'global' ;
FUNCTION : 'fun' ;
BOOLCONST : ('true' | 'false') ; // (????) parsing problems for trueNULLdsa ??
NONECONST : 'None' ;
NAME : [a-zA-Z_] [a-zA-Z0-9_]* ;
STRCONST : '"' (~('"'|'\\') | '\\n' | '\\t' | '\\"' | '\\\\')* '"'; // (????)

// The rest of your lexical rules go here

ERROR : . ; // Must keep as last rule in grammar!
