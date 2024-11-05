cmake_minimum_required(VERSION 3.20)
project(brainrotlang)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
execute_process(
    COMMAND brew --prefix llvm
    OUTPUT_VARIABLE HOMEBREW_LLVM_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_DIR "${HOMEBREW_LLVM_PATH}/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Add LLVM definitions and include paths
add_definitions(${LLVM_DEFINITIONS})
include_directories(SYSTEM ${HOMEBREW_LLVM_PATH}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(brainrotlang
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/codegen.cpp
)

llvm_map_components_to_libnames(llvm_libs
    core
    support
    orcjit
    native
    mcjit
    executionengine
    interpreter
    x86codegen
    aarch64codegen
)

target_link_libraries(brainrotlang PRIVATE ${llvm_libs})

# Set compile options but without -fno-rtti
if(APPLE)
    target_compile_options(brainrotlang PRIVATE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O3>
    )
    target_link_options(brainrotlang PRIVATE -stdlib=libc++)
endif()