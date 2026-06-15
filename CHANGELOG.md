# Changelog

All notable changes to **Lacuna** will be documented here.

This project follows Keep a Changelog and Semantic Versioning.

---

## [Unreleased]

### Added

- Planning for future algorithms (e.g. LZW/Deflate).

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

[Unreleased]: https://github.com/core-red-project/lacuna-cli/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/core-red-project/lacuna-cli/releases/tag/v0.1.0
