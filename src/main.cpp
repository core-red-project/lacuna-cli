#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/huffman.hpp"
#include "core/rle.hpp"
#include "utils/benchmark.hpp"
#include "utils/file_io.hpp"
#include "utils/terminal.hpp"

namespace {

constexpr std::string_view LACUNA_VERSION = "0.1.1";

const std::vector<std::string_view> KNOWN_COMMANDS =
    {"compress", "decompress", "info", "benchmark", "help", "version"};

const std::vector<std::string_view> KNOWN_ALGOS = {"rle", "huffman", "auto"};

struct CliOptions {
    std::string command;
    std::string positional_target;
    std::optional<std::string> output_path;
    std::optional<std::string> algorithm;
    bool json_output{false};
    bool quiet{false};
    bool verbose{false};
    bool show_help{false};
    bool show_version{false};
};

void print_version() {
    std::cout << "lacuna " << LACUNA_VERSION << " (C++20, "
              << lacuna::utils::terminal::cyan("Core Red Project") << ")\n";
}

void print_usage(std::ostream& os = std::cout) {
    using namespace lacuna::utils::terminal;
    os << bold("Lacuna") << " " << dim(std::string("v") + std::string(LACUNA_VERSION)) << " — "
       << "Minimalist binary data compression CLI\n\n"
       << bold("USAGE:\n") << "  lacuna <command> [options] <target>\n"
       << "  lacuna [options]\n\n"
       << bold("COMMANDS:\n") << "  " << cyan("compress")
       << " <file> [algo]     Compress file (default: auto)\n"
       << "  " << cyan("decompress") << " <file.lac>      Decompress .lac archive\n"
       << "  " << cyan("info")
       << " <file.lac>            Inspect .lac headers and compression metrics\n"
       << "  " << cyan("benchmark")
       << " <dir>           Benchmark RLE vs Huffman across a directory\n"
       << "  " << cyan("help") << "                       Show this help menu\n"
       << "  " << cyan("version") << "                    Display version information\n\n"
       << bold("OPTIONS:\n") << "  " << yellow("-o, --output")
       << " <path>        Custom destination output path (or '-' for stdout)\n"
       << "  " << yellow("-a, --algo") << " <rle|huffman>   Explicit compression algorithm\n"
       << "  " << yellow("-q, --quiet") << "                Suppress non-error output\n"
       << "  " << yellow("-v, --verbose") << "              Show detailed execution diagnostics\n"
       << "  " << yellow("    --json")
       << "                 Output machine-readable JSON (info, benchmark, compress)\n"
       << "  " << yellow("-h, --help") << "                 Display help information\n"
       << "  " << yellow("-V, --version") << "              Display version information\n\n"
       << bold("EXAMPLES:\n") << "  " << dim("# Compress with automatic trial-selection:") << "\n"
       << "  lacuna compress document.txt\n\n"
       << "  " << dim("# Compress via UNIX pipe:") << "\n"
       << "  cat data.bin | lacuna compress - -o output.lac\n\n"
       << "  " << dim("# Inspect compression ratio in JSON format:") << "\n"
       << "  lacuna info document.txt.lac --json\n\n"
       << "  " << dim("# Benchmark an entire data directory:") << "\n"
       << "  lacuna benchmark ./test_data\n";
}

void print_onboarding() {
    using namespace lacuna::utils::terminal;
    std::cout << "\n"
              << bold("  ┌─────────────────────────────────────────────────────────────┐\n")
              << bold("  │ ") << magenta("✦") << " " << bold(cyan("L A C U N A")) << " "
              << dim(std::string("v") + std::string(LACUNA_VERSION)) << "  "
              << dim("Core Red Project") << "                       │\n"
              << bold("  └─────────────────────────────────────────────────────────────┘\n") << "  "
              << dim("High-performance zero-dependency binary data compression") << "\n\n"
              << bold("  Quick Start:") << "\n"
              << "    " << cyan("lacuna compress") << " <input_file>        "
              << dim("Auto-select best algorithm (RLE/Huffman)") << "\n"
              << "    " << cyan("lacuna decompress") << " <file.lac>        "
              << dim("Restore file to original state") << "\n"
              << "    " << cyan("lacuna info") << " <file.lac>              "
              << dim("Display compression ratio & metadata") << "\n"
              << "    " << cyan("lacuna benchmark") << " <directory>       "
              << dim("Compare algorithms across test data") << "\n\n"
              << "  Run " << yellow("lacuna --help")
              << " for options, pipes, and JSON output flags.\n\n";
}

bool parse_cli_args(int argc, char** argv, CliOptions& opt) {
    std::vector<std::string_view> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty()) {
        return true;
    }

    size_t idx = 0;
    while (idx < args.size()) {
        std::string_view a = args[idx];

        if (a == "-h" || a == "--help" || a == "help") {
            opt.show_help = true;
            return true;
        }
        if (a == "-V" || a == "--version" || a == "version") {
            opt.show_version = true;
            return true;
        }
        if (a == "-q" || a == "--quiet") {
            opt.quiet = true;
            ++idx;
            continue;
        }
        if (a == "-v" || a == "--verbose") {
            opt.verbose = true;
            ++idx;
            continue;
        }
        if (a == "--json") {
            opt.json_output = true;
            ++idx;
            continue;
        }
        if (a == "-o" || a == "--output") {
            if (idx + 1 >= args.size()) {
                std::cerr << lacuna::utils::terminal::icon_error() << " "
                          << lacuna::utils::terminal::red("Option requires argument: ") << a
                          << "\n";
                return false;
            }
            opt.output_path = std::string(args[++idx]);
            ++idx;
            continue;
        }
        if (a == "-a" || a == "--algo" || a == "--algorithm") {
            if (idx + 1 >= args.size()) {
                std::cerr << lacuna::utils::terminal::icon_error() << " "
                          << lacuna::utils::terminal::red("Option requires argument: ") << a
                          << "\n";
                return false;
            }
            opt.algorithm = std::string(args[++idx]);
            ++idx;
            continue;
        }

        // Positional argument
        if (opt.command.empty()) {
            opt.command = std::string(a);
        } else if (opt.positional_target.empty()) {
            opt.positional_target = std::string(a);
        } else if (!opt.algorithm && (a == "rle" || a == "huffman" || a == "auto")) {
            // Support legacy positional algorithm flag: lacuna compress input.txt rle
            opt.algorithm = std::string(a);
        } else {
            std::cerr << lacuna::utils::terminal::icon_error() << " "
                      << lacuna::utils::terminal::red("Unexpected argument: ") << a << "\n";
            return false;
        }
        ++idx;
    }

    return true;
}

bool handle_compress(const CliOptions& opt) {
    using namespace lacuna::utils::terminal;
    const std::string& input_path = opt.positional_target;

    if (input_path.empty()) {
        std::cerr << icon_error() << " " << red("Missing target input file for compression.")
                  << "\n";
        print_usage(std::cerr);
        return false;
    }

    std::string file_data;
    if (input_path == "-") {
        // Read from standard input
        file_data.assign((std::istreambuf_iterator<char>(std::cin)),
                         std::istreambuf_iterator<char>());
    } else {
        auto original_size_opt = lacuna::utils::get_file_size(input_path);
        if (!original_size_opt) {
            std::cerr << icon_error() << " "
                      << red("Input file does not exist or is not a regular file: ") << input_path
                      << "\n";
            return false;
        }

        std::ifstream in(input_path, std::ios::binary);
        if (!in) {
            std::cerr << icon_error() << " " << red("Failed to open input file for reading: ")
                      << input_path << "\n";
            return false;
        }
        file_data.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }

    uint8_t selected_algo = 0;
    std::string algo_name;
    std::string serialized_data;

    auto start_time = std::chrono::high_resolution_clock::now();

    if (opt.algorithm && *opt.algorithm != "auto") {
        std::string_view algo_str = *opt.algorithm;
        if (algo_str == "rle") {
            selected_algo = 0x01;
            algo_name = "RLE";
        } else if (algo_str == "huffman") {
            selected_algo = 0x02;
            algo_name = "Huffman";
        } else {
            std::cerr << icon_error() << " " << red("Unknown algorithm: '") << algo_str << "'\n";
            std::string suggestion = find_suggestion(algo_str, KNOWN_ALGOS);
            if (!suggestion.empty()) {
                std::cerr << "  " << dim("Did you mean: ") << yellow(suggestion) << "?\n";
            }
            return false;
        }

        lacuna::utils::TrialResult result = lacuna::utils::run_trial(file_data, selected_algo);
        if (!result.success) {
            std::cerr << icon_error() << " " << red("Compression failed for: ") << input_path
                      << "\n";
            return false;
        }
        serialized_data = std::move(result.serialized_data);
    } else {
        // Auto-select optimal algorithm
        lacuna::utils::TrialResult rle_result = lacuna::utils::run_trial(file_data, 0x01);
        lacuna::utils::TrialResult huff_result = lacuna::utils::run_trial(file_data, 0x02);

        if (!rle_result.success && !huff_result.success) {
            std::cerr << icon_error() << " " << red("Compression trials failed for: ") << input_path
                      << "\n";
            return false;
        }

        if (huff_result.success &&
            (!rle_result.success || huff_result.compressed_size < rle_result.compressed_size)) {
            selected_algo = 0x02;
            algo_name = "Huffman";
            serialized_data = std::move(huff_result.serialized_data);
        } else {
            selected_algo = 0x01;
            algo_name = "RLE";
            serialized_data = std::move(rle_result.serialized_data);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    // Determine output destination
    std::string out_dest =
        opt.output_path.value_or(input_path == "-" ? "-" : (input_path + ".lac"));

    if (out_dest == "-") {
        std::cout.write(serialized_data.data(),
                        static_cast<std::streamsize>(serialized_data.size()));
        std::cout.flush();
    } else {
        std::ofstream out(out_dest, std::ios::binary);
        if (!out) {
            std::cerr << icon_error() << " " << red("Failed to open output file for writing: ")
                      << out_dest << "\n";
            return false;
        }
        out.write(serialized_data.data(), static_cast<std::streamsize>(serialized_data.size()));
        if (!out) {
            std::cerr << icon_error() << " " << red("Failed to write compressed data to: ")
                      << out_dest << "\n";
            return false;
        }
    }

    uint64_t orig_sz = file_data.size();
    uint64_t comp_sz = serialized_data.size();
    double ratio =
        (orig_sz > 0)
            ? ((1.0 - static_cast<double>(comp_sz) / static_cast<double>(orig_sz)) * 100.0)
            : 0.0;

    if (opt.json_output) {
        std::cout << "{\n"
                  << "  \"status\": \"success\",\n"
                  << "  \"input\": \"" << (input_path == "-" ? "<stdin>" : input_path) << "\",\n"
                  << "  \"output\": \"" << (out_dest == "-" ? "<stdout>" : out_dest) << "\",\n"
                  << "  \"algorithm\": \"" << algo_name << "\",\n"
                  << "  \"original_bytes\": " << orig_sz << ",\n"
                  << "  \"compressed_bytes\": " << comp_sz << ",\n"
                  << std::fixed << std::setprecision(2) << "  \"ratio_pct\": " << ratio << ",\n"
                  << "  \"duration_ms\": " << elapsed.count() << "\n"
                  << "}\n";
    } else if (!opt.quiet && out_dest != "-") {
        std::cout << icon_success() << " " << bold("Compressed") << " " << cyan(input_path)
                  << " -> " << bold(out_dest) << "\n"
                  << "  " << dim("Algorithm:") << " " << yellow(algo_name) << " " << dim("│ Size:")
                  << " " << format_bytes(orig_sz) << " -> " << format_bytes(comp_sz) << " "
                  << dim("│ Ratio:") << " "
                  << (ratio >= 0 ? green(std::to_string(ratio).substr(0, 5) + "%")
                                 : red(std::to_string(ratio).substr(0, 5) + "%"))
                  << "\n";
        if (opt.verbose) {
            std::cout << "  " << dim("Elapsed:") << " " << elapsed.count() << " ms\n";
        }
    }

    return true;
}

bool handle_decompress(const CliOptions& opt) {
    using namespace lacuna::utils::terminal;
    const std::string& input_path = opt.positional_target;

    if (input_path.empty()) {
        std::cerr << icon_error() << " " << red("Missing target .lac file for decompression.")
                  << "\n";
        print_usage(std::cerr);
        return false;
    }

    std::istream* in_ptr = nullptr;
    std::ifstream file_in;
    if (input_path == "-") {
        in_ptr = &std::cin;
    } else {
        file_in.open(input_path, std::ios::binary);
        if (!file_in) {
            std::cerr << icon_error() << " " << red("Failed to open input file: ") << input_path
                      << "\n";
            return false;
        }
        in_ptr = &file_in;
    }

    auto header_opt = lacuna::utils::read_header(*in_ptr);
    if (!header_opt) {
        std::cerr << icon_error() << " " << red("Invalid or corrupted .lac header in: ")
                  << input_path << "\n";
        return false;
    }

    std::string out_dest;
    if (opt.output_path) {
        out_dest = *opt.output_path;
    } else if (input_path == "-") {
        out_dest = "-";
    } else if (std::string_view(input_path).ends_with(".lac")) {
        out_dest = input_path.substr(0, input_path.size() - 4);
    } else {
        out_dest = input_path + ".decompressed";
    }

    std::ostream* out_ptr = nullptr;
    std::ofstream file_out;
    if (out_dest == "-") {
        out_ptr = &std::cout;
    } else {
        file_out.open(out_dest, std::ios::binary);
        if (!file_out) {
            std::cerr << icon_error() << " " << red("Failed to open destination output file: ")
                      << out_dest << "\n";
            return false;
        }
        out_ptr = &file_out;
    }

    std::unique_ptr<lacuna::core::Compressor> compressor;
    if (header_opt->algorithm_id == 0x01) {
        compressor = std::make_unique<lacuna::core::RleCompressor>();
    } else if (header_opt->algorithm_id == 0x02) {
        compressor = std::make_unique<lacuna::core::HuffmanCompressor>();
    } else {
        std::cerr << icon_error() << " " << red("Unsupported algorithm ID: ")
                  << static_cast<int>(header_opt->algorithm_id) << "\n";
        return false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    auto decompressed_bytes = compressor->decompress(*in_ptr, *out_ptr, header_opt->original_size);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    if (!decompressed_bytes) {
        std::cerr << icon_error() << " " << red("Decompression failed or payload corrupted in: ")
                  << input_path << "\n";
        if (out_dest != "-") {
            file_out.close();
            std::error_code ec;
            std::filesystem::remove(out_dest, ec);
        }
        return false;
    }

    if (opt.json_output) {
        std::cout << "{\n"
                  << "  \"status\": \"success\",\n"
                  << "  \"input\": \"" << (input_path == "-" ? "<stdin>" : input_path) << "\",\n"
                  << "  \"output\": \"" << (out_dest == "-" ? "<stdout>" : out_dest) << "\",\n"
                  << "  \"restored_bytes\": " << *decompressed_bytes << ",\n"
                  << "  \"duration_ms\": " << elapsed.count() << "\n"
                  << "}\n";
    } else if (!opt.quiet && out_dest != "-") {
        std::cout << icon_success() << " " << bold("Decompressed") << " " << cyan(input_path)
                  << " -> " << bold(out_dest) << " " << dim("(")
                  << format_bytes(*decompressed_bytes) << dim(")") << "\n";
        if (opt.verbose) {
            std::cout << "  " << dim("Elapsed:") << " " << elapsed.count() << " ms\n";
        }
    }

    return true;
}

bool handle_info(const CliOptions& opt) {
    using namespace lacuna::utils::terminal;
    const std::string& input_path = opt.positional_target;

    if (input_path.empty()) {
        std::cerr << icon_error() << " " << red("Missing target .lac file for info inspection.")
                  << "\n";
        print_usage(std::cerr);
        return false;
    }

    auto compressed_size_opt = lacuna::utils::get_file_size(input_path);
    if (!compressed_size_opt) {
        std::cerr << icon_error() << " "
                  << red("Compressed file does not exist or is not accessible: ") << input_path
                  << "\n";
        return false;
    }
    uint64_t compressed_size = *compressed_size_opt;

    std::ifstream in(input_path, std::ios::binary);
    if (!in) {
        std::cerr << icon_error() << " " << red("Failed to open file: ") << input_path << "\n";
        return false;
    }

    auto header_opt = lacuna::utils::read_header(in);
    if (!header_opt) {
        std::cerr << icon_error() << " " << red("Invalid or corrupted .lac header in: ")
                  << input_path << "\n";
        return false;
    }

    std::string algo_str = (header_opt->algorithm_id == 0x01)   ? "RLE"
                           : (header_opt->algorithm_id == 0x02) ? "Huffman"
                                                                : "Unknown";

    double ratio = 0.0;
    if (header_opt->original_size > 0) {
        ratio = (1.0 - (static_cast<double>(compressed_size) /
                        static_cast<double>(header_opt->original_size))) *
                100.0;
    }

    if (opt.json_output) {
        std::cout << "{\n"
                  << "  \"file\": \"" << input_path << "\",\n"
                  << "  \"algorithm\": \"" << algo_str << "\",\n"
                  << "  \"version\": " << static_cast<int>(header_opt->version) << ",\n"
                  << "  \"original_size\": " << header_opt->original_size << ",\n"
                  << "  \"compressed_size\": " << compressed_size << ",\n"
                  << std::fixed << std::setprecision(2) << "  \"ratio\": " << ratio << "\n"
                  << "}\n";
    } else {
        std::cout << "Algorithm: " << algo_str << "\n";
        std::cout << "Original Size: " << header_opt->original_size << " bytes\n";
        std::cout << "Compressed Size: " << compressed_size << " bytes\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Ratio: " << ratio << "%\n";
        std::cout << "Version: " << static_cast<int>(header_opt->version) << "\n";
    }

    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    CliOptions opt;
    if (!parse_cli_args(argc, argv, opt)) {
        return 1;
    }

    if (opt.show_version) {
        print_version();
        return 0;
    }

    if (opt.show_help) {
        print_usage(std::cout);
        return 0;
    }

    if (opt.command.empty()) {
        print_onboarding();
        return 0;
    }

    if (opt.command == "compress" || opt.command == "c") {
        return handle_compress(opt) ? 0 : 1;
    }
    if (opt.command == "decompress" || opt.command == "d") {
        return handle_decompress(opt) ? 0 : 1;
    }
    if (opt.command == "info" || opt.command == "i") {
        return handle_info(opt) ? 0 : 1;
    }
    if (opt.command == "benchmark" || opt.command == "bench" || opt.command == "b") {
        std::string dir = opt.positional_target.empty() ? "." : opt.positional_target;
        return lacuna::utils::run_benchmark(dir, opt.json_output, opt.quiet) ? 0 : 1;
    }

    std::cerr << lacuna::utils::terminal::icon_error() << " "
              << lacuna::utils::terminal::red("Unknown command: '") << opt.command << "'\n";

    std::string suggestion = lacuna::utils::terminal::find_suggestion(opt.command, KNOWN_COMMANDS);
    if (!suggestion.empty()) {
        std::cerr << "  " << lacuna::utils::terminal::dim("Did you mean: ")
                  << lacuna::utils::terminal::yellow(suggestion) << "?\n\n";
    } else {
        std::cerr << "\n";
    }

    print_usage(std::cerr);
    return 1;
}
