{
    files = {
        "external/antlr_rt/InterpreterRuleContext.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/InterpreterRuleContext.cpp.o:  external/antlr_rt/InterpreterRuleContext.cpp  external/antlr_rt/InterpreterRuleContext.h  external/antlr_rt/ParserRuleContext.h external/antlr_rt/RuleContext.h  external/antlr_rt/tree/ParseTree.h external/antlr_rt/support/Any.h  external/antlr_rt/antlr4-common.h external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h  external/antlr_rt/support/CPPUtils.h\
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