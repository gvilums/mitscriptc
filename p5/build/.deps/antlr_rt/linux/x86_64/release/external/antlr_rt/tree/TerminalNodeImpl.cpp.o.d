{
    files = {
        "external/antlr_rt/tree/TerminalNodeImpl.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/tree/TerminalNodeImpl.cpp.o:  external/antlr_rt/tree/TerminalNodeImpl.cpp  external/antlr_rt/misc/Interval.h external/antlr_rt/antlr4-common.h  external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h external/antlr_rt/Token.h  external/antlr_rt/IntStream.h external/antlr_rt/RuleContext.h  external/antlr_rt/tree/ParseTree.h external/antlr_rt/support/Any.h  external/antlr_rt/tree/ParseTreeVisitor.h  external/antlr_rt/tree/TerminalNodeImpl.h  external/antlr_rt/tree/TerminalNode.h\
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