{
    files = {
        "external/asmjit/core/virtmem.cpp"
    },
    depfiles_gcc = "build/.objs/asmjit/linux/x86_64/release/external/asmjit/core/virtmem.cpp.o:  external/asmjit/core/virtmem.cpp  external/asmjit/core/../core/api-build_p.h  external/asmjit/core/../core/./api-config.h  external/asmjit/core/../core/osutils.h  external/asmjit/core/../core/../core/globals.h  external/asmjit/core/../core/../core/../core/api-config.h  external/asmjit/core/../core/string.h  external/asmjit/core/../core/../core/support.h  external/asmjit/core/../core/../core/../core/globals.h  external/asmjit/core/../core/../core/zone.h  external/asmjit/core/../core/../core/../core/support.h  external/asmjit/core/../core/support.h  external/asmjit/core/../core/virtmem.h  external/asmjit/core/../core/../core/api-config.h\
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