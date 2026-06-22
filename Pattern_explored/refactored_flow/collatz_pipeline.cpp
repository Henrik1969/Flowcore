#include "collatz_pipeline.h"

#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

namespace collatz {

StdinTextProducer::StdinTextProducer(flow::PipelineContext& ctx)
    : ctx_(&ctx) {}

Envelope StdinTextProducer::produce() const {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();

    return Envelope{
        TextPayload{buffer.str()},
        ctx_
    };
}

std::vector<Route> ParseUIntNode::run(Envelope env) {
    auto& text = requirePayload<TextPayload>(env, "parse");

    const std::string trimmed = trim(text.text);

    if (trimmed.empty()) {
        throw flow::DiagnosticError{"parse", "empty input"};
    }

    std::size_t pos = 0;
    unsigned long long value = 0;

    try {
        value = std::stoull(trimmed, &pos);
    } catch (const std::exception&) {
        throw flow::DiagnosticError{"parse", "expected unsigned integer input"};
    }

    if (pos != trimmed.size()) {
        throw flow::DiagnosticError{"parse", "input contains non-integer trailing data"};
    }

    if (value == 0) {
        throw flow::DiagnosticError{"parse", "Collatz input must be greater than zero"};
    }

    CollatzPayload payload;
    payload.n = static_cast<std::uint64_t>(value);
    payload.steps = 0;
    payload.sequence.push_back(payload.n);

    return {
        Route{
            "ok",
            Envelope{std::move(payload), env.ctx}
        }
    };
}

std::string ParseUIntNode::trim(const std::string& input) {
    std::size_t first = 0;

    while (first < input.size() &&
           std::isspace(static_cast<unsigned char>(input[first]))) {
        ++first;
    }

    std::size_t last = input.size();

    while (last > first &&
           std::isspace(static_cast<unsigned char>(input[last - 1]))) {
        --last;
    }

    return input.substr(first, last - first);
}

std::vector<Route> CheckDoneNode::run(Envelope env) {
    if (env.ctx == nullptr) {
        throw flow::DiagnosticError{"check", "missing pipeline context"};
    }

    auto& payload = requirePayload<CollatzPayload>(env, "check");

    const int maxSteps =
        env.ctx->policies.getIntOrDefault(
            "collatz.max_steps",
            10000,
            env.ctx->diagnostics,
            "check"
        );

    if (payload.n == 1) {
        return {Route{"done", std::move(env)}};
    }

    if (maxSteps >= 0 &&
        payload.steps >= static_cast<std::uint64_t>(maxSteps)) {
        throw flow::DiagnosticError{"check", "maximum Collatz steps reached"};
    }

    return {Route{"continue", std::move(env)}};
}

std::vector<Route> ParityBranchNode::run(Envelope env) {
    const auto& payload = requirePayloadConst<CollatzPayload>(env, "parity");

    if ((payload.n % 2U) == 0U) {
        return {Route{"even", std::move(env)}};
    }

    return {Route{"odd", std::move(env)}};
}

std::vector<Route> EvenStepNode::run(Envelope env) {
    auto& payload = requirePayload<CollatzPayload>(env, "even");

    payload.n /= 2U;
    ++payload.steps;
    payload.sequence.push_back(payload.n);

    return {Route{"out", std::move(env)}};
}

std::vector<Route> OddStepNode::run(Envelope env) {
    if (env.ctx == nullptr) {
        throw flow::DiagnosticError{"odd", "missing pipeline context"};
    }

    auto& payload = requirePayload<CollatzPayload>(env, "odd");

    const bool stopOnOverflow =
        env.ctx->policies.getBoolOrDefault(
            "collatz.stop_on_overflow",
            true,
            env.ctx->diagnostics,
            "odd"
        );

    const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();

    if (payload.n > (max - 1U) / 3U) {
        if (stopOnOverflow) {
            throw flow::DiagnosticError{"odd", "3n + 1 would overflow uint64_t"};
        }

        env.ctx->diagnostics.push_back({
            flow::Severity::Warning,
            "odd",
            "overflow would occur; stopping sequence at current value"
        });

        payload.n = 1;
        payload.sequence.push_back(payload.n);

        return {Route{"out", std::move(env)}};
    }

    payload.n = (payload.n * 3U) + 1U;
    ++payload.steps;
    payload.sequence.push_back(payload.n);

    return {Route{"out", std::move(env)}};
}

std::vector<Route> FormatOutputNode::run(Envelope env) {
    if (env.ctx == nullptr) {
        throw flow::DiagnosticError{"format", "missing pipeline context"};
    }

    const auto& payload = requirePayloadConst<CollatzPayload>(env, "format");

    const bool includeSteps =
        env.ctx->policies.getBoolOrDefault(
            "format.include_steps",
            true,
            env.ctx->diagnostics,
            "format"
        );

    std::ostringstream out;

    if (includeSteps) {
        out << "steps: " << payload.steps << '\n';
    }

    out << "sequence:\n";

    for (std::size_t i = 0; i < payload.sequence.size(); ++i) {
        if (i > 0) {
            out << " -> ";
        }

        out << payload.sequence[i];
    }

    out << '\n';

    return {
        Route{
            "out",
            Envelope{OutputPayload{out.str()}, env.ctx}
        }
    };
}

std::vector<Route> StdoutNode::run(Envelope env) {
    const auto& payload = requirePayloadConst<OutputPayload>(env, "stdout");

    std::cout << payload.text;

    return {};
}

void RuntimeGraph::addNode(std::string id, std::unique_ptr<Node> node) {
    nodes_[std::move(id)] = std::move(node);
}

void RuntimeGraph::connect(std::string fromNode, std::string fromPort, std::string toNode) {
    wires_[wireKey(fromNode, fromPort)] = std::move(toNode);
}

void RuntimeGraph::startFromProducer(
    const std::string& producerId,
    const std::string& producerPort,
    Envelope env
) {
    deliver(producerId, producerPort, std::move(env));

    while (!queue_.empty()) {
        Pending pending = std::move(queue_.front());
        queue_.pop();

        auto it = nodes_.find(pending.nodeId);

        if (it == nodes_.end()) {
            throw flow::DiagnosticError{"runtime", "unknown node: " + pending.nodeId};
        }

        trace("runtime", "enter node '" + pending.nodeId + "'", pending.env);

        std::vector<Route> routes = it->second->run(std::move(pending.env));

        for (auto& route : routes) {
            deliver(pending.nodeId, route.port, std::move(route.envelope));
        }
    }
}

std::string RuntimeGraph::wireKey(const std::string& node, const std::string& port) {
    return node + "." + port;
}

void RuntimeGraph::deliver(
    const std::string& fromNode,
    const std::string& fromPort,
    Envelope env
) {
    if (env.ctx == nullptr) {
        throw flow::DiagnosticError{"runtime", "missing pipeline context"};
    }

    const auto key = wireKey(fromNode, fromPort);
    const auto it = wires_.find(key);

    if (it == wires_.end()) {
        env.ctx->diagnostics.push_back({
            flow::Severity::Warning,
            "runtime",
            "no wire connected from " + key + "; dropping envelope"
        });

        return;
    }

    trace("runtime", "route " + key + " -> " + it->second + ".in", env);

    queue_.push(Pending{it->second, std::move(env)});
}

void RuntimeGraph::trace(
    const std::string& stage,
    const std::string& message,
    Envelope& env
) {
    if (env.ctx == nullptr) {
        return;
    }

    const bool enabled =
        env.ctx->policies.getBoolOrDefault(
            "runtime.trace",
            false,
            env.ctx->diagnostics,
            stage
        );

    if (!enabled) {
        return;
    }

    env.ctx->diagnostics.push_back({
        flow::Severity::Info,
        stage,
        message
    });
}

} // namespace collatz
