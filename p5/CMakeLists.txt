cmake_minimum_required(VERSION 3.10)
project(mitscript)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_subdirectory(external)

set(sources ${sources}
    src/codegen.cpp
    src/compiler.cpp
    src/const_propagator.cpp
    src/dead_code_remover.cpp
    src/shape_analysis.cpp
    src/ir.cpp
    src/irprinter.cpp
    src/parsercode.cpp
    src/regalloc.cpp
    src/value.cpp
    src/utils.cpp
    grammar/MITScript.cpp
)

# set(test_sources ${test_sources}
    # test/doctest.cpp
    # test/regalloc_test.cpp
    # test/value_test.cpp
# )

add_executable(mitscriptc ${sources} src/compilertest.cpp)
target_link_libraries(mitscriptc PUBLIC antlr)
target_link_libraries(mitscriptc PUBLIC asmjit)
target_include_directories(mitscriptc PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/grammar")
target_include_directories(mitscriptc PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/external/parallel_hashmap")

# add_executable(test ${sources} ${test_sources})
# target_link_libraries(test PUBLIC antlr)
# target_link_libraries(test PUBLIC asmjit)
# target_include_directories(test PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/grammar")
# target_include_directories(test PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/src")


if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_library(LIB_RT rt)
    target_link_libraries(mitscriptc PUBLIC ${LIB_RT})
endif()