{
    files = {
        "external/antlr_rt/ParserRuleContext.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/ParserRuleContext.cpp.o:  external/antlr_rt/ParserRuleContext.cpp  external/antlr_rt/tree/TerminalNode.h external/antlr_rt/tree/ParseTree.h  external/antlr_rt/support/Any.h external/antlr_rt/antlr4-common.h  external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h  external/antlr_rt/tree/ErrorNode.h external/antlr_rt/misc/Interval.h  external/antlr_rt/Parser.h external/antlr_rt/Recognizer.h  external/antlr_rt/ProxyErrorListener.h  external/antlr_rt/ANTLRErrorListener.h  external/antlr_rt/RecognitionException.h external/antlr_rt/Exceptions.h  external/antlr_rt/support/Casts.h  external/antlr_rt/tree/ParseTreeListener.h  external/antlr_rt/TokenStream.h external/antlr_rt/IntStream.h  external/antlr_rt/TokenSource.h external/antlr_rt/TokenFactory.h  external/antlr_rt/Token.h external/antlr_rt/support/CPPUtils.h  external/antlr_rt/ParserRuleContext.h external/antlr_rt/RuleContext.h\
",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++2a",
            "-Iexternal/antlr_rt",
            "-DNDEBUG"
        }
    }
}