#ifndef COLLATZ_PIPELINE_H
#define COLLATZ_PIPELINE_H

#include "flow_common.h"

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>

namespace collatz {

struct TextPayload {
    std::string text;
};

struct CollatzPayload {
    std::uint64_t n = 0;
    std::uint64_t steps = 0;
    std::vector<std::uint64_t> sequence;
};

struct OutputPayload {
    std::string text;
};

using Payload = std::variant<TextPayload, CollatzPayload, OutputPayload>;

struct Envelope {
    Payload payload;
    flow::PipelineContext* ctx = nullptr;
};

template <typename T>
T& requirePayload(Envelope& env, const char* stage) {
    if (auto* value = std::get_if<T>(&env.payload)) {
        return *value;
    }

    throw flow::DiagnosticError{stage, "payload contract mismatch"};
}

template <typename T>
const T& requirePayloadConst(const Envelope& env, const char* stage) {
    if (const auto* value = std::get_if<T>(&env.payload)) {
        return *value;
    }

    throw flow::DiagnosticError{stage, "payload contract mismatch"};
}

struct Route {
    std::string port;
    Envelope envelope;
};

class Node {
public:
    virtual ~Node() = default;

    [[nodiscard]] virtual std::vector<Route> run(Envelope env) = 0;
};

class StdinTextProducer {
public:
    explicit StdinTextProducer(flow::PipelineContext& ctx);

    [[nodiscard]] Envelope produce() const;

private:
    flow::PipelineContext* ctx_;
};

class ParseUIntNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;

private:
    [[nodiscard]] static std::string trim(const std::string& input);
};

class CheckDoneNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class ParityBranchNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class EvenStepNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class OddStepNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class FormatOutputNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class StdoutNode final : public Node {
public:
    [[nodiscard]] std::vector<Route> run(Envelope env) override;
};

class RuntimeGraph {
public:
    void addNode(std::string id, std::unique_ptr<Node> node);
    void connect(std::string fromNode, std::string fromPort, std::string toNode);

    void startFromProducer(
        const std::string& producerId,
        const std::string& producerPort,
        Envelope env
    );

private:
    struct Pending {
        std::string nodeId;
        Envelope env;
    };

    [[nodiscard]] static std::string wireKey(
        const std::string& node,
        const std::string& port
    );

    void deliver(
        const std::string& fromNode,
        const std::string& fromPort,
        Envelope env
    );

    static void trace(
        const std::string& stage,
        const std::string& message,
        Envelope& env
    );

    std::map<std::string, std::unique_ptr<Node>> nodes_;
    std::map<std::string, std::string> wires_;
    std::queue<Pending> queue_;
};

} // namespace collatz

#endif // COLLATZ_PIPELINE_H
