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
#include "utils/terminal.hpp"

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
                return static_cast<char>(std::tolower(c));
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

bool run_benchmark(const std::filesystem::path& dir_path, bool json_output, bool quiet) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir_path, ec)) {
        std::cerr << terminal::icon_error() << " "
                  << terminal::red("Benchmark path is not a directory: ") << dir_path.string()
                  << "\n";
        return false;
    }

    std::vector<std::filesystem::path> files = scan_directory(dir_path);
    if (files.empty()) {
        if (json_output) {
            std::cout << "{\"directory\":\"" << dir_path.string()
                      << "\",\"files_count\":0,\"results\":[]}\n";
        } else if (!quiet) {
            std::cout << terminal::icon_info()
                      << " No matching files (.json, .yaml, .yml, .toml, .txt) found in: "
                      << dir_path.string() << "\n";
        }
        return true;
    }

    std::vector<BenchmarkResult> results;
    results.reserve(files.size());

    const bool show_progress = (!json_output && !quiet && terminal::is_color_enabled(std::cerr));
    size_t processed = 0;

    for (const auto& path : files) {
        ++processed;
        if (show_progress) {
            std::cerr << "\r" << terminal::cyan("⠋") << " Benchmarking [" << processed << "/"
                      << files.size() << "] " << terminal::dim(path.filename().string()) << "..."
                      << std::flush;
        }

        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }

        std::string original_data((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        uint64_t original_size = original_data.size();

        TrialResult rle_res = run_trial(original_data, 0x01);
        TrialResult huff_res = run_trial(original_data, 0x02);

        if (!rle_res.success || !huff_res.success) {
            continue;
        }

        BenchmarkResult res;
        res.file_name = std::filesystem::relative(path, dir_path).string();
        res.original_size = original_size;
        res.rle_size = rle_res.compressed_size;
        res.huff_size = huff_res.compressed_size;

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

        if (huff_res.compressed_size < rle_res.compressed_size) {
            res.recommended_algo = "Huffman";
        } else {
            res.recommended_algo = "RLE";
        }

        results.push_back(res);
    }

    if (show_progress) {
        std::cerr << "\r\033[K" << std::flush;
    }

    if (json_output) {
        std::cout << "{\n  \"directory\": \"" << dir_path.string() << "\",\n";
        std::cout << "  \"files_count\": " << results.size() << ",\n";
        std::cout << "  \"results\": [\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            std::cout << "    {\n";
            std::cout << "      \"file\": \"" << r.file_name << "\",\n";
            std::cout << "      \"original_bytes\": " << r.original_size << ",\n";
            std::cout << "      \"rle\": { \"bytes\": " << r.rle_size
                      << ", \"ratio_pct\": " << std::fixed << std::setprecision(2) << r.rle_ratio
                      << ", \"duration_ms\": " << std::setprecision(3) << r.rle_time_ms << " },\n";
            std::cout << "      \"huffman\": { \"bytes\": " << r.huff_size
                      << ", \"ratio_pct\": " << std::fixed << std::setprecision(2) << r.huff_ratio
                      << ", \"duration_ms\": " << std::setprecision(3) << r.huff_time_ms << " },\n";
            std::cout << "      \"winner\": \"" << r.recommended_algo << "\"\n";
            std::cout << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
        }
        std::cout << "  ]\n}\n";
        return true;
    }

    if (quiet) {
        return true;
    }

    // Print styled table header
    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";
    std::cout << "| " << std::setw(30) << terminal::bold("File Name");
    std::cout << " | " << std::setw(11) << terminal::bold("Orig Size");
    std::cout << " | " << std::setw(11) << terminal::bold("RLE Ratio");
    std::cout << " | " << std::setw(11) << terminal::bold("Huff Ratio");
    std::cout << " | " << std::setw(11) << terminal::bold("RLE Time");
    std::cout << " | " << std::setw(11) << terminal::bold("Huff Time");
    std::cout << " | " << std::setw(11) << terminal::bold("Winner");
    std::cout << " |\n";
    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";

    std::cout << std::fixed << std::setprecision(2);
    for (const auto& res : results) {
        std::string orig_sz_str = std::to_string(res.original_size) + " B";

        std::stringstream rle_t_ss, huff_t_ss;
        rle_t_ss << std::fixed << std::setprecision(3) << res.rle_time_ms << " ms";
        huff_t_ss << std::fixed << std::setprecision(3) << res.huff_time_ms << " ms";

        std::string fn = res.file_name;
        if (fn.size() > 30) {
            fn = "..." + fn.substr(fn.size() - 27);
        }

        std::stringstream rle_r_ss, huff_r_ss;
        rle_r_ss << std::fixed << std::setprecision(2) << res.rle_ratio << "%";
        huff_r_ss << std::fixed << std::setprecision(2) << res.huff_ratio << "%";

        std::cout << "| " << std::setw(30) << fn;
        std::cout << " | " << std::setw(11) << orig_sz_str;
        std::cout << " | " << std::setw(11) << rle_r_ss.str();
        std::cout << " | " << std::setw(11) << huff_r_ss.str();
        std::cout << " | " << std::setw(11) << rle_t_ss.str();
        std::cout << " | " << std::setw(11) << huff_t_ss.str();

        if (res.recommended_algo == "Huffman") {
            std::cout << " | " << std::setw(11) << terminal::cyan("Huffman");
        } else {
            std::cout << " | " << std::setw(11) << terminal::yellow("RLE");
        }
        std::cout << " |\n";
    }

    std::cout << std::left << std::setfill('-') << std::setw(105) << "" << std::setfill(' ')
              << "\n";

    return true;
}

} // namespace lacuna::utils
