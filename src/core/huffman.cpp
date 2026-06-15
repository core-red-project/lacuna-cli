#include "huffman.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace lacuna::core {

namespace {

struct BitWriter {
    std::ostream& out;
    uint8_t current_byte{0};
    int bit_count{0};

    explicit BitWriter(std::ostream& o) : out(o) {}

    bool write_bit(int bit) {
        current_byte = static_cast<uint8_t>((current_byte << 1) | (bit & 1));
        bit_count++;
        if (bit_count == 8) {
            out.put(static_cast<char>(current_byte));
            current_byte = 0;
            bit_count = 0;
        }
        return out.good();
    }

    bool write_code(const std::vector<bool>& code) {
        for (bool bit : code) {
            if (!write_bit(bit ? 1 : 0)) {
                return false;
            }
        }
        return true;
    }

    bool flush() {
        if (bit_count > 0) {
            current_byte = static_cast<uint8_t>(current_byte << (8 - bit_count));
            out.put(static_cast<char>(current_byte));
            current_byte = 0;
            bit_count = 0;
        }
        return out.good();
    }
};

struct BitReader {
    std::istream& in;
    uint8_t current_byte{0};
    int bit_count{0};

    explicit BitReader(std::istream& i) : in(i) {}

    std::optional<int> read_bit() {
        if (bit_count == 0) {
            char c = 0;
            if (!in.get(c)) {
                return std::nullopt;
            }
            current_byte = static_cast<uint8_t>(c);
            bit_count = 8;
        }
        int bit = (current_byte >> (bit_count - 1)) & 1;
        bit_count--;
        return bit;
    }
};

void generate_codes(const HuffmanNode* node, std::vector<bool>& current_code,
                    std::vector<std::vector<bool>>& codes) {
    if (node->is_leaf()) {
        codes[node->value] = current_code;
        return;
    }
    if (node->left) {
        current_code.push_back(false);
        generate_codes(node->left.get(), current_code, codes);
        current_code.pop_back();
    }
    if (node->right) {
        current_code.push_back(true);
        generate_codes(node->right.get(), current_code, codes);
        current_code.pop_back();
    }
}

std::unique_ptr<HuffmanNode> build_tree(const std::array<uint64_t, 256>& frequencies) {
    std::vector<std::unique_ptr<HuffmanNode>> heap;
    HuffmanNodeComparator cmp;

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            heap.push_back(std::make_unique<HuffmanNode>(static_cast<uint8_t>(i), frequencies[i]));
        }
    }

    std::make_heap(heap.begin(), heap.end(), cmp);

    while (heap.size() > 1) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        auto left = std::move(heap.back());
        heap.pop_back();

        std::pop_heap(heap.begin(), heap.end(), cmp);
        auto right = std::move(heap.back());
        heap.pop_back();

        uint64_t parent_freq = left->frequency + right->frequency;
        auto parent = std::make_unique<HuffmanNode>(parent_freq, std::move(left), std::move(right));
        heap.push_back(std::move(parent));
        std::push_heap(heap.begin(), heap.end(), cmp);
    }

    if (heap.empty()) {
        return nullptr;
    }
    return std::move(heap.front());
}

} // namespace

bool HuffmanCompressor::compress(std::istream& in, std::ostream& out) {
    if (!in || !out) {
        return false;
    }

    // 1. Build frequency map (using array for O(1) performance)
    std::array<uint64_t, 256> frequencies{};
    constexpr size_t buffer_size = 65536;
    std::vector<char> buffer(buffer_size);

    while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
        std::streamsize bytes_read = in.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            uint8_t byte = static_cast<uint8_t>(buffer[i]);
            frequencies[byte]++;
        }
    }

    // Determine unique count
    uint16_t unique_count = 0;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            unique_count++;
        }
    }

    // Serialize frequency table
    // - Unique count: 2 bytes (little-endian)
    uint8_t count_low = static_cast<uint8_t>(unique_count & 0xFF);
    uint8_t count_high = static_cast<uint8_t>((unique_count >> 8) & 0xFF);
    out.put(static_cast<char>(count_low));
    out.put(static_cast<char>(count_high));

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            out.put(static_cast<char>(static_cast<uint8_t>(i)));
            uint64_t freq = frequencies[i];
            for (size_t b = 0; b < 8; ++b) {
                out.put(static_cast<char>((freq >> (b * 8)) & 0xFF));
            }
        }
    }

    if (unique_count <= 1) {
        return out.good();
    }

    // 2. Build Huffman tree & codes
    auto root = build_tree(frequencies);
    if (!root) {
        return false;
    }

    std::vector<std::vector<bool>> codes(256);
    std::vector<bool> current_code;
    generate_codes(root.get(), current_code, codes);

    // 3. Rewind input stream to start compression pass
    in.clear();
    if (!in.seekg(0, std::ios::beg)) {
        return false;
    }

    // 4. Compress data stream
    BitWriter writer(out);
    while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
        std::streamsize bytes_read = in.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            uint8_t byte = static_cast<uint8_t>(buffer[i]);
            if (!writer.write_code(codes[byte])) {
                return false;
            }
        }
    }

    return writer.flush();
}

std::optional<uint64_t> HuffmanCompressor::decompress(std::istream& in, std::ostream& out,
                                                      uint64_t expected_size) {
    if (!in || !out) {
        return std::nullopt;
    }

    if (expected_size == 0) {
        // Read the unique count to ensure the stream is aligned
        char low = 0;
        char high = 0;
        if (!in.get(low) || !in.get(high)) {
            return std::nullopt;
        }
        uint16_t unique_count = static_cast<uint16_t>(
            static_cast<uint8_t>(low) | (static_cast<uint16_t>(static_cast<uint8_t>(high)) << 8));
        if (unique_count != 0 || in.peek() != EOF) {
            return std::nullopt; // Corrupted empty file representation
        }
        return 0;
    }

    // Read unique count
    char low = 0;
    char high = 0;
    if (!in.get(low) || !in.get(high)) {
        return std::nullopt;
    }
    uint16_t unique_count = static_cast<uint16_t>(
        static_cast<uint8_t>(low) | (static_cast<uint16_t>(static_cast<uint8_t>(high)) << 8));
    if (unique_count == 0 || unique_count > 256) {
        return std::nullopt;
    }

    // Deserialize frequency map
    std::array<uint64_t, 256> frequencies{};
    uint64_t sum_frequencies = 0;
    for (uint16_t i = 0; i < unique_count; ++i) {
        char val_char = 0;
        if (!in.get(val_char)) {
            return std::nullopt;
        }
        uint8_t value = static_cast<uint8_t>(val_char);

        uint64_t freq = 0;
        for (size_t b = 0; b < 8; ++b) {
            char f_char = 0;
            if (!in.get(f_char)) {
                return std::nullopt;
            }
            freq |= (static_cast<uint64_t>(static_cast<uint8_t>(f_char)) << (b * 8));
        }
        frequencies[value] = freq;
        sum_frequencies += freq;
    }

    if (sum_frequencies != expected_size) {
        return std::nullopt; // Header uncompressed size mismatch
    }

    if (unique_count == 1) {
        // Special case: Single character file
        uint8_t single_char = 0;
        for (int i = 0; i < 256; ++i) {
            if (frequencies[i] > 0) {
                single_char = static_cast<uint8_t>(i);
                break;
            }
        }
        constexpr size_t buffer_size = 65536;
        std::vector<char> out_buf(std::min(static_cast<size_t>(expected_size), buffer_size),
                                  static_cast<char>(single_char));
        uint64_t remaining = expected_size;
        while (remaining > 0) {
            size_t to_write = std::min(static_cast<size_t>(remaining), out_buf.size());
            out.write(out_buf.data(), to_write);
            if (!out) {
                return std::nullopt;
            }
            remaining -= to_write;
        }
        return expected_size;
    }

    // Reconstruct tree
    auto root = build_tree(frequencies);
    if (!root) {
        return std::nullopt;
    }

    // Decompress payload
    BitReader reader(in);
    constexpr size_t buffer_size = 65536;
    std::vector<char> out_buf;
    out_buf.reserve(buffer_size);

    auto flush_out_buf = [&out, &out_buf]() {
        if (!out_buf.empty()) {
            out.write(out_buf.data(), out_buf.size());
            out_buf.clear();
        }
        return out.good();
    };

    uint64_t decompressed_count = 0;
    while (decompressed_count < expected_size) {
        const HuffmanNode* current = root.get();
        while (!current->is_leaf()) {
            auto bit_opt = reader.read_bit();
            if (!bit_opt) {
                return std::nullopt; // Truncated bitstream
            }
            int bit = *bit_opt;
            if (bit == 0) {
                current = current->left.get();
            } else {
                current = current->right.get();
            }
        }
        out_buf.push_back(static_cast<char>(current->value));
        decompressed_count++;

        if (out_buf.size() >= buffer_size) {
            if (!flush_out_buf()) {
                return std::nullopt;
            }
        }
    }

    if (!flush_out_buf()) {
        return std::nullopt;
    }

    return decompressed_count;
}

} // namespace lacuna::core
