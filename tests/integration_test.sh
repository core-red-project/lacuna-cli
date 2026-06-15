#!/bin/bash
set -eo pipefail

LACUNA_BIN="./build/lacuna"
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT

echo "=== Running Lacuna Integration Tests ==="

# Create some test files
echo "Hello, Lacuna! This is an integration test." > "$TEMP_DIR/original.txt"

# Test RLE Explicit
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt" rle
mv "$TEMP_DIR/original.txt" "$TEMP_DIR/moved.txt"
"$LACUNA_BIN" decompress "$TEMP_DIR/original.txt.lac"
if ! cmp -s "$TEMP_DIR/moved.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: RLE Decompressed file differs from original!"
    exit 1
fi
echo "[+] RLE Identity Check Passed!"

# Test Huffman Explicit
mv "$TEMP_DIR/moved.txt" "$TEMP_DIR/original.txt"
rm -f "$TEMP_DIR/original.txt.lac"
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt" huffman
mv "$TEMP_DIR/original.txt" "$TEMP_DIR/moved.txt"
"$LACUNA_BIN" decompress "$TEMP_DIR/original.txt.lac"
if ! cmp -s "$TEMP_DIR/moved.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: Huffman Decompressed file differs from original!"
    exit 1
fi
echo "[+] Huffman Identity Check Passed!"

# Test Auto-Selection
mv "$TEMP_DIR/moved.txt" "$TEMP_DIR/original.txt"
rm -f "$TEMP_DIR/original.txt.lac"
"$LACUNA_BIN" compress "$TEMP_DIR/original.txt"
mv "$TEMP_DIR/original.txt" "$TEMP_DIR/moved.txt"
"$LACUNA_BIN" decompress "$TEMP_DIR/original.txt.lac"
if ! cmp -s "$TEMP_DIR/moved.txt" "$TEMP_DIR/original.txt"; then
    echo "[-] ERROR: Auto Decompressed file differs from original!"
    exit 1
fi
echo "[+] Auto-Selection Identity Check Passed!"

echo "=== All Integration Tests Completed Successfully! ==="
exit 0
