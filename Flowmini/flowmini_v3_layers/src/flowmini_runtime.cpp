#include "flowmini_runtime.h"

#include "flow_common.h"

#include <iostream>
#include <limits>
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

[[nodiscard]] std::string endpointText(const Endpoint& endpoint) {
    return endpoint.node + "." + endpoint.port;
}

[[nodiscard]] std::string getPolicyString(const NodeConfig& config, const std::string& key, std::string fallback) {
    const auto it = config.policies.find(key);
    if (it == config.policies.end()) {
        return fallback;
    }
    if (const auto* value = std::get_if<std::string>(&it->second)) {
        return *value;
    }
    throw flow::DiagnosticError{config.kind, "policy '" + config.id + "." + key + "' expected string"};
}

[[nodiscard]] int getPolicyInt(const NodeConfig& config, const std::string& key, int fallback) {
    const auto it = config.policies.find(key);
    if (it == config.policies.end()) {
        return fallback;
    }
    if (const auto* value = std::get_if<int>(&it->second)) {
        return *value;
    }
    throw flow::DiagnosticError{config.kind, "policy '" + config.id + "." + key + "' expected int"};
}

[[maybe_unused]] [[nodiscard]] bool getPolicyBool(const NodeConfig& config, const std::string& key, bool fallback) {
    const auto it = config.policies.find(key);
    if (it == config.policies.end()) {
        return fallback;
    }
    if (const auto* value = std::get_if<bool>(&it->second)) {
        return *value;
    }
    throw flow::DiagnosticError{config.kind, "policy '" + config.id + "." + key + "' expected bool"};
}

[[nodiscard]] std::int64_t rhsInt(const RecordPayload& record, const NodeConfig& config, const std::string& stage) {
    const auto constIt = config.policies.find("rhs_const");
    if (constIt != config.policies.end()) {
        if (const auto* value = std::get_if<int>(&constIt->second)) {
            return *value;
        }
        throw flow::DiagnosticError{stage, "policy 'rhs_const' expected int"};
    }

    const std::string rhs = getPolicyString(config, "rhs", "");
    if (rhs.empty()) {
        throw flow::DiagnosticError{stage, "missing policy rhs or rhs_const"};
    }
    return getPathInt(record, rhs, stage);
}

class ConfiguredNode : public INode {
public:
    explicit ConfiguredNode(NodeConfig config) : config_(std::move(config)) {}

protected:
    NodeConfig config_;
};

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

class ParseIntToRecordNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        const std::string text = requirePayloadConst<TextPayload>(env, "parse.int.to_record").value;
        RecordPayload record;
        setPathInt(record, getPolicyString(config_, "out", "n"), parseInt64Text(text), "parse.int.to_record");
        env.payload = std::move(record);
        return {Route{"out", std::move(env)}};
    }
};

class ConstIntNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, "const.int");
        const std::string out = getPolicyString(config_, "out", "value");
        const int value = getPolicyInt(config_, "value", 0);
        setPathInt(record, out, value, "const.int");
        return {Route{"out", std::move(env)}};
    }
};

class IntBinaryOpNode final : public ConfiguredNode {
public:
    enum class Op { Add, Sub, Mul, Div, Mod };

    IntBinaryOpNode(NodeConfig config, Op op, std::string stage)
        : ConfiguredNode(std::move(config)), op_(op), stage_(std::move(stage)) {}

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, stage_);
        const std::int64_t lhs = getPathInt(record, getPolicyString(config_, "lhs", "lhs"), stage_);
        const std::int64_t rhs = rhsInt(record, config_, stage_);
        const std::string out = getPolicyString(config_, "out", "out");

        std::int64_t result = 0;
        switch (op_) {
            case Op::Add:
                if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() - rhs) ||
                    (rhs < 0 && lhs < std::numeric_limits<std::int64_t>::min() - rhs)) {
                    throw flow::DiagnosticError{stage_, "integer addition overflow"};
                }
                result = lhs + rhs;
                break;
            case Op::Sub:
                if ((rhs < 0 && lhs > std::numeric_limits<std::int64_t>::max() + rhs) ||
                    (rhs > 0 && lhs < std::numeric_limits<std::int64_t>::min() + rhs)) {
                    throw flow::DiagnosticError{stage_, "integer subtraction overflow"};
                }
                result = lhs - rhs;
                break;
            case Op::Mul:
                if (lhs != 0 && rhs != 0) {
                    const auto max = std::numeric_limits<std::int64_t>::max();
                    const auto min = std::numeric_limits<std::int64_t>::min();
                    if ((lhs == -1 && rhs == min) || (rhs == -1 && lhs == min) ||
                        (lhs > 0 && rhs > 0 && lhs > max / rhs) ||
                        (lhs > 0 && rhs < 0 && rhs < min / lhs) ||
                        (lhs < 0 && rhs > 0 && lhs < min / rhs) ||
                        (lhs < 0 && rhs < 0 && lhs < max / rhs)) {
                        throw flow::DiagnosticError{stage_, "integer multiplication overflow"};
                    }
                }
                result = lhs * rhs;
                break;
            case Op::Div:
                if (rhs == 0) {
                    throw flow::DiagnosticError{stage_, "division by zero"};
                }
                if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) {
                    throw flow::DiagnosticError{stage_, "integer division overflow"};
                }
                result = lhs / rhs;
                break;
            case Op::Mod:
                if (rhs == 0) {
                    throw flow::DiagnosticError{stage_, "modulo by zero"};
                }
                result = lhs % rhs;
                break;
        }

        setPathInt(record, out, result, stage_);
        return {Route{"out", std::move(env)}};
    }

private:
    Op op_;
    std::string stage_;
};

class IntCompareNode final : public ConfiguredNode {
public:
    enum class Op { Eq, Lt, Gt };

    IntCompareNode(NodeConfig config, Op op, std::string stage)
        : ConfiguredNode(std::move(config)), op_(op), stage_(std::move(stage)) {}

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, stage_);
        const std::int64_t lhs = getPathInt(record, getPolicyString(config_, "lhs", "lhs"), stage_);
        const std::int64_t rhs = rhsInt(record, config_, stage_);
        const std::string out = getPolicyString(config_, "out", "cond");

        bool result = false;
        switch (op_) {
            case Op::Eq: result = (lhs == rhs); break;
            case Op::Lt: result = (lhs < rhs); break;
            case Op::Gt: result = (lhs > rhs); break;
        }

        setPathBool(record, out, result, stage_);
        return {Route{"out", std::move(env)}};
    }

private:
    Op op_;
    std::string stage_;
};

class RouteBoolNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        const auto* record = std::get_if<RecordPayload>(&env.payload);
        if (record == nullptr) {
            throw flow::DiagnosticError{"route.bool", "payload contract mismatch"};
        }
        const bool value = getPathBool(*record, getPolicyString(config_, "path", "cond"), "route.bool");
        return {Route{value ? "true" : "false", std::move(env)}};
    }
};

class StdoutIntLineRecordNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        const auto* record = std::get_if<RecordPayload>(&env.payload);
        if (record == nullptr) {
            throw flow::DiagnosticError{"stdout.int_line", "payload contract mismatch"};
        }
        const std::int64_t value = getPathInt(*record, getPolicyString(config_, "path", "n"), "stdout.int_line");
        std::cout << value << '\n';
        return {Route{"out", std::move(env)}};
    }
};


class RecordCopyNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, "record.copy");
        const std::string from = getPolicyString(config_, "from", "from");
        const std::string to = getPolicyString(config_, "to", "to");
        setPathValue(record, to, getPathValue(record, from, "record.copy"), "record.copy");
        return {Route{"out", std::move(env)}};
    }
};

class RecordSwapNode final : public ConfiguredNode {
public:
    using ConfiguredNode::ConfiguredNode;

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, "record.swap");
        const std::string a = getPolicyString(config_, "a", "a");
        const std::string b = getPolicyString(config_, "b", "b");

        Value av = getPathValue(record, a, "record.swap");
        Value bv = getPathValue(record, b, "record.swap");
        setPathValue(record, a, std::move(bv), "record.swap");
        setPathValue(record, b, std::move(av), "record.swap");

        return {Route{"out", std::move(env)}};
    }
};

class IntCompareSwapNode final : public ConfiguredNode {
public:
    enum class Order { Asc, Desc };

    IntCompareSwapNode(NodeConfig config, Order order, std::string stage)
        : ConfiguredNode(std::move(config)), order_(order), stage_(std::move(stage)) {}

    std::vector<Route> run(MiniEnvelope env) override {
        auto& record = requirePayload<RecordPayload>(env, stage_);
        const std::string a = getPolicyString(config_, "a", "a");
        const std::string b = getPolicyString(config_, "b", "b");
        const std::int64_t av = getPathInt(record, a, stage_);
        const std::int64_t bv = getPathInt(record, b, stage_);

        const bool shouldSwap = (order_ == Order::Asc) ? (av > bv) : (av < bv);
        if (shouldSwap) {
            setPathInt(record, a, bv, stage_);
            setPathInt(record, b, av, stage_);
        }

        return {Route{"out", std::move(env)}};
    }

private:
    Order order_;
    std::string stage_;
};

class IntGtZeroNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "int.gt_zero").value;
        return {Route{value > 0 ? "true" : "false", std::move(env)}};
    }
};

class IntRequirePositiveNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "int.require_positive").value;
        if (value <= 0) {
            throw flow::DiagnosticError{"int.require_positive", "expected positive integer"};
        }
        return {Route{"out", std::move(env)}};
    }
};

class IntEqOneNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "int.eq_one").value;
        return {Route{value == 1 ? "true" : "false", std::move(env)}};
    }
};

class IntIsEvenNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "int.is_even").value;
        return {Route{(value % 2) == 0 ? "true" : "false", std::move(env)}};
    }
};

class StdoutIntLineNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        const std::int64_t value = requirePayloadConst<IntPayload>(env, "stdout.int_line.legacy").value;
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

class IntHalfNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        auto& payload = requirePayload<IntPayload>(env, "int.half");
        payload.value /= 2;
        return {Route{"out", std::move(env)}};
    }
};

class IntThreeNPlusOneNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope env) override {
        auto& payload = requirePayload<IntPayload>(env, "int.three_n_plus_one");
        const auto max = std::numeric_limits<std::int64_t>::max();
        if (payload.value > (max - 1) / 3) {
            throw flow::DiagnosticError{"int.three_n_plus_one", "3n + 1 would overflow Int"};
        }
        payload.value = (payload.value * 3) + 1;
        return {Route{"out", std::move(env)}};
    }
};

class HaltNode final : public INode {
public:
    std::vector<Route> run(MiniEnvelope) override {
        return {};
    }
};

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

std::unique_ptr<INode> AtomRegistry::create(NodeConfig config) const {
    const auto it = factories_.find(config.kind);
    if (it == factories_.end()) {
        throw flow::DiagnosticError{"runtime", "no factory for atom kind: " + config.kind};
    }
    return it->second(std::move(config));
}

bool AtomRegistry::contains(const std::string& kind) const {
    return contracts_.find(kind) != contracts_.end();
}

AtomRegistry makeCoreAtomRegistry() {
    AtomRegistry registry;

    registry.registerAtom(AtomContract{"stdin.text", {{"in", "Unit"}}, {{"out", "Text"}}, {"stdin.read"}},
        [](NodeConfig) { return std::make_unique<StdinTextNode>(); });

    registry.registerAtom(AtomContract{"parse.int", {{"in", "Text"}}, {{"out", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<ParseIntNode>(); });

    registry.registerAtom(AtomContract{"parse.int.to_record", {{"in", "Text"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<ParseIntToRecordNode>(std::move(config)); });

    registry.registerAtom(AtomContract{"const.int", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<ConstIntNode>(std::move(config)); });

    registry.registerAtom(AtomContract{"int.add", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntBinaryOpNode>(std::move(config), IntBinaryOpNode::Op::Add, "int.add"); });
    registry.registerAtom(AtomContract{"int.sub", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntBinaryOpNode>(std::move(config), IntBinaryOpNode::Op::Sub, "int.sub"); });
    registry.registerAtom(AtomContract{"int.mul", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntBinaryOpNode>(std::move(config), IntBinaryOpNode::Op::Mul, "int.mul"); });
    registry.registerAtom(AtomContract{"int.div", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntBinaryOpNode>(std::move(config), IntBinaryOpNode::Op::Div, "int.div"); });
    registry.registerAtom(AtomContract{"int.mod", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntBinaryOpNode>(std::move(config), IntBinaryOpNode::Op::Mod, "int.mod"); });

    registry.registerAtom(AtomContract{"int.eq", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntCompareNode>(std::move(config), IntCompareNode::Op::Eq, "int.eq"); });
    registry.registerAtom(AtomContract{"int.lt", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntCompareNode>(std::move(config), IntCompareNode::Op::Lt, "int.lt"); });
    registry.registerAtom(AtomContract{"int.gt", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntCompareNode>(std::move(config), IntCompareNode::Op::Gt, "int.gt"); });

    registry.registerAtom(AtomContract{"route.bool", {{"in", "Record"}}, {{"true", "Record"}, {"false", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<RouteBoolNode>(std::move(config)); });

    registry.registerAtom(AtomContract{"stdout.int_line", {{"in", "Record"}}, {{"out", "Record"}}, {"stdout.write"}},
        [](NodeConfig config) { return std::make_unique<StdoutIntLineRecordNode>(std::move(config)); });

    registry.registerAtom(AtomContract{"record.copy", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<RecordCopyNode>(std::move(config)); });
    registry.registerAtom(AtomContract{"record.swap", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<RecordSwapNode>(std::move(config)); });

    registry.registerAtom(AtomContract{"int.compare_swap_asc", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntCompareSwapNode>(std::move(config), IntCompareSwapNode::Order::Asc, "int.compare_swap_asc"); });
    registry.registerAtom(AtomContract{"int.compare_swap_desc", {{"in", "Record"}}, {{"out", "Record"}}, {}},
        [](NodeConfig config) { return std::make_unique<IntCompareSwapNode>(std::move(config), IntCompareSwapNode::Order::Desc, "int.compare_swap_desc"); });

    registry.registerAtom(AtomContract{"halt.record", {{"in", "Record"}}, {}, {}},
        [](NodeConfig) { return std::make_unique<HaltNode>(); });

    // Legacy v1 atoms retained so older examples still demonstrate the previous task-shaped atom layer.
    registry.registerAtom(AtomContract{"int.gt_zero", {{"in", "Int"}}, {{"true", "Int"}, {"false", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntGtZeroNode>(); });
    registry.registerAtom(AtomContract{"int.require_positive", {{"in", "Int"}}, {{"out", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntRequirePositiveNode>(); });
    registry.registerAtom(AtomContract{"int.eq_one", {{"in", "Int"}}, {{"true", "Int"}, {"false", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntEqOneNode>(); });
    registry.registerAtom(AtomContract{"int.is_even", {{"in", "Int"}}, {{"true", "Int"}, {"false", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntIsEvenNode>(); });
    registry.registerAtom(AtomContract{"stdout.int_line.legacy", {{"in", "Int"}}, {{"out", "Int"}}, {"stdout.write"}},
        [](NodeConfig) { return std::make_unique<StdoutIntLineNode>(); });
    registry.registerAtom(AtomContract{"int.dec", {{"in", "Int"}}, {{"out", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntDecNode>(); });
    registry.registerAtom(AtomContract{"int.half", {{"in", "Int"}}, {{"out", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntHalfNode>(); });
    registry.registerAtom(AtomContract{"int.three_n_plus_one", {{"in", "Int"}}, {{"out", "Int"}}, {}},
        [](NodeConfig) { return std::make_unique<IntThreeNPlusOneNode>(); });
    registry.registerAtom(AtomContract{"halt", {{"in", "Int"}}, {}, {}},
        [](NodeConfig) { return std::make_unique<HaltNode>(); });

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
    std::map<std::string, NodePolicyMap> policiesByNode;
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

    for (const auto& policy : module.policies) {
        if (nodesById.find(policy.node) == nodesById.end()) {
            throw flow::DiagnosticError{"validator", "policy references unknown node: " + policy.node};
        }
        policiesByNode[policy.node][policy.key] = policy.value;
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
            throw flow::DiagnosticError{"validator", "source endpoint " + endpointText(wire.from) + " is not an output port"};
        }

        const auto toPortIt = toContract.inputs.find(wire.to.port);
        if (toPortIt == toContract.inputs.end()) {
            throw flow::DiagnosticError{"validator", "target endpoint " + endpointText(wire.to) + " is not an input port"};
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
        NodeConfig config;
        config.id = item.second.id;
        config.kind = item.second.kind;
        config.policies = policiesByNode[item.second.id];
        result.graph.addNode(item.first, registry.create(std::move(config)));
    }

    for (const auto& wire : module.wires) {
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
