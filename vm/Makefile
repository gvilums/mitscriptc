all: mitscript

mitscript: MITScriptLexer.cpp MITScriptParser.cpp mitscript.cpp utils.cpp vm.cpp value.cpp bc-parser.cpp bc-lexer.cpp
	g++-11 -g -Wno-attributes -std=c++17 -Iinclude MITScript.cpp utils.cpp bc-parser.cpp bc-lexer.cpp mitscript.cpp vm.cpp value.cpp lib/libantlr4-runtime.a -o mitscript 

bc-parser.cpp: bc-parser.yy
	bison -Dapi.prefix={bc} --output=bc-parser.cpp --defines=bc-parser.h bc-parser.yy

bc-lexer.cpp: bc-lexer.lex
	flex -P bc --outfile=bc-lexer.cpp --header-file=bc-lexer.h bc-lexer.lex

compiler: MITScriptLexer.cpp MITScriptParser.cpp compiler.cpp utils.cpp
	g++-11 -Wno-attributes -std=c++17 -Iinclude MITScript.cpp utils.cpp compiler.cpp lib/libantlr4-runtime.a -o compiler

MITScriptLexer.cpp MITScriptParser.cpp: MITScript.g.intermediate
	@# Empty recipe

.INTERMEDIATE: MITScript.g.intermediate

MITScript.g.intermediate: MITScript.g
	java -Xmx500M -cp "bin/antlr-4.7.2-complete.jar:${CLASSPATH}" org.antlr.v4.Tool -Dlanguage=Cpp MITScript.g

clean:
	rm -f MITScript.h MITScript.cpp MITScript.interp  MITScript.tokens bc-lexer.cpp bc-lexer.h bc-parser.cpp bc-parser.h mitscript
