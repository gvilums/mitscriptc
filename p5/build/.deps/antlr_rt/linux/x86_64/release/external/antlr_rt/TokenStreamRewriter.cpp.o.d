{
    files = {
        "external/antlr_rt/TokenStreamRewriter.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/TokenStreamRewriter.cpp.o:  external/antlr_rt/TokenStreamRewriter.cpp external/antlr_rt/Exceptions.h  external/antlr_rt/antlr4-common.h external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h  external/antlr_rt/misc/Interval.h external/antlr_rt/Token.h  external/antlr_rt/IntStream.h external/antlr_rt/TokenStream.h  external/antlr_rt/TokenStreamRewriter.h\
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