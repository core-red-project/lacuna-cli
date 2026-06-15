#pragma once

#include <cstdint>

#include "compressor.hpp"

namespace lacuna::core {

/**
 * @brief Binary Run-Length Encoding (RLE) implementation.
 */
class RleCompressor : public Compressor {
public:
    RleCompressor() = default;
    ~RleCompressor() override = default;

    bool compress(std::istream& in, std::ostream& out) override;
    std::optional<uint64_t>
    decompress(std::istream& in, std::ostream& out, uint64_t expected_size) override;
};

} // namespace lacuna::core
