lexer grammar MITScript;
 
// Lexer Rules

INT : [0-9]+ ;

MUL : '*' ;

DIV : '/' ;

COMMENT: '//'~( '\r' | '\n' )* -> skip ;

WHITESPACE : ( '\t' | ' ' | '\r' | '\n'| '\u000C' )+ -> skip ;

// The rest of your lexical rules go here

ERROR : . ; // Must keep as last rule in grammar!
