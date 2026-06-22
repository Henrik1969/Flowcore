// flow_policy_envelope_base64.cpp
//
// Minimal Flow Policy Envelope proof-of-concept.
//
// Build:
//   g++ -std=c++17 -Wall -Wextra -pedantic flow_policy_envelope_base64.cpp -o flowpipe
//
// Usage:
//   echo "Hello Flowcore" | ./flowpipe --mode encode
//   echo "SGVsbG8gRmxvd2NvcmUK" | ./flowpipe --mode decode
//
// Policy examples:
//   --mode encode
//   --mode decode
//   --strict true
//   --ignore-whitespace true
//   --wrap 76

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

class Base64Stage {
public:
    [[nodiscard]] Envelope<ByteBuffer> run(Envelope<ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string encode(const std::string& input, int wrap);
    [[nodiscard]] static std::string decode(
        const std::string& input,
        bool strict,
        bool ignoreWhitespace,
        std::vector<Diagnostic>& diagnostics
    );

    [[nodiscard]] static int decodeChar(unsigned char c);
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
    static const char* severityName(Severity severity);

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

// ------------------------------------------------------------
// Sinks
// ------------------------------------------------------------

StdoutSink::StdoutSink(std::ostream& out)
    : out_(&out) {}

void StdoutSink::run(Envelope<ByteBuffer> env) const {
    if (out_ == nullptr) {
        throw DiagnosticError{"stdout-sink", "missing output stream"};
    }

    *out_ << env.payload.bytes;
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
    policies.set("base64.mode", std::string{"encode"});
    policies.set("base64.strict", true);
    policies.set("base64.ignore_whitespace", true);
    policies.set("base64.wrap", 0);

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
            policies.set("base64.mode", requireValue(arg));
        } else if (arg == "--strict") {
            policies.set("base64.strict", parseBool(requireValue(arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("base64.ignore_whitespace", parseBool(requireValue(arg)));
        } else if (arg == "--wrap") {
            policies.set("base64.wrap", parseInt(requireValue(arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  flowpipe [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode encode|decode\n"
                << "  --strict true|false\n"
                << "  --ignore-whitespace true|false\n"
                << "  --wrap N\n";
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
        Base64Stage base64;
        StdoutSink stdoutSink(std::cout);

        stdinProducer
            >> base64
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
