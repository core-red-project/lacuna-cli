#!/bin/bash
set -eo pipefail

LACUNA_BIN="./build/lacuna"

if [ ! -f "$LACUNA_BIN" ]; then
    echo "[-] ERROR: Binary not found at $LACUNA_BIN. Please build first."
    exit 1
fi

TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT

echo "=== Running Lacuna Test Suite ==="

# 1. Basic text payload
echo "Hello, Lacuna! This is an integration test with repeating patterns: AAAAAAAAAAAAAAAAAAAAAAAAAAAAA BBBBBBBBBBBBB." > "$TEMP_DIR/original.txt"

# Test RLE Explicit
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt" -a rle -o "$TEMP_DIR/rle.lac" -q
"$LACUNA_BIN" decompress "$TEMP_DIR/rle.lac" -o "$TEMP_DIR/rle_decomp.txt" -q
if ! cmp -s "$TEMP_DIR/rle_decomp.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: RLE Decompressed file differs from original!"
    exit 1
fi
echo "[+] RLE Identity Check Passed!"

# Test Huffman Explicit
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt" -a huffman -o "$TEMP_DIR/huff.lac" -q
"$LACUNA_BIN" decompress "$TEMP_DIR/huff.lac" -o "$TEMP_DIR/huff_decomp.txt" -q
if ! cmp -s "$TEMP_DIR/huff_decomp.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: Huffman Decompressed file differs from original!"
    exit 1
fi
echo "[+] Huffman Identity Check Passed!"

# Test Auto-Selection
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt" -o "$TEMP_DIR/auto.lac" -q
"$LACUNA_BIN" decompress "$TEMP_DIR/auto.lac" -o "$TEMP_DIR/auto_decomp.txt" -q
if ! cmp -s "$TEMP_DIR/auto_decomp.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: Auto Decompressed file differs from original!"
    exit 1
fi
echo "[+] Auto-Selection Identity Check Passed!"

# Test Info Command (Text and JSON)
echo "[*] Testing info command..."
INFO_OUTPUT=$("$LACUNA_BIN" info "$TEMP_DIR/auto.lac")
if ! echo "$INFO_OUTPUT" | grep -q "Algorithm:"; then
    echo "[-] ERROR: info output missing Algorithm field"
    exit 1
fi
if ! echo "$INFO_OUTPUT" | grep -q "Ratio:"; then
    echo "[-] ERROR: info output missing Ratio field"
    exit 1
fi

JSON_INFO=$("$LACUNA_BIN" info "$TEMP_DIR/auto.lac" --json)
if ! echo "$JSON_INFO" | grep -q '"algorithm":'; then
    echo "[-] ERROR: info --json output missing algorithm key"
    exit 1
fi
echo "[+] Info Command & JSON Inspection Passed!"

# Test Help Command
echo "[*] Testing help command..."
HELP_OUTPUT=$("$LACUNA_BIN" help)
if ! echo "$HELP_OUTPUT" | grep -iq "USAGE:"; then
    echo "[-] ERROR: help output missing USAGE block"
    exit 1
fi
echo "[+] Help Command Check Passed!"

# Test UNIX Pipes (stdin / stdout)
echo "[*] Testing UNIX pipe streaming..."
cat "$TEMP_DIR/original.txt" | "$LACUNA_BIN" compress - -o "$TEMP_DIR/pipe.lac" -q
cat "$TEMP_DIR/pipe.lac" | "$LACUNA_BIN" decompress - -o "$TEMP_DIR/pipe_decomp.txt" -q
if ! cmp -s "$TEMP_DIR/pipe_decomp.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: Pipe streaming roundtrip differs from original!"
    exit 1
fi
echo "[+] UNIX Pipe Streaming Check Passed!"

# Test Benchmark Command (Text & JSON)
echo "[*] Testing benchmark command on sample directory..."
BENCH_DIR="$TEMP_DIR/bench"
mkdir -p "$BENCH_DIR"
echo "key: value" > "$BENCH_DIR/sample.yaml"
echo '{"test": 123, "active": true}' > "$BENCH_DIR/sample.json"
echo "Some text for benchmark." > "$BENCH_DIR/sample.txt"
"$LACUNA_BIN" benchmark "$BENCH_DIR" -q
BENCH_JSON=$("$LACUNA_BIN" benchmark "$BENCH_DIR" --json)
if ! echo "$BENCH_JSON" | grep -q '"results":'; then
    echo "[-] ERROR: benchmark --json missing results array"
    exit 1
fi
echo "[+] Benchmark Command & JSON Format Check Passed!"

# Test Typo Suggestion
echo "[*] Testing typo suggestions..."
TYPO_OUTPUT=$("$LACUNA_BIN" comprs 2>&1 || true)
if ! echo "$TYPO_OUTPUT" | grep -q "Did you mean: compress"; then
    echo "[-] ERROR: Expected 'Did you mean: compress' suggestion"
    exit 1
fi
echo "[+] Typo Suggestion Check Passed!"

# Test Edge Case: Non-existent file error handling
if "$LACUNA_BIN" compress "$TEMP_DIR/non_existent_file.txt" 2>/dev/null; then
    echo "[-] ERROR: Expected failure on non-existent file, but command succeeded!"
    exit 1
fi
echo "[+] Error Handling (Missing File) Check Passed!"

# Test Edge Case: Invalid algorithm flag
if "$LACUNA_BIN" compress "$TEMP_DIR/original.txt" -a invalid_algo 2>/dev/null; then
    echo "[-] ERROR: Expected failure on invalid algorithm, but command succeeded!"
    exit 1
fi
echo "[+] Error Handling (Invalid Algo) Check Passed!"

echo "=== All Tests Completed Successfully! ==="
exit 0
