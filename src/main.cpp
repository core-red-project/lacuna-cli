#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

#include "core/huffman.hpp"
#include "core/rle.hpp"
#include "utils/benchmark.hpp"
#include "utils/file_io.hpp"

namespace {

void print_usage() {
    std::cerr
        << "Usage: lacuna <command> <args>\n"
        << "Commands:\n"
        << "  compress <input_file> [rle|huffman]  Compress file using specified or best "
           "algorithm\n"
        << "  decompress <input_file.lac>          Decompresses .lac file to its original form\n"
        << "  info <input_file.lac>                Prints metadata information about the "
           "compressed file\n"
        << "  benchmark <directory_path>           Run benchmark for files in directory\n";
}

void print_onboarding() {
    std::cout
        << "======================================================================\n"
        << "  L A C U N A\n"
        << "======================================================================\n"
        << "  The Void / Missing Space:\n"
        << "  Lacuna is a minimalist compression utility designed to explore the\n"
        << "  preservation of content by encoding the silence (runs of duplicate\n"
        << "  bytes) within binary streams. It represents the space left behind.\n\n"
        << "  Quick Start / Onboarding Tips:\n"
        << "    lacuna compress <input_file> [rle|huffman]  Compress file (default: auto)\n"
        << "    lacuna decompress <input_file.lac>         Decompresses .lac file to original\n"
        << "    lacuna info <input_file.lac>               Prints metadata information\n"
        << "    lacuna benchmark <directory_path>          Run benchmark for files in directory\n"
        << "======================================================================\n";
}

bool handle_compress(std::string_view input_path, std::optional<std::string_view> algo_opt) {
    auto original_size_opt = lacuna::utils::get_file_size(input_path);
    if (!original_size_opt) {
        std::cerr << "Error: Input file does not exist or is not a regular file: " << input_path
                  << "\n";
        return false;
    }

    std::ifstream in(std::string(input_path), std::ios::binary);
    if (!in) {
        std::cerr << "Error: Failed to open input file for reading: " << input_path << "\n";
        return false;
    }

    std::string output_path = std::string(input_path) + ".lac";
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Failed to open output file for writing: " << output_path << "\n";
        return false;
    }

    // Read original data into memory to allow trial runs without multiple disk reads
    std::string file_data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    uint8_t selected_algo = 0;
    std::string serialized_data;

    if (algo_opt) {
        std::string_view algo_str = *algo_opt;
        if (algo_str == "rle") {
            selected_algo = 0x01;
        } else if (algo_str == "huffman") {
            selected_algo = 0x02;
        } else {
            std::cerr << "Error: Unknown algorithm '" << algo_str << "'. Use 'rle' or 'huffman'.\n";
            return false;
        }

        lacuna::utils::TrialResult result = lacuna::utils::run_trial(file_data, selected_algo);
        if (!result.success) {
            std::cerr << "Error: Compression failed for: " << input_path << "\n";
            out.close();
            std::error_code ec;
            std::filesystem::remove(output_path, ec);
            return false;
        }
        serialized_data = std::move(result.serialized_data);
    } else {
        // Auto-select by running both in memory
        lacuna::utils::TrialResult rle_result = lacuna::utils::run_trial(file_data, 0x01);
        lacuna::utils::TrialResult huff_result = lacuna::utils::run_trial(file_data, 0x02);

        if (!rle_result.success && !huff_result.success) {
            std::cerr << "Error: Compression trials failed for both algorithms on: " << input_path
                      << "\n";
            out.close();
            std::error_code ec;
            std::filesystem::remove(output_path, ec);
            return false;
        }

        lacuna::utils::TrialResult best_result;
        if (huff_result.success &&
            (!rle_result.success || huff_result.compressed_size < rle_result.compressed_size)) {
            best_result = std::move(huff_result);
        } else {
            best_result = std::move(rle_result);
        }
        serialized_data = std::move(best_result.serialized_data);
    }

    out.write(serialized_data.data(), serialized_data.size());
    if (!out) {
        std::cerr << "Error: Failed to write compressed data to output file: " << output_path
                  << "\n";
        out.close();
        std::error_code ec;
        std::filesystem::remove(output_path, ec);
        return false;
    }

    return true;
}

bool handle_decompress(std::string_view input_path) {
    std::ifstream in(std::string(input_path), std::ios::binary);
    if (!in) {
        std::cerr << "Error: Failed to open compressed file: " << input_path << "\n";
        return false;
    }

    auto header_opt = lacuna::utils::read_header(in);
    if (!header_opt) {
        std::cerr << "Error: Invalid or corrupted .lac header in: " << input_path << "\n";
        return false;
    }

    std::string output_path;
    if (input_path.ends_with(".lac")) {
        output_path = input_path.substr(0, input_path.size() - 4);
    } else {
        output_path = std::string(input_path) + ".decompressed";
    }

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Failed to open output file for writing: " << output_path << "\n";
        return false;
    }

    std::unique_ptr<lacuna::core::Compressor> compressor;
    if (header_opt->algorithm_id == 0x01) {
        compressor = std::make_unique<lacuna::core::RleCompressor>();
    } else if (header_opt->algorithm_id == 0x02) {
        compressor = std::make_unique<lacuna::core::HuffmanCompressor>();
    } else {
        std::cerr << "Error: Unsupported algorithm ID: "
                  << static_cast<int>(header_opt->algorithm_id) << "\n";
        return false;
    }

    auto decompressed_bytes = compressor->decompress(in, out, header_opt->original_size);
    if (!decompressed_bytes) {
        std::cerr << "Error: Decompression failed or file payload is corrupted: " << input_path
                  << "\n";
        out.close();
        std::error_code ec;
        std::filesystem::remove(output_path, ec);
        return false;
    }

    return true;
}

bool handle_info(std::string_view input_path) {
    auto compressed_size_opt = lacuna::utils::get_file_size(input_path);
    if (!compressed_size_opt) {
        std::cerr << "Error: Compressed file does not exist or is not a regular file: "
                  << input_path << "\n";
        return false;
    }
    uint64_t compressed_size = *compressed_size_opt;

    std::ifstream in(std::string(input_path), std::ios::binary);
    if (!in) {
        std::cerr << "Error: Failed to open compressed file: " << input_path << "\n";
        return false;
    }

    auto header_opt = lacuna::utils::read_header(in);
    if (!header_opt) {
        std::cerr << "Error: Invalid or corrupted .lac header in: " << input_path << "\n";
        return false;
    }

    double ratio = 0.0;
    if (header_opt->original_size > 0) {
        ratio = (1.0 - (static_cast<double>(compressed_size) / header_opt->original_size)) * 100.0;
    }

    if (header_opt->algorithm_id == 0x01) {
        std::cout << "Algorithm: RLE\n";
    } else if (header_opt->algorithm_id == 0x02) {
        std::cout << "Algorithm: Huffman\n";
    } else {
        std::cout << "Algorithm: Unknown\n";
    }

    std::cout << "Original Size: " << header_opt->original_size << " bytes\n";
    std::cout << "Compressed Size: " << compressed_size << " bytes\n";

    // Exact format required: "Ratio: <percentage>%" (formatted to two decimal places)
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Ratio: " << ratio << "%\n";

    std::cout << "Version: " << static_cast<int>(header_opt->version) << "\n";

    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_onboarding();
        return 0;
    }

    std::string_view command = argv[1];

    if (command == "compress") {
        if (argc < 3 || argc > 4) {
            print_usage();
            return 1;
        }
        std::string_view filepath = argv[2];
        std::optional<std::string_view> algo;
        if (argc == 4) {
            algo = argv[3];
        }
        if (!handle_compress(filepath, algo)) {
            return 1;
        }
    } else if (command == "decompress") {
        if (argc != 3) {
            print_usage();
            return 1;
        }
        std::string_view filepath = argv[2];
        if (!handle_decompress(filepath)) {
            return 1;
        }
    } else if (command == "info") {
        if (argc != 3) {
            print_usage();
            return 1;
        }
        std::string_view filepath = argv[2];
        if (!handle_info(filepath)) {
            return 1;
        }
    } else if (command == "benchmark") {
        if (argc != 3) {
            print_usage();
            return 1;
        }
        std::string_view dirpath = argv[2];
        if (!lacuna::utils::run_benchmark(dirpath)) {
            return 1;
        }
    } else {
        std::cerr << "Error: Unknown command '" << command << "'\n\n";
        print_usage();
        return 1;
    }

    return 0;
}
