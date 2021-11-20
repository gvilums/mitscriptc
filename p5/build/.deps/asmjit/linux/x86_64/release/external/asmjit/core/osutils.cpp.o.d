{
    files = {
        "external/asmjit/core/osutils.cpp"
    },
    depfiles_gcc = "build/.objs/asmjit/linux/x86_64/release/external/asmjit/core/osutils.cpp.o:  external/asmjit/core/osutils.cpp  external/asmjit/core/../core/api-build_p.h  external/asmjit/core/../core/./api-config.h  external/asmjit/core/../core/osutils.h  external/asmjit/core/../core/../core/globals.h  external/asmjit/core/../core/../core/../core/api-config.h  external/asmjit/core/../core/support.h\
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