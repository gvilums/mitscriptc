{
    files = {
        "external/asmjit/core/globals.cpp"
    },
    depfiles_gcc = "build/.objs/asmjit/linux/x86_64/release/external/asmjit/core/globals.cpp.o:  external/asmjit/core/globals.cpp  external/asmjit/core/../core/api-build_p.h  external/asmjit/core/../core/./api-config.h  external/asmjit/core/../core/globals.h  external/asmjit/core/../core/../core/api-config.h  external/asmjit/core/../core/support.h  external/asmjit/core/../core/../core/globals.h\
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