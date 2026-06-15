#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string_view>

namespace lacuna::utils {

/**
 * @brief Binary Header for Lacuna compressed files (.lac).
 *
 * Layout:
 * - Magic Bytes: 4 bytes ("LAC\0")
 * - Version: 1 byte (0x01)
 * - Algorithm ID: 1 byte (0x01 = Run-Length Encoding)
 * - Original Size: 8 bytes (uint64_t representation of uncompressed file size)
 */
struct Header {
    static constexpr std::array<char, 4> MAGIC = {'L', 'A', 'C', '\0'};
    static constexpr uint8_t CURRENT_VERSION = 0x01;
    static constexpr uint8_t CURRENT_ALGO = 0x01;

    uint8_t version{CURRENT_VERSION};
    uint8_t algorithm_id{CURRENT_ALGO};
    uint64_t original_size{0};
};

/**
 * @brief Serializes and writes the binary header to the given output stream.
 * @param out The destination binary output stream.
 * @param header The header structure to serialize.
 * @return true if successfully written, false otherwise.
 */
bool write_header(std::ostream& out, const Header& header);

/**
 * @brief Reads, deserializes, and validates the binary header from the given input stream.
 * @param in The source binary input stream.
 * @return The parsed Header on success, or std::nullopt if the stream is corrupted/invalid.
 */
std::optional<Header> read_header(std::istream& in);

/**
 * @brief Retrieves the size of a file in bytes.
 * @param filepath The path of the target file.
 * @return The size of the file in bytes, or std::nullopt if the file cannot be accessed.
 */
std::optional<uint64_t> get_file_size(std::string_view filepath);

} // namespace lacuna::utils
