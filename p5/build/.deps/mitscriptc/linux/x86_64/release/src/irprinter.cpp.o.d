{
    files = {
        "src/irprinter.cpp"
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/irprinter.cpp.o:  src/irprinter.cpp src/ir.h src/value.h src/irprinter.h src/regalloc.h\
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