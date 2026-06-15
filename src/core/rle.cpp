#include "rle.hpp"

#include <cstdint>
#include <vector>

namespace lacuna::core {

bool RleCompressor::compress(std::istream& in, std::ostream& out) {
    if (!in || !out) {
        return false;
    }

    constexpr size_t buffer_size = 65536;
    std::vector<char> buffer(buffer_size);

    uint8_t run_val = 0;
    uint8_t run_len = 0;
    bool has_run = false;

    auto write_block = [&out](uint8_t len, uint8_t val) {
        out.put(static_cast<char>(len));
        out.put(static_cast<char>(val));
        return out.good();
    };

    while (in.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || in.gcount() > 0) {
        std::streamsize bytes_read = in.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            auto byte = static_cast<uint8_t>(buffer[i]);
            if (!has_run) {
                run_val = byte;
                run_len = 1;
                has_run = true;
            } else if (byte == run_val) {
                if (run_len == 255) {
                    if (!write_block(run_len, run_val)) {
                        return false;
                    }
                    run_len = 1;
                } else {
                    run_len++;
                }
            } else {
                if (!write_block(run_len, run_val)) {
                    return false;
                }
                run_val = byte;
                run_len = 1;
            }
        }
    }

    if (has_run && run_len > 0) {
        if (!write_block(run_len, run_val)) {
            return false;
        }
    }

    return out.good();
}

std::optional<uint64_t>
RleCompressor::decompress(std::istream& in, std::ostream& out, uint64_t expected_size) {
    if (!in || !out) {
        return std::nullopt;
    }

    if (expected_size == 0) {
        if (in.peek() != EOF) {
            return std::nullopt;
        }
        return 0;
    }

    constexpr size_t buffer_size = 65536;
    std::vector<char> in_buf(buffer_size);
    std::vector<char> out_buf;
    out_buf.reserve(buffer_size);

    uint64_t total_decompressed = 0;
    bool has_count = false;
    uint8_t count = 0;

    auto flush_out_buf = [&out, &out_buf]() {
        if (!out_buf.empty()) {
            out.write(out_buf.data(), static_cast<std::streamsize>(out_buf.size()));
            out_buf.clear();
        }
        return out.good();
    };

    while (in.read(in_buf.data(), static_cast<std::streamsize>(in_buf.size())) || in.gcount() > 0) {
        std::streamsize bytes_read = in.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            auto byte = static_cast<uint8_t>(in_buf[i]);
            if (!has_count) {
                count = byte;
                if (count == 0) {
                    return std::nullopt;
                }
                has_count = true;
            } else {
                uint8_t value = byte;

                if (total_decompressed + count > expected_size) {
                    return std::nullopt;
                }

                for (uint32_t c = 0; c < count; ++c) {
                    out_buf.push_back(static_cast<char>(value));
                    if (out_buf.size() >= buffer_size) {
                        if (!flush_out_buf()) {
                            return std::nullopt;
                        }
                    }
                }
                total_decompressed += count;
                has_count = false;
            }
        }
    }

    if (has_count) {
        return std::nullopt;
    }

    if (!flush_out_buf()) {
        return std::nullopt;
    }

    if (total_decompressed != expected_size) {
        return std::nullopt;
    }

    return total_decompressed;
}

} // namespace lacuna::core
