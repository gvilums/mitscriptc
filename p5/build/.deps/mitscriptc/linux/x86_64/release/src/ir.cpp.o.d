{
    files = {
        "src/ir.cpp"
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/ir.cpp.o: src/ir.cpp  src/ir.h src/value.h\
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