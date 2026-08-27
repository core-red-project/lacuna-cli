#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace lacuna::utils::terminal {

/**
 * @brief Checks if ANSI color codes should be emitted.
 * Respects NO_COLOR, CLICOLOR_FORCE, TERM=dumb, and TTY status.
 */
bool is_color_enabled(const std::ostream& os = std::cout);

// Style codes
std::string reset();
std::string bold(std::string_view text);
std::string dim(std::string_view text);
std::string red(std::string_view text);
std::string green(std::string_view text);
std::string yellow(std::string_view text);
std::string blue(std::string_view text);
std::string magenta(std::string_view text);
std::string cyan(std::string_view text);
std::string gray(std::string_view text);

// Standard status indicators
std::string icon_success();
std::string icon_error();
std::string icon_info();
std::string icon_sparkle();

/**
 * @brief Formats raw bytes into human-readable representation (e.g. "1.42 KB", "250 B").
 */
std::string format_bytes(uint64_t bytes);

/**
 * @brief Computes Levenshtein edit distance between two strings.
 */
size_t levenshtein_distance(std::string_view s1, std::string_view s2);

/**
 * @brief Finds the best match suggestion if within a reasonable edit distance threshold.
 */
std::string find_suggestion(std::string_view input,
                            const std::vector<std::string_view>& candidates);

} // namespace lacuna::utils::terminal
