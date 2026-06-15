#pragma once

#include "compressor.hpp"
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

namespace lacuna::core {

/**
 * @brief Node structure representing the Huffman Tree.
 */
struct HuffmanNode {
    uint8_t value{0};
    uint64_t frequency{0};
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;

    HuffmanNode(uint8_t val, uint64_t freq) : value(val), frequency(freq) {}
    HuffmanNode(uint64_t freq, std::unique_ptr<HuffmanNode> l, std::unique_ptr<HuffmanNode> r)
        : frequency(freq), left(std::move(l)), right(std::move(r)) {}

    bool is_leaf() const { return !left && !right; }
};

/**
 * @brief Deterministic comparator for Huffman tree nodes.
 */
struct HuffmanNodeComparator {
    bool operator()(const std::unique_ptr<HuffmanNode>& a,
                    const std::unique_ptr<HuffmanNode>& b) const {
        if (a->frequency != b->frequency) {
            return a->frequency > b->frequency; // Min-heap (smallest frequency first)
        }
        return get_min_value(*a) > get_min_value(*b);
    }

private:
    static uint8_t get_min_value(const HuffmanNode& node) {
        if (node.is_leaf()) {
            return node.value;
        }
        uint8_t min_l = get_min_value(*node.left);
        uint8_t min_r = get_min_value(*node.right);
        return std::min(min_l, min_r);
    }
};

/**
 * @brief High-performance bit-level Huffman Coding compressor.
 */
class HuffmanCompressor : public Compressor {
public:
    HuffmanCompressor() = default;
    ~HuffmanCompressor() override = default;

    bool compress(std::istream& in, std::ostream& out) override;
    std::optional<uint64_t> decompress(std::istream& in,
                                       std::ostream& out,
                                       uint64_t expected_size) override;
};

} // namespace lacuna::core
