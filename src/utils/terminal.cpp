#include "terminal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace lacuna::utils::terminal {

namespace {

constexpr const char* ANSI_RESET = "\033[0m";
constexpr const char* ANSI_BOLD = "\033[1m";
constexpr const char* ANSI_DIM = "\033[2m";
constexpr const char* ANSI_RED = "\033[31m";
constexpr const char* ANSI_GREEN = "\033[32m";
constexpr const char* ANSI_YELLOW = "\033[33m";
constexpr const char* ANSI_BLUE = "\033[34m";
constexpr const char* ANSI_MAGENTA = "\033[35m";
constexpr const char* ANSI_CYAN = "\033[36m";
constexpr const char* ANSI_GRAY = "\033[90m";

std::string wrap_ansi(std::string_view code, std::string_view text, bool enabled) {
    if (!enabled || text.empty()) {
        return std::string(text);
    }
    return std::string(code) + std::string(text) + ANSI_RESET;
}

} // namespace

bool is_color_enabled(const std::ostream& os) {
    const char* no_color = std::getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') {
        return false;
    }

    const char* force_color = std::getenv("CLICOLOR_FORCE");
    if (force_color && force_color[0] != '0') {
        return true;
    }

    const char* term = std::getenv("TERM");
    if (term && std::string_view(term) == "dumb") {
        return false;
    }

    if (&os == &std::cout) {
        return ISATTY(FILENO(stdout)) != 0;
    }
    if (&os == &std::cerr) {
        return ISATTY(FILENO(stderr)) != 0;
    }

    return false;
}

std::string reset() {
    return is_color_enabled() ? ANSI_RESET : "";
}

std::string bold(std::string_view text) {
    return wrap_ansi(ANSI_BOLD, text, is_color_enabled());
}

std::string dim(std::string_view text) {
    return wrap_ansi(ANSI_DIM, text, is_color_enabled());
}

std::string red(std::string_view text) {
    return wrap_ansi(ANSI_RED, text, is_color_enabled());
}

std::string green(std::string_view text) {
    return wrap_ansi(ANSI_GREEN, text, is_color_enabled());
}

std::string yellow(std::string_view text) {
    return wrap_ansi(ANSI_YELLOW, text, is_color_enabled());
}

std::string blue(std::string_view text) {
    return wrap_ansi(ANSI_BLUE, text, is_color_enabled());
}

std::string magenta(std::string_view text) {
    return wrap_ansi(ANSI_MAGENTA, text, is_color_enabled());
}

std::string cyan(std::string_view text) {
    return wrap_ansi(ANSI_CYAN, text, is_color_enabled());
}

std::string gray(std::string_view text) {
    return wrap_ansi(ANSI_GRAY, text, is_color_enabled());
}

std::string icon_success() {
    return is_color_enabled() ? green("✔") : "[ok]";
}

std::string icon_error() {
    return is_color_enabled() ? red("✖") : "[x]";
}

std::string icon_info() {
    return is_color_enabled() ? cyan("ℹ") : "[i]";
}

std::string icon_sparkle() {
    return is_color_enabled() ? magenta("✦") : "*";
}

std::string format_bytes(uint64_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    }
    double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024.0) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << kb << " KB";
        return ss.str();
    }
    double mb = kb / 1024.0;
    if (mb < 1024.0) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << mb << " MB";
        return ss.str();
    }
    double gb = mb / 1024.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << gb << " GB";
    return ss.str();
}

size_t levenshtein_distance(std::string_view s1, std::string_view s2) {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();

    std::vector<std::vector<size_t>> d(len1 + 1, std::vector<size_t>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i) {
        d[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j) {
        d[0][j] = j;
    }

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost});
        }
    }

    return d[len1][len2];
}

std::string find_suggestion(std::string_view input,
                            const std::vector<std::string_view>& candidates) {
    if (input.empty() || candidates.empty()) {
        return "";
    }

    auto min_dist = static_cast<size_t>(-1);
    std::string best_candidate;

    for (const auto& candidate : candidates) {
        size_t dist = levenshtein_distance(input, candidate);
        if (dist < min_dist) {
            min_dist = dist;
            best_candidate = std::string(candidate);
        }
    }

    // Only suggest if distance is <= 2 or within 40% of length
    size_t threshold = std::max<size_t>(2, input.size() / 2);
    if (min_dist <= threshold) {
        return best_candidate;
    }

    return "";
}

} // namespace lacuna::utils::terminal
