{
    files = {
        "src/compiler.cpp"
    },
    depfiles_gcc = "build/.objs/mitscriptc/linux/x86_64/release/src/compiler.cpp.o:  src/compiler.cpp src/compiler.h src/AST.h src/Visitor.h src/Assigns.h  src/FreeVariables.h src/Globals.h src/ir.h src/value.h src/irprinter.h  src/regalloc.h\
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