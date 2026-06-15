cmake_minimum_required(VERSION 3.20)
project(lacuna LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compilation warning flags for robust and secure code
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Integrate clang-tidy checks natively into the build
find_program(CLANG_TIDY_BIN NAMES clang-tidy)
if(CLANG_TIDY_BIN)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_BIN}")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN})
else()
    message(WARNING "clang-tidy not found; static analysis checks will not run during compilation.")
endif()

add_executable(lacuna
    src/main.cpp
    src/core/rle.cpp
    src/core/huffman.cpp
    src/utils/file_io.cpp
    src/utils/benchmark.cpp
)

target_include_directories(lacuna PRIVATE src)
