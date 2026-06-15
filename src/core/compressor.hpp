#pragma once

#include <cstdint>
#include <iostream>
#include <optional>

namespace lacuna::core {

/**
 * @brief Pure virtual interface for all compression algorithms.
 */
class Compressor {
public:
    virtual ~Compressor() = default;

    /**
     * @brief Compresses the input binary stream and writes the payload to the output stream.
     * @param in Input binary stream.
     * @param out Output binary stream.
     * @return true on success, false on failure.
     */
    virtual bool compress(std::istream& in, std::ostream& out) = 0;

    /**
     * @brief Decompresses the input binary stream and writes raw bytes to the output stream.
     * @param in Input binary stream.
     * @param out Output binary stream.
     * @param expected_size The uncompressed size expected in bytes.
     * @return The total decompressed bytes on success, std::nullopt on corruption or error.
     */
    virtual std::optional<uint64_t>
    decompress(std::istream& in, std::ostream& out, uint64_t expected_size) = 0;
};

} // namespace lacuna::core
