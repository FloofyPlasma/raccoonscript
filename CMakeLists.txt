cmake_minimum_required(VERSION 3.21)
project(RaccoonCompiler LANGUAGES C CXX)

find_package(LLVM 21.1 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM install dir: ${LLVM_INSTALL_PREFIX}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

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

target_link_libraries(raccoonc LLVM)

if(WIN32)
    target_link_libraries(raccoonc ntdll)
    
    foreach(target ${LLVM_AVAILABLE_LIBS})
        if(TARGET ${target})
            get_target_property(link_libs ${target} INTERFACE_LINK_LIBRARIES)
            if(link_libs)
                list(FILTER link_libs EXCLUDE REGEX "diaguids")
                set_target_properties(${target} PROPERTIES 
                    INTERFACE_LINK_LIBRARIES "${link_libs}")
            endif()
        endif()
    endforeach()
elseif(UNIX AND NOT APPLE)
    target_link_libraries(raccoonc pthread dl)
endif()

get_target_property(FINAL_LINK_LIBS raccoonc LINK_LIBRARIES)
message(STATUS "Final link libraries: ${FINAL_LINK_LIBS}")
