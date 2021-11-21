file(GLOB_RECURSE antlr_source "antlr_rt/*.cpp")
add_library(antlr ${antlr_source})
target_include_directories(antlr PUBLIC "antlr_rt")

file(GLOB_RECURSE asmjit_source "asmjit/*.cpp")
add_library(asmjit ${asmjit_source})
target_include_directories(asmjit PUBLIC "asmjit")