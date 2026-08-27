# Lacuna — Engineering Context & Guidelines

## Overview & Topology

- **Project**: Lacuna (`lacuna-cli`)
- **Topology**: **Monolithic** (single compiled C++20 CLI binary)
- **Language**: C++20
- **Build System**: CMake (>= 3.20) + Ninja / Make
- **Task Runner**: `just` (root `Justfile`)

Lacuna is a zero-dependency, local-first binary data compression utility implementing Run-Length Encoding (RLE) and bit-level Huffman Coding with automated trial-based algorithm selection.

---

## Command Surface

All project tasks are driven through `just`:

| Command | Description |
|---|---|
| `just install` | Bootstrap development dependencies and git hooks |
| `just dev` | Configure and compile in debug mode with Address & UB Sanitizers (`-fsanitize=address,undefined`) |
| `just build` | Produce optimized release build artifact (`./build/lacuna`) |
| `just test` | Run the complete automated test suite |
| `just typecheck` | Perform strict compilation correctness check (`-Wall -Wextra -Werror -pedantic`) |
| `just lint` | Run static analysis using `clang-tidy` |
| `just format` | Automatically format all C++ sources and headers using `clang-format` |
| `just check` | Full quality gate: `format` -> `lint` -> `typecheck` -> `test` |
| `just clean` | Remove build directories, caches, and temporary artifacts |

---

## Directory Structure & Component Map

```
lacuna-cli/
├── Justfile               # Canonical task runner abstraction layer
├── CMakeLists.txt         # Root build configuration with strict warning flags and sanitizers
├── .clang-format          # Formatting style rules (LLVM-based)
├── .clang-tidy            # Static analysis and modernization checks
├── .pre-commit-config.yaml # Git hook definitions
├── .github/workflows/     # CI/CD automation pipelines
├── src/
│   ├── main.cpp           # CLI routing, onboarding view, argument parsing, and command execution
│   ├── core/
│   │   ├── compressor.hpp # Base abstract Compressor interface
│   │   ├── rle.hpp/.cpp   # Run-Length Encoding engine
│   │   └── huffman.hpp/.cpp # Bit-level Huffman coding engine with canonical tree serialization
│   └── utils/
│       ├── file_io.hpp/.cpp # Binary stream reading/writing and .lac header serialization
│       └── benchmark.hpp/.cpp # Directory scanner and algorithm benchmarking engine
└── tests/
    └── integration_test.sh # End-to-end integration and verification test suite
```

---

## Coding Standards & Conventions

1. **Modern C++20**:
   - Favor RAII, value semantics, `std::string_view`, `std::span`, and `std::optional`.
   - Explicit ownership semantics (`std::unique_ptr` / stack allocations). Avoid raw pointers owning memory.
   - Zero-cost abstractions and zero external runtime dependencies.

2. **Correctness & Safety**:
   - Strict compiler flags: `-Wall -Wextra -Werror -pedantic` (GCC/Clang) or `/W4 /WX` (MSVC).
   - Sanitizers enabled in dev builds (`-fsanitize=address,undefined`).
   - Comprehensive error handling: return explicit booleans or `std::optional` on failure; never leak invalid file handles.

3. **Style & Formatting**:
   - Enforced by `clang-format` (.clang-format in repository root).
   - Clean naming: `lower_snake_case` for functions/variables/namespaces, `PascalCase` for classes/structs.
   - Self-documenting code with meaningful identifiers.

4. **Git Workflow**:
   - Conventional Commits (`feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `chore:`, `perf:`).
   - Every pull request must pass the automated quality gate (`just check`).
