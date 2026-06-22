#include "flowmini_runtime.h"

#include "flow_common.h"

#include <iostream>
#include <sstream>
#include <utility>

namespace flowmini {

namespace {

[[nodiscard]] std::int64_t parseInt64Text(const std::string& text) {
    try {
        std::size_t pos = 0;
        const long long result = std::stoll(text, &pos);

        while (pos < text.size()) {
            const unsigned char c = static_cast<unsigned char>(text[pos]);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                throw flow::DiagnosticError{"parse.int", "input contains trailing non-integer data"};
            }
            ++pos;
        }

        return static_cast<std::int64_t>(result);
    } catch (const std::invalid_argument&) {
        throw flow::DiagnosticError{"parse.int", "expected integer input"};
    } catch (const std::out_of_range&) {
        throw flow::DiagnosticError{"parse.int", "integer out of range"};
    }
}

class StdinTextNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        if (env.ctx == nullptr) {
            throw flow::DiagnosticError{"stdin.text", "missing pipeline context"};
        }

        std::ostringstream buffer;
        buffer << std::cin.rdbuf();
        env.payload = TextPayload{buffer.str()};
        return {Route{"out", std::move(env)}};
    }
};

class ParseIntNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::string text = requirePayloadConst<TextPayload>(env, "parse.int").value;
        env.payload = IntPayload{parseInt64Text(text)};
        return {Route{"out", std::move(env)}};
    }
};

class IntGtZeroNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "int.gt_zero").value;
        return {Route{value > 0 ? "true" : "false", std::move(env)}};
    }
};

class StdoutIntLineNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "stdout.int_line").value;
        std::cout << value << '\n';
        return {Route{"out", std::move(env)}};
    }
};

class IntDecNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        auto& payload = requirePayload<IntPayload>(env, "int.dec");
        --payload.value;
        return {Route{"out", std::move(env)}};
    }
};

class HaltNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope) override {
        return {};
    }
};

[[nodiscard]] std::string endpointText(const Endpoint& endpoint) {
    return endpoint.node + "." + endpoint.port;
}

} // namespace

void AtomRegistry::registerAtom(AtomContract contract, Factory factory) {
    const std::string kind = contract.kind;
    contracts_[kind] = std::move(contract);
    factories_[kind] = std::move(factory);
}

const AtomContract& AtomRegistry::contractFor(const std::string& kind) const {
    const auto it = contracts_.find(kind);
    if (it == contracts_.end()) {
        throw flow::DiagnosticError{"validator", "unknown atom kind: " + kind};
    }
    return it->second;
}

std::unique_ptr<INode> AtomRegistry::create(const std::string& kind) const {
    const auto it = factories_.find(kind);
    if (it == factories_.end()) {
        throw flow::DiagnosticError{"runtime", "no factory for atom kind: " + kind};
    }
    return it->second();
}

bool AtomRegistry::contains(const std::string& kind) const {
    return contracts_.find(kind) != contracts_.end();
}

AtomRegistry makeCoreAtomRegistry() {
    AtomRegistry registry;

    registry.registerAtom(
        AtomContract{
            "stdin.text",
            {{"in", "Unit"}},
            {{"out", "Text"}},
            {"stdin.read"}
        },
        [] { return std::make_unique<StdinTextNode>(); }
    );

    registry.registerAtom(
        AtomContract{
            "parse.int",
            {{"in", "Text"}},
            {{"out", "Int"}},
            {}
        },
        [] { return std::make_unique<ParseIntNode>(); }
    );

    registry.registerAtom(
        AtomContract{
            "int.gt_zero",
            {{"in", "Int"}},
            {{"true", "Int"}, {"false", "Int"}},
            {}
        },
        [] { return std::make_unique<IntGtZeroNode>(); }
    );

    registry.registerAtom(
        AtomContract{
            "stdout.int_line",
            {{"in", "Int"}},
            {{"out", "Int"}},
            {"stdout.write"}
        },
        [] { return std::make_unique<StdoutIntLineNode>(); }
    );

    registry.registerAtom(
        AtomContract{
            "int.dec",
            {{"in", "Int"}},
            {{"out", "Int"}},
            {}
        },
        [] { return std::make_unique<IntDecNode>(); }
    );

    registry.registerAtom(
        AtomContract{
            "halt",
            {{"in", "Int"}},
            {},
            {}
        },
        [] { return std::make_unique<HaltNode>(); }
    );

    return registry;
}

void RuntimeGraph::addNode(std::string id, std::unique_ptr<INode> node) {
    nodes_[std::move(id)] = std::move(node);
}

void RuntimeGraph::connect(std::string fromNode, std::string fromPort, std::string toNode) {
    wires_[wireKey(fromNode, fromPort)].push_back(std::move(toNode));
}

void RuntimeGraph::startAt(const std::string& nodeId, MiniEnvelope env) {
    queue_.push(Pending{nodeId, std::move(env)});

    while (!queue_.empty()) {
        Pending pending = std::move(queue_.front());
        queue_.pop();

        const auto it = nodes_.find(pending.nodeId);
        if (it == nodes_.end()) {
            throw flow::DiagnosticError{"runtime", "unknown node: " + pending.nodeId};
        }

        trace("enter " + pending.nodeId + " with " + payloadTypeName(pending.envelope.payload), pending.envelope);

        std::vector<Route> routes = it->second->run(std::move(pending.envelope));
        for (auto& route : routes) {
            deliver(pending.nodeId, route.port, std::move(route.envelope));
        }
    }
}

std::string RuntimeGraph::wireKey(const std::string& node, const std::string& port) {
    return node + "." + port;
}

void RuntimeGraph::deliver(const std::string& fromNode, const std::string& fromPort, MiniEnvelope env) {
    if (env.ctx == nullptr) {
        throw flow::DiagnosticError{"runtime", "missing pipeline context"};
    }

    const std::string key = wireKey(fromNode, fromPort);
    const auto it = wires_.find(key);

    if (it == wires_.end()) {
        env.ctx->diagnostics.push_back({
            flow::Severity::Warning,
            "runtime",
            "no wire connected from " + key + "; dropping envelope"
        });
        return;
    }

    for (const auto& target : it->second) {
        trace("route " + key + " => " + target + ".in", env);
        queue_.push(Pending{target, env});
    }
}

void RuntimeGraph::trace(const std::string& message, MiniEnvelope& env) const {
    if (env.ctx == nullptr) {
        return;
    }

    const bool enabled = env.ctx->policies.getBoolOrDefault(
        "runtime.trace",
        false,
        env.ctx->diagnostics,
        "runtime"
    );

    if (enabled) {
        env.ctx->diagnostics.push_back({flow::Severity::Info, "runtime", message});
    }
}

BuildResult buildCheckedGraph(const ModuleSpec& module, const AtomRegistry& registry) {
    std::map<std::string, NodeDecl> nodesById;
    std::vector<std::string> producerIds;

    for (const auto& node : module.nodes) {
        if (nodesById.find(node.id) != nodesById.end()) {
            throw flow::DiagnosticError{"validator", "duplicate node id: " + node.id};
        }

        if (!registry.contains(node.kind)) {
            throw flow::DiagnosticError{"validator", "unknown atom kind: " + node.kind};
        }

        if (node.role == "producer") {
            producerIds.push_back(node.id);
        }

        nodesById[node.id] = node;
    }

    if (producerIds.empty()) {
        throw flow::DiagnosticError{"validator", "module has no producer"};
    }

    for (const auto& wire : module.wires) {
        const auto fromNodeIt = nodesById.find(wire.from.node);
        if (fromNodeIt == nodesById.end()) {
            throw flow::DiagnosticError{"validator", "wire references unknown source node: " + wire.from.node};
        }

        const auto toNodeIt = nodesById.find(wire.to.node);
        if (toNodeIt == nodesById.end()) {
            throw flow::DiagnosticError{"validator", "wire references unknown target node: " + wire.to.node};
        }

        const auto& fromContract = registry.contractFor(fromNodeIt->second.kind);
        const auto& toContract = registry.contractFor(toNodeIt->second.kind);

        const auto fromPortIt = fromContract.outputs.find(wire.from.port);
        if (fromPortIt == fromContract.outputs.end()) {
            throw flow::DiagnosticError{
                "validator",
                "source endpoint " + endpointText(wire.from) + " is not an output port"
            };
        }

        const auto toPortIt = toContract.inputs.find(wire.to.port);
        if (toPortIt == toContract.inputs.end()) {
            throw flow::DiagnosticError{
                "validator",
                "target endpoint " + endpointText(wire.to) + " is not an input port"
            };
        }

        if (fromPortIt->second != toPortIt->second) {
            throw flow::DiagnosticError{
                "validator",
                "type mismatch on wire " + endpointText(wire.from) + " => " + endpointText(wire.to) +
                ": " + fromPortIt->second + " cannot connect to " + toPortIt->second
            };
        }
    }

    BuildResult result;
    result.producerIds = producerIds;

    for (const auto& item : nodesById) {
        result.graph.addNode(item.first, registry.create(item.second.kind));
    }

    for (const auto& wire : module.wires) {
        // v0 assumes all target ports are named "in" at runtime. The validator still checks the declared port.
        result.graph.connect(wire.from.node, wire.from.port, wire.to.node);
    }

    return result;
}

void runModule(const ModuleSpec& module, flow::PipelineContext& ctx, const AtomRegistry& registry) {
    BuildResult build = buildCheckedGraph(module, registry);

    for (const auto& producerId : build.producerIds) {
        build.graph.startAt(producerId, MiniEnvelope{UnitPayload{}, &ctx});
    }
}

} // namespace flowmini
