cmake_minimum_required(VERSION 3.20)

project(RaccoonCompiler LANGUAGES C CXX)
find_package(LLVM 20.1.8 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries: ${LLVM_LIBS}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/ASTPrinter.cpp
    src/Parser.cpp
    src/Codegen.cpp
)

add_executable(raccoonc ${SOURCES})

# Use llvm-config to get correct link flags
execute_process(
    COMMAND llvm-config --libs
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND llvm-config --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE " " ";" LLVM_LIBS "${LLVM_LIBS}")
string(REPLACE " " ";" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")

target_compile_options(raccoonc PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(raccoonc ${LLVM_LIBS})
