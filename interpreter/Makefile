parser: MITScriptLexer.cpp MITScriptParser.cpp parser.cpp
	g++ -std=c++11 -Iinclude MITScript.cpp parser.cpp lib/libantlr4-runtime.a -o mitscript-parser 

parse-tree: MITScriptLexer.cpp MITScriptParser.cpp parser.cpp
	g++ -std=c++11 -Iinclude MITScript.cpp parse-tree.cpp lib/libantlr4-runtime.a -o mitscript-parse-tree 

interpreter: MITScriptLexer.cpp MITScriptParser.cpp interpreter.cpp
	g++ -std=c++11 -Iinclude MITScript.cpp interpreter.cpp lib/libantlr4-runtime.a -o mitscript


MITScriptLexer.cpp MITScriptParser.cpp: MITScript.g.intermediate
	@# Empty recipe

.INTERMEDIATE: MITScript.g.intermediate

MITScript.g.intermediate: MITScript.g
	java -Xmx500M -cp "bin/antlr-4.7.2-complete.jar:${CLASSPATH}" org.antlr.v4.Tool -Dlanguage=Cpp MITScript.g

clean:
	rm -f MITScript.h MITScript.cpp mitscript-parser mitscript-parse-tree mitscript MITScript.interp  MITScript.tokens
