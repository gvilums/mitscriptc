{
    values = {
        "/usr/bin/g++",
        {
            "-m64",
            "-Lbuild/linux/x86_64/release",
            "-s",
            "-lantlr_rt",
            "-lasmjit"
        }
    },
    files = {
        "build/.objs/mitscriptc/linux/x86_64/release/grammar/MITScript.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/ir.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/compiler.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/compilertest.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/value.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/parsercode.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/const_propagator.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/irprinter.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/regalloc.cpp.o",
        "build/.objs/mitscriptc/linux/x86_64/release/src/dead_code_remover.cpp.o",
        "build/linux/x86_64/release/libasmjit.a",
        "build/linux/x86_64/release/libantlr_rt.a"
    }
}