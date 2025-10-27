cmake_minimum_required(VERSION 3.21)

project(RaccoonCompiler LANGUAGES C CXX)
find_package(LLVM 21.1 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

# HUGE HACK: Remove diaguids from link libraries to avoid stupid errors.
# TODO: Find a better solution. This is disgusting, but it works for now.
if(WIN32)
    foreach(target ${LLVM_AVAILABLE_LIBS})
        if(TARGET ${target})
            get_target_property(link_libs ${target} INTERFACE_LINK_LIBRARIES)
            if(link_libs)
                list(FILTER link_libs EXCLUDE REGEX "diaguids")
                set_target_properties(${target} PROPERTIES INTERFACE_LINK_LIBRARIES "${link_libs}")
            endif()
        endif()
    endforeach()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/ASTPrinter.cpp
    src/Parser.cpp
    src/Codegen.cpp
)

add_executable(raccoonc ${SOURCES})

llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
    target
    ${LLVM_TARGETS_TO_BUILD}
)

if(WIN32)
    list(FILTER llvm_libs EXCLUDE REGEX "diaguids")
endif()

message(STATUS "LLVM libraries: ${llvm_libs}")

target_link_libraries(raccoonc ${llvm_libs})

if(WIN32)
    target_link_libraries(raccoonc ntdll)
endif()
