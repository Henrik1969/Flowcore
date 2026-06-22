// pipe_bitart.cpp
//
// Flow Policy Envelope Pattern - BitArt pack/unpack example
//
// Build:
//   g++ -std=c++17 -Wall -Wextra -pedantic pipe_bitart.cpp -o bitart
//
// Usage:
//   cat art.txt  | ./bitart --mode pack   --width 8 > art.bart
//   cat art.bart | ./bitart --mode unpack > restored.txt
//
// Test art.txt:
//   ..##....
//   .####...
//   ######..
//   .####...
//   ..##....
//
// Roundtrip:
//   cat art.txt | ./bitart --mode pack --width 8 > art.bart
//   cat art.bart | ./bitart --mode unpack > restored.txt
//   diff art.txt restored.txt
//
// Policy options:
//   --mode pack|unpack
//   --width N
//   --one "#"
//   --zero "."
//   --strict true|false
//   --ignore-whitespace true|false
//
// BART v1 binary ABI:
//
//   offset size field
//   0      4    magic: "BART"
//   4      1    version: 1
//   5      1    flags: reserved, currently 0
//   6      2    width, little-endian
//   8      2    height, little-endian
//   10     4    bit_count, little-endian
//   14     N    packed bit payload, MSB-first inside each byte

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// ============================================================
// Declarations
// ============================================================

enum class Severity {
    Info,
    Warning,
    Error,
    Fatal
};

struct Diagnostic {
    Severity severity;
    std::string stage;
    std::string message;
};

class DiagnosticError : public std::exception {
public:
    DiagnosticError(std::string stage, std::string message);

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const std::string& stage() const noexcept;

private:
    std::string stage_;
    std::string message_;
};

using PolicyValue = std::variant<bool, int, std::string>;

class PolicyBag {
public:
    void set(std::string key, PolicyValue value);

    [[nodiscard]] bool getBoolOrDefault(
        const std::string& key,
        bool fallback,
        std::vector<Diagnostic>& diagnostics,
        const std::string& stage
    ) const;

    [[nodiscard]] int getIntOrDefault(
        const std::string& key,
        int fallback,
        std::vector<Diagnostic>& diagnostics,
        const std::string& stage
    ) const;

    [[nodiscard]] std::string getStringOrDefault(
        const std::string& key,
        std::string fallback,
        std::vector<Diagnostic>& diagnostics,
        const std::string& stage
    ) const;

private:
    std::map<std::string, PolicyValue> values_;
};

struct PipelineContext {
    PolicyBag policies;
    std::vector<Diagnostic> diagnostics;
};

struct ByteBuffer {
    std::string bytes;
};

template <typename Payload>
struct Envelope {
    Payload payload;
    PipelineContext* ctx = nullptr;
};

class StdinProducer {
public:
    explicit StdinProducer(PipelineContext& ctx);

    [[nodiscard]] Envelope<ByteBuffer> produce();

private:
    PipelineContext* ctx_;
};

struct BitArtHeader {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint32_t bitCount = 0;
};

class BitArtStage {
public:
    [[nodiscard]] Envelope<ByteBuffer> run(Envelope<ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string pack(
        const std::string& input,
        int width,
        char oneChar,
        char zeroChar,
        bool strict,
        bool ignoreWhitespace,
        std::vector<Diagnostic>& diagnostics
    );

    [[nodiscard]] static std::string unpack(
        const std::string& input,
        char oneChar,
        char zeroChar,
        bool strict,
        std::vector<Diagnostic>& diagnostics
    );

    [[nodiscard]] static std::string writeBart(
        std::uint16_t width,
        std::uint16_t height,
        const std::vector<bool>& bits
    );

    [[nodiscard]] static BitArtHeader readHeader(const std::string& input);

    static void appendU16Le(std::string& out, std::uint16_t value);
    static void appendU32Le(std::string& out, std::uint32_t value);

    [[nodiscard]] static std::uint16_t readU16Le(const std::string& input, std::size_t offset);
    [[nodiscard]] static std::uint32_t readU32Le(const std::string& input, std::size_t offset);
};

class StdoutSink {
public:
    explicit StdoutSink(std::ostream& out);

    void run(Envelope<ByteBuffer> env) const;

private:
    std::ostream* out_;
};

class LogSink {
public:
    explicit LogSink(std::ostream& err);

    void write(const PipelineContext& ctx) const;
    void writeFatal(const DiagnosticError& err) const;

private:
    [[nodiscard]] static const char* severityName(Severity severity);

    std::ostream* err_;
};

// Pipeline operators

template <typename Producer, typename Stage>
auto operator>>(Producer& producer, const Stage& stage)
    -> decltype(stage.run(producer.produce()));

template <typename Payload, typename Stage>
auto operator>>(Envelope<Payload> env, const Stage& stage)
    -> decltype(stage.run(std::move(env)));

template <typename Payload>
void operator>>(Envelope<Payload> env, const StdoutSink& sink);

// CLI policy loading

void loadPoliciesFromArgs(int argc, char** argv, PolicyBag& policies);
bool parseBool(const std::string& value);
int parseInt(const std::string& value);

// ============================================================
// Implementations
// ============================================================

DiagnosticError::DiagnosticError(std::string stage, std::string message)
    : stage_(std::move(stage)),
      message_(std::move(message)) {}

const char* DiagnosticError::what() const noexcept {
    return message_.c_str();
}

const std::string& DiagnosticError::stage() const noexcept {
    return stage_;
}

// ------------------------------------------------------------
// PolicyBag
// ------------------------------------------------------------

void PolicyBag::set(std::string key, PolicyValue value) {
    values_[std::move(key)] = std::move(value);
}

bool PolicyBag::getBoolOrDefault(
    const std::string& key,
    bool fallback,
    std::vector<Diagnostic>& diagnostics,
    const std::string& stage
) const {
    const auto it = values_.find(key);

    if (it == values_.end()) {
        return fallback;
    }

    if (const auto* value = std::get_if<bool>(&it->second)) {
        return *value;
    }

    diagnostics.push_back({
        Severity::Warning,
        stage,
        "policy '" + key + "' expected bool; using fallback"
    });

    return fallback;
}

int PolicyBag::getIntOrDefault(
    const std::string& key,
    int fallback,
    std::vector<Diagnostic>& diagnostics,
    const std::string& stage
) const {
    const auto it = values_.find(key);

    if (it == values_.end()) {
        return fallback;
    }

    if (const auto* value = std::get_if<int>(&it->second)) {
        return *value;
    }

    diagnostics.push_back({
        Severity::Warning,
        stage,
        "policy '" + key + "' expected int; using fallback"
    });

    return fallback;
}

std::string PolicyBag::getStringOrDefault(
    const std::string& key,
    std::string fallback,
    std::vector<Diagnostic>& diagnostics,
    const std::string& stage
) const {
    const auto it = values_.find(key);

    if (it == values_.end()) {
        return fallback;
    }

    if (const auto* value = std::get_if<std::string>(&it->second)) {
        return *value;
    }

    diagnostics.push_back({
        Severity::Warning,
        stage,
        "policy '" + key + "' expected string; using fallback"
    });

    return fallback;
}

// ------------------------------------------------------------
// Producer
// ------------------------------------------------------------

StdinProducer::StdinProducer(PipelineContext& ctx)
    : ctx_(&ctx) {}

Envelope<ByteBuffer> StdinProducer::produce() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();

    return Envelope<ByteBuffer>{
        ByteBuffer{buffer.str()},
        ctx_
    };
}

// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Sinks
// ------------------------------------------------------------

StdoutSink::StdoutSink(std::ostream& out)
    : out_(&out) {}

void StdoutSink::run(Envelope<ByteBuffer> env) const {
    if (out_ == nullptr) {
        throw DiagnosticError{"stdout-sink", "missing output stream"};
    }

    // Binary-safe output.
    out_->write(
        env.payload.bytes.data(),
        static_cast<std::streamsize>(env.payload.bytes.size())
    );
}

LogSink::LogSink(std::ostream& err)
    : err_(&err) {}

void LogSink::write(const PipelineContext& ctx) const {
    for (const auto& diagnostic : ctx.diagnostics) {
        *err_
            << severityName(diagnostic.severity)
            << " in "
            << diagnostic.stage
            << ": "
            << diagnostic.message
            << '\n';
    }
}

void LogSink::writeFatal(const DiagnosticError& err) const {
    *err_
        << "fatal in "
        << err.stage()
        << ": "
        << err.what()
        << '\n';
}

const char* LogSink::severityName(Severity severity) {
    switch (severity) {
        case Severity::Info:    return "info";
        case Severity::Warning: return "warning";
        case Severity::Error:   return "error";
        case Severity::Fatal:   return "fatal";
    }

    return "unknown";
}

// ------------------------------------------------------------
// Pipeline operators
// ------------------------------------------------------------

template <typename Producer, typename Stage>
auto operator>>(Producer& producer, const Stage& stage)
    -> decltype(stage.run(producer.produce())) {
    return stage.run(producer.produce());
}

template <typename Payload, typename Stage>
auto operator>>(Envelope<Payload> env, const Stage& stage)
    -> decltype(stage.run(std::move(env))) {
    return stage.run(std::move(env));
}

template <typename Payload>
void operator>>(Envelope<Payload> env, const StdoutSink& sink) {
    sink.run(std::move(env));
}

// ------------------------------------------------------------
// CLI policy loading
// ------------------------------------------------------------

void loadPoliciesFromArgs(int argc, char** argv, PolicyBag& policies) {
    // Defaults
    policies.set("bitart.mode", std::string{"pack"});
    policies.set("bitart.width", 8);
    policies.set("bitart.one", std::string{"#"});
    policies.set("bitart.zero", std::string{"."});
    policies.set("bitart.strict", true);
    policies.set("bitart.ignore_whitespace", true);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto requireValue = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                throw DiagnosticError{"cli", "missing value after " + option};
            }

            ++i;
            return argv[i];
        };

        if (arg == "--mode") {
            policies.set("bitart.mode", requireValue(arg));
        } else if (arg == "--width") {
            policies.set("bitart.width", parseInt(requireValue(arg)));
        } else if (arg == "--one") {
            policies.set("bitart.one", requireValue(arg));
        } else if (arg == "--zero") {
            policies.set("bitart.zero", requireValue(arg));
        } else if (arg == "--strict") {
            policies.set("bitart.strict", parseBool(requireValue(arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("bitart.ignore_whitespace", parseBool(requireValue(arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  bitart [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode pack|unpack\n"
                << "  --width N\n"
                << "  --one CHAR\n"
                << "  --zero CHAR\n"
                << "  --strict true|false\n"
                << "  --ignore-whitespace true|false\n\n"
                << "Examples:\n"
                << "  cat art.txt  | ./bitart --mode pack --width 8 > art.bart\n"
                << "  cat art.bart | ./bitart --mode unpack > restored.txt\n"
                << "  cat art.txt  | ./bitart --mode pack --width 8 | ./bitart --mode unpack\n";
            std::exit(EXIT_SUCCESS);
        } else {
            throw DiagnosticError{"cli", "unknown argument: " + arg};
        }
    }
}

bool parseBool(const std::string& value) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }

    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }

    throw DiagnosticError{"cli", "expected bool, got: " + value};
}

int parseInt(const std::string& value) {
    try {
        std::size_t pos = 0;
        const int result = std::stoi(value, &pos);

        if (pos != value.size()) {
            throw DiagnosticError{"cli", "expected integer, got: " + value};
        }

        return result;
    } catch (const std::invalid_argument&) {
        throw DiagnosticError{"cli", "expected integer, got: " + value};
    } catch (const std::out_of_range&) {
        throw DiagnosticError{"cli", "integer out of range: " + value};
    }
}

// ============================================================
// Orchestration
// ============================================================

int main(int argc, char** argv) {
    PipelineContext ctx;
    LogSink log(std::cerr);

    try {
        loadPoliciesFromArgs(argc, argv, ctx.policies);

        StdinProducer stdinProducer(ctx);
        BitArtStage bitart;
        StdoutSink stdoutSink(std::cout);

        stdinProducer
            >> bitart
            >> stdoutSink;

        log.write(ctx);
    } catch (const DiagnosticError& err) {
        log.writeFatal(err);
        log.write(ctx);
        return EXIT_FAILURE;
    } catch (const std::exception& err) {
        std::cerr << "fatal internal error: " << err.what() << '\n';
        log.write(ctx);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
