{
    files = {
        "external/asmjit/core/zonestack.cpp"
    },
    depfiles_gcc = "build/.objs/asmjit/linux/x86_64/release/external/asmjit/core/zonestack.cpp.o:  external/asmjit/core/zonestack.cpp  external/asmjit/core/../core/api-build_p.h  external/asmjit/core/../core/./api-config.h  external/asmjit/core/../core/zone.h  external/asmjit/core/../core/../core/support.h  external/asmjit/core/../core/../core/../core/globals.h  external/asmjit/core/../core/../core/../core/../core/api-config.h  external/asmjit/core/../core/zonestack.h  external/asmjit/core/../core/../core/zone.h\
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