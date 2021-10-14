all: bc-parser

bc-parser: bc-parser.cpp bc-lexer.cpp bc-parser.cpp utils.cpp
	g++ -g -std=c++17 bc-parser.cpp bc-lexer.cpp utils.cpp bc-parser-driver.cpp -o bc-parser

bc-parser.cpp: bc-parser.yy
	bison -Dapi.prefix={bc} --output=bc-parser.cpp --defines=bc-parser.h bc-parser.yy

bc-lexer.cpp: bc-lexer.lex
	flex -P bc --outfile=bc-lexer.cpp --header-file=bc-lexer.h bc-lexer.lex

clean:
	rm -f bc-lexer.cpp bc-lexer.h bc-parser.cpp bc-parser.h bc-parser.output bc-parser
