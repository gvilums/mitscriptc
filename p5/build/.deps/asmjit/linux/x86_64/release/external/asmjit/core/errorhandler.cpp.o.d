{
    files = {
        "external/asmjit/core/errorhandler.cpp"
    },
    depfiles_gcc = "build/.objs/asmjit/linux/x86_64/release/external/asmjit/core/errorhandler.cpp.o:  external/asmjit/core/errorhandler.cpp  external/asmjit/core/../core/api-build_p.h  external/asmjit/core/../core/./api-config.h  external/asmjit/core/../core/errorhandler.h  external/asmjit/core/../core/../core/globals.h  external/asmjit/core/../core/../core/../core/api-config.h\
",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++2a",
            "-DNDEBUG"
        }
    }
}