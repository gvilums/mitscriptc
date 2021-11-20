{
    files = {
        "external/antlr_rt/ProxyErrorListener.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/ProxyErrorListener.cpp.o:  external/antlr_rt/ProxyErrorListener.cpp  external/antlr_rt/ProxyErrorListener.h  external/antlr_rt/ANTLRErrorListener.h  external/antlr_rt/RecognitionException.h external/antlr_rt/Exceptions.h  external/antlr_rt/antlr4-common.h external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h\
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