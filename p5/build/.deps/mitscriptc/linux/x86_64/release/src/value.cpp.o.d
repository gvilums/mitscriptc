{
    files = {
        "src/value.cpp"
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/value.cpp.o:  src/value.cpp src/value.h\
",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++2a",
            "-Igrammar",
            "-Iexternal/antlr_rt",
            "-Iexternal/asmjit",
            "-DNDEBUG"
        }
    }
}