# minimum required CMAKE version
cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(mitscript LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# using /MD flag for antlr4_runtime (for Visual C++ compilers only)
set(ANTLR4_WITH_STATIC_CRT OFF)
# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
set(ANTLR_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/antlr/antlr-4.9.3-complete.jar)
# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

# Call macro to add lexer and grammar to your build dependencies.
antlr_target(SampleGrammarLexer MITScript.g LEXER
             PACKAGE lexer)
# antlr_target(SampleGrammarParser TParser.g4 PARSER
#              PACKAGE antlrcpptest
#              DEPENDS_ANTLR SampleGrammarLexer
#              COMPILE_FLAGS -lib ${ANTLR_SampleGrammarLexer_OUTPUT_DIR})

# include generated files in project environment
include_directories(${ANTLR_SampleGrammarLexer_OUTPUT_DIR})
# include_directories(${ANTLR_SampleGrammarParser_OUTPUT_DIR})

set(CMAKE_BUILD_TYPE "Debug")

# add generated grammar to demo binary target
add_executable(mitscriptc src/compiler.cpp src/parsercode.cpp src/irprinter.cpp
               ${ANTLR_SampleGrammarLexer_CXX_OUTPUTS})
            #    ${ANTLR_SampleGrammarParser_CXX_OUTPUTS})
            
add_executable(test_regalloc src/test_regalloc.cpp src/regalloc.cpp)

add_executable(test test/doctest.cpp test/regalloc_test.cpp src/regalloc.cpp)

target_link_libraries(mitscriptc antlr4_static)
