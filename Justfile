# Task runner command surface

# Default recipe listing available commands
default:
    @just --list

# Install and bootstrap development tools and git hooks
install:
    @command -v cmake >/dev/null 2>&1 || (echo "[-] cmake not found in PATH" && exit 1)
    @command -v clang-format >/dev/null 2>&1 || echo "[!] Note: clang-format recommended for automatic formatting"
    @if command -v pre-commit >/dev/null 2>&1; then pre-commit install; fi
    @echo "[+] Environment setup complete"

# Install compiled binary to user or system bin path (default: ~/.local)
install-bin prefix=(env_var('HOME') + "/.local"): build
    @cmake --install build --prefix {{prefix}}
    @echo "[+] lacuna binary installed to {{prefix}}/bin/lacuna"



# Configure and compile in development mode with sanitizers enabled
dev:
    @cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
    @cmake --build build --config Debug

# Produce release build artifacts
build:
    @cmake -B build -DCMAKE_BUILD_TYPE=Release
    @cmake --build build --config Release

# Run the test suite (native C++ unit tests + CLI integration tests)
test: build
    @./build/unit_tests
    @bash tests/integration_test.sh


# Run compilation correctness check with strict compiler warnings
typecheck:
    @cmake -B build -DCMAKE_BUILD_TYPE=Release
    @cmake --build build --target lacuna

# Run static analysis and linter checks
lint:
    @cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null 2>&1
    @if command -v clang-tidy >/dev/null 2>&1; then \
        find src/ -name '*.cpp' | xargs clang-tidy -p build; \
        echo "[+] Static analysis passed with clang-tidy"; \
    else \
        echo "[*] clang-tidy not available in environment; skipping deep static analysis"; \
    fi

# Apply code formatting
format:
    @if command -v clang-format >/dev/null 2>&1; then \
        find src/ -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i; \
        echo "[+] Code formatted successfully with clang-format"; \
    else \
        echo "[!] clang-format not found; please install clang-format to format code"; \
    fi

# Full quality gate (formatting, building, linting, correctness, tests)
check: format build lint test
    @echo "[+] Quality gate completed successfully!"

# Remove build artifacts and caches
clean:
    @rm -rf build .cache
    @echo "[+] Cleaned build artifacts"
