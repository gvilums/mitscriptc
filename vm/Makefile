all: interpreter

interpreter: MITScriptLexer.cpp MITScriptParser.cpp interpreter.cpp utils.cpp vm.cpp value.cpp
	g++-11 -Wno-attributes -std=c++17 -Iinclude MITScript.cpp utils.cpp interpreter.cpp vm.cpp value.cpp lib/libantlr4-runtime.a -o interpreter 

bc-parser: bc-parser.cpp bc-lexer.cpp bc-parser.cpp utils.cpp
	g++-11 -g -std=c++17 bc-parser.cpp bc-lexer.cpp utils.cpp bc-parser-driver.cpp -o bc-parser

bc-parser.cpp: bc-parser.yy
	bison -Dapi.prefix={bc} --output=bc-parser.cpp --defines=bc-parser.h bc-parser.yy

bc-lexer.cpp: bc-lexer.lex
	flex -P bc --outfile=bc-lexer.cpp --header-file=bc-lexer.h bc-lexer.lex

compilertest: MITScriptLexer.cpp MITScriptParser.cpp compiler.cpp utils.cpp
	g++-11 -Wno-attributes -std=c++17 -Iinclude MITScript.cpp utils.cpp compiler.cpp lib/libantlr4-runtime.a -o compilertest

MITScriptLexer.cpp MITScriptParser.cpp: MITScript.g.intermediate
	@# Empty recipe

.INTERMEDIATE: MITScript.g.intermediate

MITScript.g.intermediate: MITScript.g
	java -Xmx500M -cp "bin/antlr-4.7.2-complete.jar:${CLASSPATH}" org.antlr.v4.Tool -Dlanguage=Cpp MITScript.g

clean:
	rm -f MITScript.h MITScript.cpp mitscript-parser mitscript-parse-tree mitscript MITScript.interp  MITScript.tokens
