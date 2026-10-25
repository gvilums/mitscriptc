file(GLOB_RECURSE asmjit_source "asmjit/*.cpp")
add_library(asmjit ${asmjit_source})
target_include_directories(asmjit PUBLIC "asmjit")
