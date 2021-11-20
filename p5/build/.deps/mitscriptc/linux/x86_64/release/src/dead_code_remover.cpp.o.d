{
    files = {
        "src/dead_code_remover.cpp"
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/dead_code_remover.cpp.o:  src/dead_code_remover.cpp src/dead_code_remover.h src/value.h src/ir.h  src/irprinter.h src/regalloc.h\
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