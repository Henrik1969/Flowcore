// pipe_hex.cpp
//
// Flow Policy Envelope Pattern - Hex encode/decode example
//
// Build:
//   g++ -std=c++17 -Wall -Wextra -pedantic pipe_hex.cpp -o flowhex
//
// Usage:
//   cat file.bin | ./flowhex --mode encode > file.hex
//   cat file.hex | ./flowhex --mode decode > restored.bin
//
// Test:
//   echo "Hello World" | ./flowhex --mode encode
//   echo "48 65 6C 6C 6F 20 57 6F 72 6C 64 0A" | ./flowhex --mode decode
//
// Roundtrip:
//   echo "Hello World" | ./flowhex --mode encode | ./flowhex --mode decode
//
// Policy options:
//   --mode encode|decode
//   --width N
//   --uppercase true|false
//   --strict true|false
//   --ignore-whitespace true|false

#include <algorithm>
#include <cctype>
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

class HexStage {
public:
    [[nodiscard]] Envelope<ByteBuffer> run(Envelope<ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string encode(
        const std::string& input,
        int width,
        bool uppercase
    );

    [[nodiscard]] static std::string decode(
        const std::string& input,
        bool strict,
        bool ignoreWhitespace,
        std::vector<Diagnostic>& diagnostics
    );

    [[nodiscard]] static int hexValue(unsigned char c);
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

void loadPoliciesFromArgs(int argc,char** argv, PolicyBag& policies);
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
    policies.set("hex.mode", std::string{"encode"});
    policies.set("hex.width", 80);
    policies.set("hex.uppercase", true);
    policies.set("hex.strict", true);
    policies.set("hex.ignore_whitespace", true);

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
            policies.set("hex.mode", requireValue(arg));
        } else if (arg == "--width") {
            policies.set("hex.width", parseInt(requireValue(arg)));
        } else if (arg == "--uppercase") {
            policies.set("hex.uppercase", parseBool(requireValue(arg)));
        } else if (arg == "--strict") {
            policies.set("hex.strict", parseBool(requireValue(arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("hex.ignore_whitespace", parseBool(requireValue(arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  flowhex [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode encode|decode\n"
                << "  --width N\n"
                << "  --uppercase true|false\n"
                << "  --strict true|false\n"
                << "  --ignore-whitespace true|false\n\n"
                << "Examples:\n"
                << "  echo \"Hello\" | ./flowhex --mode encode\n"
                << "  echo \"48 65 6C 6C 6F 0A\" | ./flowhex --mode decode\n"
                << "  cat file.bin | ./flowhex --mode encode --width 80 > file.hex\n"
                << "  cat file.hex | ./flowhex --mode decode > restored.bin\n";
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
        HexStage hex;
        StdoutSink stdoutSink(std::cout);

        stdinProducer
            >> hex
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
