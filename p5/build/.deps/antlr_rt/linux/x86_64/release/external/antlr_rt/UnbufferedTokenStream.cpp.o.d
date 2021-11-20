{
    files = {
        "external/antlr_rt/UnbufferedTokenStream.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/UnbufferedTokenStream.cpp.o:  external/antlr_rt/UnbufferedTokenStream.cpp external/antlr_rt/Token.h  external/antlr_rt/IntStream.h external/antlr_rt/antlr4-common.h  external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h external/antlr_rt/Exceptions.h  external/antlr_rt/TokenSource.h external/antlr_rt/TokenFactory.h  external/antlr_rt/support/Arrays.h external/antlr_rt/misc/Interval.h  external/antlr_rt/RuleContext.h external/antlr_rt/tree/ParseTree.h  external/antlr_rt/support/Any.h external/antlr_rt/WritableToken.h  external/antlr_rt/UnbufferedTokenStream.h  external/antlr_rt/TokenStream.h\
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