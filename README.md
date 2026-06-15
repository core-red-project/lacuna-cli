# Lacuna

![License](https://img.shields.io/badge/license-MIT-blue)
![CI](https://github.com/core-red-project/lacuna-cli/actions/workflows/ci.yml/badge.svg)

<p align="center">
  <strong>Zero-dependency ✦ Local-first ✦ Chunk-based binary compression</strong><br>
  <em>A minimalist, high-performance C++20 CLI data compression tool leveraging RLE and Huffman coding.</em>
</p>

<p align="center">
  <a href="#about">About</a> ✦
  <a href="#features">Features</a> ✦
  <a href="#installation">Installation</a> ✦
  <a href="#usage">Usage</a> ✦
  <a href="#architecture">Architecture</a> ✦
  <a href="#contributing">Contributing</a>
</p>

---

## About

**Lacuna** is a minimalist, zero-dependency, high-performance CLI data compression tool designed for the Sxnnyside Project ecosystem.

It exists to provide clean, local-first binary data compression without external dependencies, focusing on simplicity, extreme reliability, and strict modern C++20 systems standards.

It implements Run-Length Encoding (RLE) and bit-level Huffman Coding, enabling trial-based auto-selection to automatically output the most optimized compressed file.

### Philosophy

> *"An intentional absence: encoding silence and stripping redundancy to reveal the space left behind."*

This is a CoreRed project, part of the Sxnnyside Project's experimental branch.

## Features

- **Automated Algorithm Selection**: Evaluates both RLE and Huffman in-memory to choose the best compression format.
- **Run-Length Encoding (RLE)**: Efficient compression of contiguous repeating byte streams.
- **Bit-Level Huffman Coding**: High-fidelity bit-packing with custom frequency-table serialization.
- **High-Performance Benchmarking**: Integrated pipeline to recursively scan and benchmark text/config formats in target directories.
- **Minimalist UX Onboarding**: Custom interactive help and clean dashboard rendering on empty execution.

## Installation

### Prerequisites

- CMake (>= 3.25)
- C++20 Compiler (GCC >= 11, Clang >= 13, Apple Clang >= 14)

### From Source

```bash
git clone https://github.com/core-red-project/lacuna-cli.git
cd lacuna-cli

cmake -B build
cmake --build build
```

## Usage

```bash
# Compress a file using the best algorithm (RLE or Huffman auto-selected)
./build/lacuna compress input.txt

# Compress a file specifying the algorithm explicitly
./build/lacuna compress input.txt huffman
./build/lacuna compress input.txt rle

# Decompress a compressed .lac file
./build/lacuna decompress input.txt.lac

# Print metadata information about a compressed file
./build/lacuna info input.txt.lac

# Benchmark RLE vs Huffman on all TXT/JSON/YAML/TOML files in a directory
./build/lacuna benchmark ./data_dir
```

## Architecture

```
lacuna-cli/
├── src/          # Source files including the CLI router entrypoint (main.cpp).
├── src/core/     # Compression engines and algorithm interfaces (RleCompressor, HuffmanCompressor).
└── src/utils/    # Binary IO, benchmarking, and custom header parsing utilities.
```

## Contributing

Contributions are accepted. See CONTRIBUTING.md for guidelines.

Before contributing, read the Code of Conduct.

## License

This project is licensed under the MIT — see the LICENSE file for details.

---

<p align="center">
  <strong>Lacuna</strong> — A CoreRed Project<br>
  <em>&copy; 2026 Sxnnyside Project</em>
</p>
