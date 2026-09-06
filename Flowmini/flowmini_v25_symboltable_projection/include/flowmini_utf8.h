#ifndef FLOWMINI_UTF8_H
#define FLOWMINI_UTF8_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace flowmini {

struct Utf8Scalar {
    std::uint32_t value = 0;
    std::size_t byte_offset = 0;
    std::size_t byte_length = 0;
};

struct Utf8Diagnostic {
    std::string code;
    std::size_t byte_offset = 0;
};

struct Utf8DecodeResult {
    std::vector<Utf8Scalar> scalars;
    std::vector<Utf8Diagnostic> diagnostics;

    [[nodiscard]] bool valid() const noexcept { return diagnostics.empty(); }
};

// Strict UTF-8 decoding for captured source bytes. Invalid input is reported
// without throwing and the decoder advances one byte after each malformed
// sequence so callers can inspect all independent failures deterministically.
[[nodiscard]] Utf8DecodeResult decodeUtf8(const std::string& bytes);

} // namespace flowmini

#endif // FLOWMINI_UTF8_H
