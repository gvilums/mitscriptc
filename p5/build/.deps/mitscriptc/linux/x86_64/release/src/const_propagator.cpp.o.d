{
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
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/const_propagator.cpp.o:  src/const_propagator.cpp src/value.h src/ir.h src/const_propagator.h\
",
    files = {
        "src/const_propagator.cpp"
    }
}