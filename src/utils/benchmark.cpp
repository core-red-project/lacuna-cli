#include "benchmark.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "core/huffman.hpp"
#include "core/rle.hpp"
#include "utils/file_io.hpp"

namespace lacuna::utils {

namespace {

void scan_recursive(const std::filesystem::path& dir, std::vector<std::filesystem::path>& files) {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            continue;
        }
        std::error_code type_ec;
        if (entry.is_regular_file(type_ec)) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            if (ext == ".json" || ext == ".yaml" || ext == ".yml" || ext == ".toml" ||
                ext == ".txt") {
                files.push_back(entry.path());
            }
        } else if (entry.is_directory(type_ec)) {
            scan_recursive(entry.path(), files);
        }
    }
}

} // namespace

TrialResult run_trial(const std::string& original_data, uint8_t algo_id) {
    TrialResult result;
    result.algo_id = algo_id;

    std::unique_ptr<core::Compressor> compressor;
    if (algo_id == 0x01) {
        compressor = std::make_unique<core::RleCompressor>();
    } else if (algo_id == 0x02) {
        compressor = std::make_unique<core::HuffmanCompressor>();
    } else {
        return result;
    }

    std::istringstream in(original_data, std::ios::binary);
    std::ostringstream out(std::ios::binary);

    Header header;
    header.version = Header::CURRENT_VERSION;
    header.algorithm_id = algo_id;
    header.original_size = original_data.size();

    if (!write_header(out, header)) {
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();
    bool comp_ok = compressor->compress(in, out);
    auto end = std::chrono::high_resolution_clock::now();

    if (!comp_ok) {
        return result;
    }

    std::chrono::duration<double, std::milli> duration = end - start;
    result.duration_ms = duration.count();
    result.serialized_data = out.str();
    result.compressed_size = result.serialized_data.size();
    result.success = true;

    return result;
}

std::vector<std::filesystem::path> scan_directory(const std::filesystem::path& dir_path) {
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (std::filesystem::is_directory(dir_path, ec)) {
        scan_recursive(dir_path, files);
    }
    std::sort(files.begin(), files.end());
    return files;
}

bool run_benchmark(const std::filesystem::path& dir_path) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir_path, ec)) {
        std::cerr << "Error: Benchmark path is not a directory: " << dir_path << "\n";
        return false;
    }

    std::vector<std::filesystem::path> files = scan_directory(dir_path);
    if (files.empty()) {
        std::cout << "No matching files (.json, .yaml, .yml, .toml, .txt) found in: " << dir_path
                  << "\n";
        return true;
    }

    std::vector<BenchmarkResult> results;
    results.reserve(files.size());

    for (const auto& path : files) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }

        // Read original data in memory
        std::string original_data((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        uint64_t original_size = original_data.size();

        // Run trial RLE
        TrialResult rle_res = run_trial(original_data, 0x01);
        // Run trial Huffman
        TrialResult huff_res = run_trial(original_data, 0x02);

        if (!rle_res.success || !huff_res.success) {
            continue;
        }

        BenchmarkResult res;
        res.file_name = std::filesystem::relative(path, dir_path).string();
        res.original_size = original_size;

        if (original_size > 0) {
            res.rle_ratio = (1.0 - (static_cast<double>(rle_res.compressed_size) /
                                    static_cast<double>(original_size))) *
                            100.0;
            res.huff_ratio = (1.0 - (static_cast<double>(huff_res.compressed_size) /
                                     static_cast<double>(original_size))) *
                             100.0;
        } else {
            res.rle_ratio = 0.0;
            res.huff_ratio = 0.0;
        }

        res.rle_time_ms = rle_res.duration_ms;
        res.huff_time_ms = huff_res.duration_ms;

        // Determine recommended algorithm based on compressed size
        if (huff_res.compressed_size < rle_res.compressed_size) {
            res.recommended_algo = "Huffman";
        } else {
            res.recommended_algo = "RLE";
        }

        results.push_back(res);
    }

    // Print table header
    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";
    std::cout << "| " << std::setw(30) << "File Name";
    std::cout << " | " << std::setw(11) << "Orig Size";
    std::cout << " | " << std::setw(11) << "RLE Ratio";
    std::cout << " | " << std::setw(11) << "Huff Ratio";
    std::cout << " | " << std::setw(11) << "RLE Time";
    std::cout << " | " << std::setw(11) << "Huff Time";
    std::cout << " | " << std::setw(11) << "Winner";
    std::cout << " |\n";
    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";

    std::cout << std::fixed << std::setprecision(2);
    for (const auto& res : results) {
        // Format original size
        std::string orig_sz_str = std::to_string(res.original_size) + " B";

        // Format timings
        std::stringstream rle_t_ss, huff_t_ss;
        rle_t_ss << std::fixed << std::setprecision(3) << res.rle_time_ms << " ms";
        huff_t_ss << std::fixed << std::setprecision(3) << res.huff_time_ms << " ms";

        // Handle file names that might be too long
        std::string fn = res.file_name;
        if (fn.size() > 30) {
            fn = "..." + fn.substr(fn.size() - 27);
        }

        // Format ratios
        std::stringstream rle_r_ss, huff_r_ss;
        rle_r_ss << std::fixed << std::setprecision(2) << res.rle_ratio << "%";
        huff_r_ss << std::fixed << std::setprecision(2) << res.huff_ratio << "%";

        std::cout << "| " << std::setw(30) << fn;
        std::cout << " | " << std::setw(11) << orig_sz_str;
        std::cout << " | " << std::setw(11) << rle_r_ss.str();
        std::cout << " | " << std::setw(11) << huff_r_ss.str();
        std::cout << " | " << std::setw(11) << rle_t_ss.str();
        std::cout << " | " << std::setw(11) << huff_t_ss.str();
        std::cout << " | " << std::setw(11) << res.recommended_algo;
        std::cout << " |\n";
    }

    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";

    return true;
}

} // namespace lacuna::utils
