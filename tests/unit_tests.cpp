#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "core/huffman.hpp"
#include "core/rle.hpp"
#include "utils/benchmark.hpp"
#include "utils/file_io.hpp"
#include "utils/terminal.hpp"

namespace {

int total_assertions = 0;
int failed_assertions = 0;

void report_assert(bool condition, const char* expr, const char* file, int line) {
    ++total_assertions;
    if (!condition) {
        ++failed_assertions;
        std::cerr << "  " << lacuna::utils::terminal::red("FAILED: ") << expr
                  << " at " << file << ":" << line << "\n";
    }
}

#define EXPECT(cond) report_assert((cond), #cond, __FILE__, __LINE__)
#define EXPECT_EQ(a, b) report_assert(((a) == (b)), #a " == " #b, __FILE__, __LINE__)
#define EXPECT_TRUE(cond) report_assert((cond), #cond, __FILE__, __LINE__)
#define EXPECT_FALSE(cond) report_assert(!(cond), "!(" #cond ")", __FILE__, __LINE__)

template <typename Func>
void run_test(const std::string& name, Func fn) {
    std::cout << lacuna::utils::terminal::cyan("  ▶ ") << name << " ... " << std::flush;
    int before_failures = failed_assertions;
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;

    if (failed_assertions == before_failures) {
        std::cout << lacuna::utils::terminal::green("PASSED") << " (" << std::fixed
                  << std::setprecision(2) << ms.count() << " ms)\n";
    } else {
        std::cout << lacuna::utils::terminal::red("FAILED") << "\n";
    }
}

} // namespace

void test_rle_roundtrip() {
    lacuna::core::RleCompressor compressor;

    // 1. Repeating pattern
    std::string original = std::string(500, 'A') + std::string(300, 'B') + "C";
    std::istringstream in(original, std::ios::binary);
    std::ostringstream compressed(std::ios::binary);

    EXPECT_TRUE(compressor.compress(in, compressed));
    EXPECT_TRUE(compressed.str().size() < original.size());

    std::istringstream comp_in(compressed.str(), std::ios::binary);
    std::ostringstream decompressed(std::ios::binary);
    auto bytes = compressor.decompress(comp_in, decompressed, original.size());

    EXPECT_TRUE(bytes.has_value());
    if (bytes.has_value()) {
        EXPECT_EQ(bytes.value(), original.size());
        EXPECT_EQ(decompressed.str(), original);
    }

    // 2. Empty string
    std::string empty;
    std::istringstream empty_in(empty, std::ios::binary);
    std::ostringstream empty_out(std::ios::binary);
    EXPECT_TRUE(compressor.compress(empty_in, empty_out));

    std::istringstream empty_comp(empty_out.str(), std::ios::binary);
    std::ostringstream empty_decomp(std::ios::binary);
    auto empty_bytes = compressor.decompress(empty_comp, empty_decomp, 0);
    EXPECT_TRUE(empty_bytes.has_value());
    if (empty_bytes.has_value()) {
        EXPECT_EQ(empty_bytes.value(), 0ULL);
        EXPECT_EQ(empty_decomp.str(), "");
    }
}

void test_huffman_roundtrip() {
    lacuna::core::HuffmanCompressor compressor;

    // 1. Skewed frequency English text
    std::string original =
        "The quick brown fox jumps over the lazy dog. Repetition in data creates compression "
        "opportunities.";
    std::istringstream in(original, std::ios::binary);
    std::ostringstream compressed(std::ios::binary);

    EXPECT_TRUE(compressor.compress(in, compressed));

    std::istringstream comp_in(compressed.str(), std::ios::binary);
    std::ostringstream decompressed(std::ios::binary);
    auto bytes = compressor.decompress(comp_in, decompressed, original.size());

    EXPECT_TRUE(bytes.has_value());
    if (bytes.has_value()) {
        EXPECT_EQ(bytes.value(), original.size());
        EXPECT_EQ(decompressed.str(), original);
    }

    // 2. Full byte spectrum (all 256 byte values)
    std::string all_bytes;
    for (int i = 0; i < 256; ++i) {
        all_bytes.push_back(static_cast<char>(i));
    }
    std::istringstream all_in(all_bytes, std::ios::binary);
    std::ostringstream all_comp(std::ios::binary);
    EXPECT_TRUE(compressor.compress(all_in, all_comp));

    std::istringstream all_comp_in(all_comp.str(), std::ios::binary);
    std::ostringstream all_decomp(std::ios::binary);
    auto all_result = compressor.decompress(all_comp_in, all_decomp, all_bytes.size());
    EXPECT_TRUE(all_result.has_value());
    if (all_result.has_value()) {
        EXPECT_EQ(all_decomp.str(), all_bytes);
    }
}

void test_header_serialization() {
    lacuna::utils::Header header;
    header.version = 0x01;
    header.algorithm_id = 0x02;
    header.original_size = 9876543210ULL;

    std::ostringstream out(std::ios::binary);
    EXPECT_TRUE(lacuna::utils::write_header(out, header));

    std::string serialized = out.str();
    std::istringstream in(serialized, std::ios::binary);
    auto parsed = lacuna::utils::read_header(in);

    EXPECT_TRUE(parsed.has_value());
    if (parsed.has_value()) {
        EXPECT_EQ(parsed.value().version, 0x01);
        EXPECT_EQ(parsed.value().algorithm_id, 0x02);
        EXPECT_EQ(parsed.value().original_size, 9876543210ULL);
    }

    // Corrupted magic bytes
    std::string corrupt = serialized;
    corrupt[0] = 'X';
    std::istringstream corrupt_in(corrupt, std::ios::binary);
    EXPECT_FALSE(lacuna::utils::read_header(corrupt_in).has_value());

    // Corrupted version
    std::string bad_ver = serialized;
    bad_ver[4] = static_cast<char>(static_cast<uint8_t>(0x99));
    std::istringstream bad_ver_in(bad_ver, std::ios::binary);
    EXPECT_FALSE(lacuna::utils::read_header(bad_ver_in).has_value());
}

void test_terminal_and_typos() {
    using namespace lacuna::utils::terminal;

    // Levenshtein distance
    EXPECT_EQ(levenshtein_distance("compress", "compress"), 0ULL);
    EXPECT_EQ(levenshtein_distance("compress", "comprs"), 2ULL);
    EXPECT_EQ(levenshtein_distance("huffman", "hufman"), 1ULL);

    // Suggestions
    std::vector<std::string_view> candidates = {"compress", "decompress", "info", "benchmark"};
    EXPECT_EQ(find_suggestion("comprs", candidates), "compress");
    EXPECT_EQ(find_suggestion("decompr", candidates), "decompress");
    EXPECT_EQ(find_suggestion("benchmrk", candidates), "benchmark");
    EXPECT_EQ(find_suggestion("xyzunknown", candidates), "");


    // Format bytes
    EXPECT_EQ(format_bytes(500), "500 B");
    EXPECT_EQ(format_bytes(1024), "1.00 KB");
    EXPECT_EQ(format_bytes(1536), "1.50 KB");
    EXPECT_EQ(format_bytes(1048576), "1.00 MB");
}

void test_trial_execution() {
    std::string sample = "Sample test string with some repetitions aaaaaaaaaa bbbbbbbbbb";
    auto rle_trial = lacuna::utils::run_trial(sample, 0x01);
    auto huff_trial = lacuna::utils::run_trial(sample, 0x02);

    EXPECT_TRUE(rle_trial.success);
    EXPECT_TRUE(huff_trial.success);
    EXPECT_TRUE(rle_trial.compressed_size > 0);
    EXPECT_TRUE(huff_trial.compressed_size > 0);
    EXPECT_TRUE(rle_trial.duration_ms >= 0.0);
    EXPECT_TRUE(huff_trial.duration_ms >= 0.0);
}

int main() {
    std::cout << "\n"
              << lacuna::utils::terminal::bold("=== Lacuna Native C++ Unit Tests ===") << "\n";

    run_test("RLE Compressor In-Memory Roundtrip", test_rle_roundtrip);
    run_test("Huffman Compressor In-Memory Roundtrip", test_huffman_roundtrip);
    run_test("Binary Header Serialization & Validation", test_header_serialization);
    run_test("Terminal Utilities, Formatting & Typo Suggester", test_terminal_and_typos);
    run_test("In-Memory Trial Execution & Benchmarking", test_trial_execution);

    std::cout << "\nAssertions: " << total_assertions << " | Failures: " << failed_assertions
              << "\n";

    if (failed_assertions == 0) {
        std::cout << lacuna::utils::terminal::green("✔ All unit tests passed successfully!")
                  << "\n\n";
        return 0;
    }

    std::cout << lacuna::utils::terminal::red("✖ Some unit tests failed.") << "\n\n";
    return 1;
}
