#include "bitart_stage.h"

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

// BitArt stage
// ------------------------------------------------------------

Envelope<ByteBuffer> BitArtStage::run(Envelope<ByteBuffer> env) const {
    if (env.ctx == nullptr) {
        throw DiagnosticError{"bitart", "missing pipeline context"};
    }

    auto& ctx = *env.ctx;

    const std::string mode =
        ctx.policies.getStringOrDefault(
            "bitart.mode",
            "pack",
            ctx.diagnostics,
            "bitart"
        );

    const int width =
        ctx.policies.getIntOrDefault(
            "bitart.width",
            8,
            ctx.diagnostics,
            "bitart"
        );

    const std::string oneText =
        ctx.policies.getStringOrDefault(
            "bitart.one",
            "#",
            ctx.diagnostics,
            "bitart"
        );

    const std::string zeroText =
        ctx.policies.getStringOrDefault(
            "bitart.zero",
            ".",
            ctx.diagnostics,
            "bitart"
        );

    const bool strict =
        ctx.policies.getBoolOrDefault(
            "bitart.strict",
            true,
            ctx.diagnostics,
            "bitart"
        );

    const bool ignoreWhitespace =
        ctx.policies.getBoolOrDefault(
            "bitart.ignore_whitespace",
            true,
            ctx.diagnostics,
            "bitart"
        );

    const char oneChar = oneText.empty() ? '#' : oneText[0];
    const char zeroChar = zeroText.empty() ? '.' : zeroText[0];

    if (oneText.size() != 1) {
        ctx.diagnostics.push_back({
            Severity::Warning,
            "bitart",
            "bitart.one should be one character; using first character"
        });
    }

    if (zeroText.size() != 1) {
        ctx.diagnostics.push_back({
            Severity::Warning,
            "bitart",
            "bitart.zero should be one character; using first character"
        });
    }

    if (mode == "pack") {
        return Envelope<ByteBuffer>{
            ByteBuffer{
                pack(
                    env.payload.bytes,
                    width,
                    oneChar,
                    zeroChar,
                    strict,
                    ignoreWhitespace,
                    ctx.diagnostics
                )
            },
            env.ctx
        };
    }

    if (mode == "unpack") {
        return Envelope<ByteBuffer>{
            ByteBuffer{
                unpack(
                    env.payload.bytes,
                    oneChar,
                    zeroChar,
                    strict,
                    ctx.diagnostics
                )
            },
            env.ctx
        };
    }

    ctx.diagnostics.push_back({
        Severity::Warning,
        "bitart",
        "unknown bitart.mode '" + mode + "'; using pack"
    });

    return Envelope<ByteBuffer>{
        ByteBuffer{
            pack(
                env.payload.bytes,
                width,
                oneChar,
                zeroChar,
                strict,
                ignoreWhitespace,
                ctx.diagnostics
            )
        },
        env.ctx
    };
}

std::string BitArtStage::pack(
    const std::string& input,
    int width,
    char oneChar,
    char zeroChar,
    bool strict,
    bool ignoreWhitespace,
    std::vector<Diagnostic>& diagnostics
) {
    if (width <= 0) {
        diagnostics.push_back({
            Severity::Warning,
            "bitart",
            "bitart.width must be positive; using 8"
        });

        width = 8;
    }

    if (width > 65535) {
        throw DiagnosticError{"bitart", "width too large for BART v1 header"};
    }

    std::vector<bool> bits;
    bits.reserve(input.size());

    std::uint32_t height = 0;
    int column = 0;
    bool sawContentOnLine = false;

    for (unsigned char raw : input) {
        const char c = static_cast<char>(raw);

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            if (sawContentOnLine || column > 0) {
                if (column != width) {
                    const std::string message =
                        "line width was " + std::to_string(column) +
                        ", expected " + std::to_string(width);

                    if (strict) {
                        throw DiagnosticError{"bitart", message};
                    }

                    diagnostics.push_back({
                        Severity::Warning,
                        "bitart",
                        message + "; continuing"
                    });
                }

                ++height;
                column = 0;
                sawContentOnLine = false;
            }

            continue;
        }

        if (std::isspace(raw)) {
            if (ignoreWhitespace) {
                continue;
            }

            if (strict) {
                throw DiagnosticError{"bitart", "unexpected whitespace in art input"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "bitart",
                "ignored whitespace in art input"
            });

            continue;
        }

        if (c == oneChar) {
            bits.push_back(true);
            ++column;
            sawContentOnLine = true;
        } else if (c == zeroChar) {
            bits.push_back(false);
            ++column;
            sawContentOnLine = true;
        } else {
            if (strict) {
                throw DiagnosticError{
                    "bitart",
                    std::string{"invalid art character: '"} + c + "'"
                };
            }

            diagnostics.push_back({
                Severity::Warning,
                "bitart",
                std::string{"ignored invalid art character: '"} + c + "'"
            });

            continue;
        }

        if (column == width) {
            ++height;
            column = 0;
            sawContentOnLine = false;
        } else if (column > width) {
            if (strict) {
                throw DiagnosticError{"bitart", "line exceeded configured width"};
            }

            diagnostics.push_back({
                Severity::Warning,
                "bitart",
                "line exceeded configured width; continuing"
            });
        }
    }

    if (column > 0 || sawContentOnLine) {
        if (column != width) {
            const std::string message =
                "final line width was " + std::to_string(column) +
                ", expected " + std::to_string(width);

            if (strict) {
                throw DiagnosticError{"bitart", message};
            }

            diagnostics.push_back({
                Severity::Warning,
                "bitart",
                message + "; padding with zeros"
            });

            while (column < width) {
                bits.push_back(false);
                ++column;
            }
        }

        ++height;
    }

    if (height > 65535) {
        throw DiagnosticError{"bitart", "height too large for BART v1 header"};
    }

    if (bits.size() > 0xFFFFFFFFULL) {
        throw DiagnosticError{"bitart", "bit count too large for BART v1 header"};
    }

    return writeBart(
        static_cast<std::uint16_t>(width),
        static_cast<std::uint16_t>(height),
        bits
    );
}

std::string BitArtStage::unpack(
    const std::string& input,
    char oneChar,
    char zeroChar,
    bool strict,
    std::vector<Diagnostic>& diagnostics
) {
    const BitArtHeader header = readHeader(input);

    const std::size_t headerSize = 14;
    const std::size_t payloadSize = input.size() - headerSize;
    const std::size_t neededBytes = (header.bitCount + 7U) / 8U;

    if (payloadSize < neededBytes) {
        throw DiagnosticError{"bitart", "BART payload shorter than header declares"};
    }

    if (payloadSize > neededBytes) {
        const std::string message = "BART payload has trailing bytes";

        if (strict) {
            throw DiagnosticError{"bitart", message};
        }

        diagnostics.push_back({
            Severity::Warning,
            "bitart",
            message + "; ignoring trailing bytes"
        });
    }

    if (header.width == 0) {
        throw DiagnosticError{"bitart", "BART header width is zero"};
    }

    if (header.height == 0 && header.bitCount != 0) {
        throw DiagnosticError{"bitart", "BART header height is zero but bit_count is nonzero"};
    }

    const std::uint32_t expectedBits =
        static_cast<std::uint32_t>(header.width) *
        static_cast<std::uint32_t>(header.height);

    if (expectedBits != header.bitCount) {
        const std::string message =
            "BART width*height does not match bit_count";

        if (strict) {
            throw DiagnosticError{"bitart", message};
        }

        diagnostics.push_back({
            Severity::Warning,
            "bitart",
            message + "; rendering bit_count bits"
        });
    }

    std::string output;
    output.reserve(static_cast<std::size_t>(header.bitCount) + header.height);

    std::uint32_t renderedBits = 0;

    for (std::uint32_t i = 0; i < header.bitCount; ++i) {
        const std::size_t byteIndex = headerSize + (i / 8U);
        const unsigned int bitInByte = 7U - (i % 8U);

        const unsigned char byte =
            static_cast<unsigned char>(input[byteIndex]);

        const bool bit = ((byte >> bitInByte) & 0x01U) != 0;

        output.push_back(bit ? oneChar : zeroChar);
        ++renderedBits;

        if (renderedBits % header.width == 0) {
            output.push_back('\n');
        }
    }

    return output;
}

std::string BitArtStage::writeBart(
    std::uint16_t width,
    std::uint16_t height,
    const std::vector<bool>& bits
) {
    std::string output;

    output.reserve(14 + ((bits.size() + 7) / 8));

    output.push_back('B');
    output.push_back('A');
    output.push_back('R');
    output.push_back('T');

    output.push_back(static_cast<char>(1)); // version
    output.push_back(static_cast<char>(0)); // flags

    appendU16Le(output, width);
    appendU16Le(output, height);
    appendU32Le(output, static_cast<std::uint32_t>(bits.size()));

    unsigned char currentByte = 0;
    int bitPosition = 7;

    for (bool bit : bits) {
        if (bit) {
            currentByte |= static_cast<unsigned char>(1U << bitPosition);
        }

        --bitPosition;

        if (bitPosition < 0) {
            output.push_back(static_cast<char>(currentByte));
            currentByte = 0;
            bitPosition = 7;
        }
    }

    if (bitPosition != 7) {
        output.push_back(static_cast<char>(currentByte));
    }

    return output;
}

BitArtHeader BitArtStage::readHeader(const std::string& input) {
    if (input.size() < 14) {
        throw DiagnosticError{"bitart", "input too short for BART header"};
    }

    if (
        input[0] != 'B' ||
        input[1] != 'A' ||
        input[2] != 'R' ||
        input[3] != 'T'
    ) {
        throw DiagnosticError{"bitart", "invalid BART magic"};
    }

    const unsigned char version = static_cast<unsigned char>(input[4]);
    const unsigned char flags = static_cast<unsigned char>(input[5]);

    if (version != 1) {
        throw DiagnosticError{"bitart", "unsupported BART version"};
    }

    if (flags != 0) {
        throw DiagnosticError{"bitart", "unsupported BART flags"};
    }

    return BitArtHeader{
        readU16Le(input, 6),
        readU16Le(input, 8),
        readU32Le(input, 10)
    };
}

void BitArtStage::appendU16Le(std::string& out, std::uint16_t value) {
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>((value >> 8U) & 0xFFU));
}

void BitArtStage::appendU32Le(std::string& out, std::uint32_t value) {
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<char>((value >> 24U) & 0xFFU));
}

std::uint16_t BitArtStage::readU16Le(const std::string& input, std::size_t offset) {
    const auto b0 = static_cast<std::uint16_t>(
        static_cast<unsigned char>(input[offset])
    );

    const auto b1 = static_cast<std::uint16_t>(
        static_cast<unsigned char>(input[offset + 1])
    );

    return static_cast<std::uint16_t>(b0 | (b1 << 8U));
}

std::uint32_t BitArtStage::readU32Le(const std::string& input, std::size_t offset) {
    const auto b0 = static_cast<std::uint32_t>(
        static_cast<unsigned char>(input[offset])
    );

    const auto b1 = static_cast<std::uint32_t>(
        static_cast<unsigned char>(input[offset + 1])
    );

    const auto b2 = static_cast<std::uint32_t>(
        static_cast<unsigned char>(input[offset + 2])
    );

    const auto b3 = static_cast<std::uint32_t>(
        static_cast<unsigned char>(input[offset + 3])
    );

    return b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U);
}
