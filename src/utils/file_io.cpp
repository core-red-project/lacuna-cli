#include "file_io.hpp"

#include <filesystem>
#include <iostream>

namespace lacuna::utils {

bool write_header(std::ostream& out, const Header& header) {
    if (!out) {
        return false;
    }

    // Write Magic Bytes: "LAC\0" (4 bytes)
    out.write(Header::MAGIC.data(), Header::MAGIC.size());

    // Write Version: (1 byte)
    out.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));

    // Write Algorithm ID: (1 byte)
    out.write(reinterpret_cast<const char*>(&header.algorithm_id), sizeof(header.algorithm_id));

    // Write Original Size: uint64_t (8 bytes, little-endian)
    uint64_t size = header.original_size;
    std::array<uint8_t, 8> size_bytes{};
    for (size_t i = 0; i < 8; ++i) {
        size_bytes[i] = static_cast<uint8_t>((size >> (i * 8)) & 0xFF);
    }
    out.write(reinterpret_cast<const char*>(size_bytes.data()), size_bytes.size());

    return out.good();
}

std::optional<Header> read_header(std::istream& in) {
    if (!in) {
        return std::nullopt;
    }

    // Read and verify Magic Bytes
    std::array<char, 4> magic_buf{};
    in.read(magic_buf.data(), magic_buf.size());
    if (in.gcount() != static_cast<std::streamsize>(magic_buf.size()) ||
        magic_buf != Header::MAGIC) {
        return std::nullopt;
    }

    // Read Version
    uint8_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (in.gcount() != sizeof(version) || version != Header::CURRENT_VERSION) {
        return std::nullopt;
    }

    // Read Algorithm ID
    uint8_t algo = 0;
    in.read(reinterpret_cast<char*>(&algo), sizeof(algo));
    if (in.gcount() != sizeof(algo) || (algo != 0x01 && algo != 0x02)) {
        return std::nullopt;
    }

    // Read Original Size (8 bytes, little-endian reconstruction)
    std::array<uint8_t, 8> size_bytes{};
    in.read(reinterpret_cast<char*>(size_bytes.data()), size_bytes.size());
    if (in.gcount() != static_cast<std::streamsize>(size_bytes.size())) {
        return std::nullopt;
    }

    Header header;
    header.version = version;
    header.algorithm_id = algo;
    header.original_size = 0;
    for (size_t i = 0; i < 8; ++i) {
        header.original_size |= (static_cast<uint64_t>(size_bytes[i]) << (i * 8));
    }

    return header;
}

std::optional<uint64_t> get_file_size(std::string_view filepath) {
    try {
        std::filesystem::path p{filepath};
        if (std::filesystem::is_regular_file(p)) {
            return std::filesystem::file_size(p);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "File system error: " << e.what() << "\n";
    }
    return std::nullopt;
}

} // namespace lacuna::utils
