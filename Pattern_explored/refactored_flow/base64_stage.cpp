#include "base64_stage.h"

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

// Base64 stage
// ------------------------------------------------------------

Envelope<ByteBuffer> Base64Stage::run(Envelope<ByteBuffer> env) const {
    if (env.ctx == nullptr) {
        throw DiagnosticError{"base64", "missing pipeline context"};
    }

    auto& ctx = *env.ctx;

    const std::string mode =
        ctx.policies.getStringOrDefault(
            "base64.mode",
            "encode",
            ctx.diagnostics,
            "base64"
        );

    const bool strict =
        ctx.policies.getBoolOrDefault(
            "base64.strict",
            true,
            ctx.diagnostics,
            "base64"
        );

    const bool ignoreWhitespace =
        ctx.policies.getBoolOrDefault(
            "base64.ignore_whitespace",
            true,
            ctx.diagnostics,
            "base64"
        );

    const int wrap =
        ctx.policies.getIntOrDefault(
            "base64.wrap",
            0,
            ctx.diagnostics,
            "base64"
        );

    if (mode == "encode") {
        return Envelope<ByteBuffer>{
            ByteBuffer{encode(env.payload.bytes, wrap)},
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
        "base64",
        "unknown base64.mode '" + mode + "'; using encode"
    });

    return Envelope<ByteBuffer>{
        ByteBuffer{encode(env.payload.bytes, wrap)},
        env.ctx
    };
}

std::string Base64Stage::encode(const std::string& input, int wrap) {
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    int lineCount = 0;

    auto pushOutput = [&](char c) {
        output.push_back(c);
        ++lineCount;

        if (wrap > 0 && lineCount >= wrap) {
            output.push_back('\n');
            lineCount = 0;
        }
    };

    for (std::size_t i = 0; i < input.size(); i += 3) {
        const unsigned char b0 = static_cast<unsigned char>(input[i]);
        const bool hasB1 = (i + 1 < input.size());
        const bool hasB2 = (i + 2 < input.size());

        const unsigned char b1 = hasB1 ? static_cast<unsigned char>(input[i + 1]) : 0;
        const unsigned char b2 = hasB2 ? static_cast<unsigned char>(input[i + 2]) : 0;

        pushOutput(table[(b0 >> 2U) & 0x3FU]);
        pushOutput(table[((b0 << 4U) | (b1 >> 4U)) & 0x3FU]);
        pushOutput(hasB1 ? table[((b1 << 2U) | (b2 >> 6U)) & 0x3FU] : '=');
        pushOutput(hasB2 ? table[b2 & 0x3FU] : '=');
    }

    return output;
}

std::string Base64Stage::decode(
    const std::string& input,
    bool strict,
    bool ignoreWhitespace,
    std::vector<Diagnostic>& diagnostics
) {
    std::string clean;
    clean.reserve(input.size());

    for (unsigned char c : input) {
        if (std::isspace(c)) {
            if (ignoreWhitespace) {
                continue;
            }

            if (strict) {
                throw DiagnosticError{"base64", "whitespace found in strict decode mode"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "base64",
                "ignored whitespace in base64 input"
            });

            continue;
        }

        clean.push_back(static_cast<char>(c));
    }

    if (clean.empty()) {
        return {};
    }

    if (clean.size() % 4 != 0) {
        if (strict) {
            throw DiagnosticError{"base64", "input length is not a multiple of 4"};
        }

        diagnostics.push_back({
            Severity::Warning,
            "base64",
            "input length is not a multiple of 4; padding with '='"
        });

        while (clean.size() % 4 != 0) {
            clean.push_back('=');
        }
    }

    std::string output;
    output.reserve((clean.size() / 4) * 3);

    for (std::size_t i = 0; i < clean.size(); i += 4) {
        const char c0 = clean[i];
        const char c1 = clean[i + 1];
        const char c2 = clean[i + 2];
        const char c3 = clean[i + 3];

        const int v0 = decodeChar(static_cast<unsigned char>(c0));
        const int v1 = decodeChar(static_cast<unsigned char>(c1));
        const int v2 = (c2 == '=') ? 0 : decodeChar(static_cast<unsigned char>(c2));
        const int v3 = (c3 == '=') ? 0 : decodeChar(static_cast<unsigned char>(c3));

        if (v0 < 0 || v1 < 0 || (c2 != '=' && v2 < 0) || (c3 != '=' && v3 < 0)) {
            if (strict) {
                throw DiagnosticError{"base64", "invalid character in base64 input"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "base64",
                "invalid base64 block ignored"
            });

            continue;
        }

        const unsigned int triple =
            (static_cast<unsigned int>(v0) << 18U) |
            (static_cast<unsigned int>(v1) << 12U) |
            (static_cast<unsigned int>(v2) << 6U) |
            static_cast<unsigned int>(v3);

        output.push_back(static_cast<char>((triple >> 16U) & 0xFFU));

        if (c2 != '=') {
            output.push_back(static_cast<char>((triple >> 8U) & 0xFFU));
        }

        if (c3 != '=') {
            output.push_back(static_cast<char>(triple & 0xFFU));
        }
    }

    return output;
}

int Base64Stage::decodeChar(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }

    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }

    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }

    if (c == '+') {
        return 62;
    }

    if (c == '/') {
        return 63;
    }

    return -1;
}
