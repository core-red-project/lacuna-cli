#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lacuna::utils {

struct TrialResult {
    uint8_t algo_id{0};
    size_t compressed_size{0};
    double duration_ms{0.0};
    std::string serialized_data;
    bool success{false};
};

struct BenchmarkResult {
    std::string file_name;
    uint64_t original_size{0};
    double rle_ratio{0.0};
    double huff_ratio{0.0};
    double rle_time_ms{0.0};
    double huff_time_ms{0.0};
    std::string recommended_algo;
};

/**
 * @brief Performs an in-memory compression trial using the specified algorithm.
 * @param original_data The raw input file contents.
 * @param algo_id 0x01 for RLE, 0x02 for Huffman.
 * @return TrialResult containing details of the run.
 */
TrialResult run_trial(const std::string& original_data, uint8_t algo_id);

/**
 * @brief Scans a directory for .json, .yaml, .yml, .toml, and .txt files recursively.
 * @param dir_path The path to the directory.
 * @return A list of matching regular files.
 */
std::vector<std::filesystem::path> scan_directory(const std::filesystem::path& dir_path);

/**
 * @brief Runs benchmarking on all target files in the directory and prints a pristine ASCII table.
 * @param dir_path The directory path to benchmark.
 * @return true on success, false if the directory could not be accessed.
 */
bool run_benchmark(const std::filesystem::path& dir_path);

} // namespace lacuna::utils
