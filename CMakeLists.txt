cmake_minimum_required(VERSION 3.20)
project(lacuna LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

# Add compilation warning flags for robust and secure code
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Integrate clang-tidy checks natively into the build
find_program(CLANG_TIDY_BIN NAMES clang-tidy)
if(CLANG_TIDY_BIN)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_BIN}")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN})
else()
    message(STATUS "clang-tidy not found; static analysis can be run via linter tasks.")
endif()

add_executable(lacuna
    src/main.cpp
    src/core/rle.cpp
    src/core/huffman.cpp
    src/utils/file_io.cpp
    src/utils/benchmark.cpp
    src/utils/terminal.cpp
)

target_include_directories(lacuna PRIVATE src)

add_executable(unit_tests
    tests/unit_tests.cpp
    src/core/rle.cpp
    src/core/huffman.cpp
    src/utils/file_io.cpp
    src/utils/benchmark.cpp
    src/utils/terminal.cpp
)
target_include_directories(unit_tests PRIVATE src)


enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME integration_test COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.sh)

install(TARGETS lacuna DESTINATION bin)


