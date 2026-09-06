#include "flowmini_utf8.h"

namespace flowmini {
namespace {

[[nodiscard]] bool continuation(unsigned char byte) {
    return (byte & 0xc0U) == 0x80U;
}

void reject(Utf8DecodeResult& result, const char* code, std::size_t offset) {
    result.diagnostics.push_back(Utf8Diagnostic{code, offset});
}

} // namespace

Utf8DecodeResult decodeUtf8(const std::string& bytes) {
    Utf8DecodeResult result;
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const auto first = static_cast<unsigned char>(bytes[offset]);
        std::size_t length = 0;
        std::uint32_t value = 0;
        std::uint32_t minimum = 0;

        if (first <= 0x7fU) {
            length = 1;
            value = first;
            minimum = 0;
        } else if (first >= 0xc2U && first <= 0xdfU) {
            length = 2;
            value = first & 0x1fU;
            minimum = 0x80U;
        } else if (first >= 0xe0U && first <= 0xefU) {
            length = 3;
            value = first & 0x0fU;
            minimum = 0x800U;
        } else if (first >= 0xf0U && first <= 0xf4U) {
            length = 4;
            value = first & 0x07U;
            minimum = 0x10000U;
        } else {
            reject(result, first >= 0x80U && first <= 0xbfU
                ? "unexpected-continuation"
                : "invalid-leading-byte", offset);
            ++offset;
            continue;
        }

        if (offset + length > bytes.size()) {
            reject(result, "truncated-sequence", offset);
            ++offset;
            continue;
        }

        bool valid_continuations = true;
        for (std::size_t index = 1; index < length; ++index) {
            if (!continuation(static_cast<unsigned char>(bytes[offset + index]))) {
                valid_continuations = false;
                break;
            }
            value = (value << 6U)
                | (static_cast<unsigned char>(bytes[offset + index]) & 0x3fU);
        }

        if (!valid_continuations) {
            reject(result, "invalid-continuation", offset);
            ++offset;
            continue;
        }

        if (value < minimum) {
            reject(result, "overlong-sequence", offset);
            ++offset;
            continue;
        }
        if (value > 0x10ffffU) {
            reject(result, "scalar-out-of-range", offset);
            ++offset;
            continue;
        }
        if (value >= 0xd800U && value <= 0xdfffU) {
            reject(result, "surrogate-scalar", offset);
            ++offset;
            continue;
        }

        result.scalars.push_back(Utf8Scalar{value, offset, length});
        offset += length;
    }

    return result;
}

} // namespace flowmini
