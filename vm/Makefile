all: compiler

bc-parser: bc-parser.cpp bc-lexer.cpp bc-parser.cpp utils.cpp
	g++ -g -std=c++17 bc-parser.cpp bc-lexer.cpp utils.cpp bc-parser-driver.cpp -o bc-parser

compiler: MITScriptLexer.cpp MITScriptParser.cpp compiler.cpp utils.cpp
	g++ -Wno-attributes -std=c++17 -Iinclude MITScript.cpp utils.cpp compiler.cpp lib/libantlr4-runtime.a -o compiler

bc-parser.cpp: bc-parser.yy
	bison -Dapi.prefix={bc} --output=bc-parser.cpp --defines=bc-parser.h bc-parser.yy

bc-lexer.cpp: bc-lexer.lex
	flex -P bc --outfile=bc-lexer.cpp --header-file=bc-lexer.h bc-lexer.lex

MITScriptLexer.cpp MITScriptParser.cpp: MITScript.g.intermediate
	@# Empty recipe

.INTERMEDIATE: MITScript.g.intermediate

MITScript.g.intermediate: MITScript.g
	java -Xmx500M -cp "bin/antlr-4.7.2-complete.jar:${CLASSPATH}" org.antlr.v4.Tool -Dlanguage=Cpp MITScript.g

clean:
	rm -f bc-lexer.cpp bc-lexer.h bc-parser.cpp bc-parser.h bc-parser.output bc-parser MITScript.h MITScript.cpp mitscript-parser mitscript-parse-tree mitscript MITScript.interp  MITScript.tokens
