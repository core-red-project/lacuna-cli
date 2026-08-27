# Changelog

All notable changes to **Lacuna** are documented here.

This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [0.1.1] — 2026-08-27

### Added

- **Modern CLI Terminal UX**: Styled ANSI colors, indicators (`✔`, `✖`, `ℹ`, `✦`), and strict `NO_COLOR`/TTY detection.
- **Flexible Argument Parsing**: Standard flags `-o/--output`, `-a/--algo`, `-q/--quiet`, `-v/--verbose`, `-h/--help`, `-V/--version`.
- **Structured JSON Output**: Flag `--json` across `info`, `compress`, and `benchmark` for CI pipelines and AI agents.
- **UNIX Pipe Composability**: Full streaming support for standard streams with `-` (`cat data | lacuna compress - -o output.lac`).
- **Typo Suggester**: Intelligent command and algorithm suggestions based on Levenshtein distance (*"Did you mean: compress?"*).
- **Benchmark Progress Feedback**: Dynamic interactive progress indicators during directory benchmarking.
- **Native C++ Unit Test Runner**: High-speed in-memory unit tests in `tests/unit_tests.cpp` covering compression algorithms, headers, and terminal utilities.
- **Task Runner Integration**: Justfile recipes for development, building, testing, linting, formatting, and installation.

---

## [0.1.0] — 2026-06-15

### Added

- **Initial Suite Release**: Multi-algorithm CLI compression tool for the Sxnnyside Project ecosystem.
- **RleCompressor**: Binary Run-Length Encoding engine.
- **HuffmanCompressor**: High-performance, bit-level Huffman Coding engine with deterministic leaf comparator.
- **In-Memory Trial Benchmarks**: Sequentially evaluates algorithms in memory to dynamically write the smallest size.
- **Directory Benchmarking Command**: Recursively scans and timed-benchmarks `.json`, `.yaml`, `.yml`, `.toml`, and `.txt` files, producing a clean ASCII report.
- **Onboarding UX**: Typography layouts, tips, and command explanations.
- **Robust Error Handling**: Non-zero exits on header version mismatches, table corruption, payload truncation, or empty-file failures.

---

[Unreleased]: https://github.com/core-red-project/lacuna-cli/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/core-red-project/lacuna-cli/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/core-red-project/lacuna-cli/releases/tag/v0.1.0
