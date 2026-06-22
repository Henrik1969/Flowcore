#include "hex_stage.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using flow::ByteBuffer;
using flow::Diagnostic;
using flow::DiagnosticError;
using flow::Envelope;
using flow::Severity;

// Hex stage
// ------------------------------------------------------------

Envelope<ByteBuffer> HexStage::run(Envelope<ByteBuffer> env) const {
    if (env.ctx == nullptr) {
        throw DiagnosticError{"hex", "missing pipeline context"};
    }

    auto& ctx = *env.ctx;

    const std::string mode =
        ctx.policies.getStringOrDefault(
            "hex.mode",
            "encode",
            ctx.diagnostics,
            "hex"
        );

    const int width =
        ctx.policies.getIntOrDefault(
            "hex.width",
            80,
            ctx.diagnostics,
            "hex"
        );

    const bool uppercase =
        ctx.policies.getBoolOrDefault(
            "hex.uppercase",
            true,
            ctx.diagnostics,
            "hex"
        );

    const bool strict =
        ctx.policies.getBoolOrDefault(
            "hex.strict",
            true,
            ctx.diagnostics,
            "hex"
        );

    const bool ignoreWhitespace =
        ctx.policies.getBoolOrDefault(
            "hex.ignore_whitespace",
            true,
            ctx.diagnostics,
            "hex"
        );

    if (mode == "encode") {
        return Envelope<ByteBuffer>{
            ByteBuffer{encode(env.payload.bytes, width, uppercase)},
            env.ctx
        };
    }

    if (mode == "decode") {
        return Envelope<ByteBuffer>{
            ByteBuffer{decode(env.payload.bytes, strict, ignoreWhitespace, ctx.diagnostics)},
            env.ctx
        };
    }

    ctx.diagnostics.push_back({
        Severity::Warning,
        "hex",
        "unknown hex.mode '" + mode + "'; using encode"
    });

    return Envelope<ByteBuffer>{
        ByteBuffer{encode(env.payload.bytes, width, uppercase)},
        env.ctx
    };
}

std::string HexStage::encode(
    const std::string& input,
    int width,
    bool uppercase
) {
    const char* digits = uppercase
        ? "0123456789ABCDEF"
        : "0123456789abcdef";

    const int effectiveWidth = width > 0 ? width : 80;

    // "AA BB CC" means each byte is 2 chars plus usually 1 separator.
    // Width 80 gives 27 bytes per line: 27*2 + 26 spaces = 80.
    const int bytesPerLine = std::max(1, (effectiveWidth + 1) / 3);

    std::string output;
    output.reserve(input.size() * 3);

    int byteOnLine = 0;

    for (unsigned char byte : input) {
        if (byteOnLine > 0) {
            output.push_back(' ');
        }

        output.push_back(digits[(byte >> 4U) & 0x0FU]);
        output.push_back(digits[byte & 0x0FU]);

        ++byteOnLine;

        if (byteOnLine >= bytesPerLine) {
            output.push_back('\n');
            byteOnLine = 0;
        }
    }

    if (!output.empty() && output.back() != '\n') {
        output.push_back('\n');
    }

    return output;
}

std::string HexStage::decode(
    const std::string& input,
    bool strict,
    bool ignoreWhitespace,
    std::vector<Diagnostic>& diagnostics
) {
    std::string nibbles;
    nibbles.reserve(input.size());

    bool previousWasZeroBeforeX = false;

    for (unsigned char c : input) {
        if (std::isspace(c)) {
            previousWasZeroBeforeX = false;

            if (ignoreWhitespace) {
                continue;
            }

            if (strict) {
                throw DiagnosticError{"hex", "whitespace found in strict decode mode"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "hex",
                "ignored whitespace in hex input"
            });

            continue;
        }

        // Accept commas as separators in permissive input.
        if (c == ',') {
            previousWasZeroBeforeX = false;
            continue;
        }

        // Accept 0xNN style input by removing the 0 before x.
        if ((c == 'x' || c == 'X') && previousWasZeroBeforeX) {
            if (!nibbles.empty() && nibbles.back() == '0') {
                nibbles.pop_back();
            }

            previousWasZeroBeforeX = false;
            continue;
        }

        const int value = hexValue(c);

        if (value < 0) {
            previousWasZeroBeforeX = false;

            if (strict) {
                throw DiagnosticError{"hex", "invalid character in hex input"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "hex",
                "ignored invalid character in hex input"
            });

            continue;
        }

        nibbles.push_back(static_cast<char>(c));
        previousWasZeroBeforeX = (c == '0');
    }

    if (nibbles.size() % 2 != 0) {
        if (strict) {
            throw DiagnosticError{"hex", "odd number of hex digits"};
        }

        diagnostics.push_back({
            Severity::Warning,
            "hex",
            "odd number of hex digits; dropping final nibble"
        });

        nibbles.pop_back();
    }

    std::string output;
    output.reserve(nibbles.size() / 2);

    for (std::size_t i = 0; i < nibbles.size(); i += 2) {
        const int high = hexValue(static_cast<unsigned char>(nibbles[i]));
        const int low  = hexValue(static_cast<unsigned char>(nibbles[i + 1]));

        output.push_back(static_cast<char>((high << 4) | low));
    }

    return output;
}

int HexStage::hexValue(unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}
