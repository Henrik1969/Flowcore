#ifndef FLOWMINI_RUNTIME_H
#define FLOWMINI_RUNTIME_H

#include "flowmini_payload.h"
#include "flowmini_schema.h"

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace flowmini {

struct Route {
    std::string port;
    MiniEnvelope envelope;
};

class INode {
public:
    virtual ~INode() = default;
    [[nodiscard]] virtual std::vector<Route> run(MiniEnvelope env) = 0;
};

class AtomRegistry {
public:
    using Factory = std::function<std::unique_ptr<INode>()>;

    void registerAtom(AtomContract contract, Factory factory);

    [[nodiscard]] const AtomContract& contractFor(const std::string& kind) const;
    [[nodiscard]] std::unique_ptr<INode> create(const std::string& kind) const;
    [[nodiscard]] bool contains(const std::string& kind) const;

private:
    std::map<std::string, AtomContract> contracts_;
    std::map<std::string, Factory> factories_;
};

[[nodiscard]] AtomRegistry makeCoreAtomRegistry();

class RuntimeGraph {
public:
    void addNode(std::string id, std::unique_ptr<INode> node);
    void connect(std::string fromNode, std::string fromPort, std::string toNode);

    void startAt(const std::string& nodeId, MiniEnvelope env);

private:
    struct Pending {
        std::string nodeId;
        MiniEnvelope envelope;
    };

    [[nodiscard]] static std::string wireKey(const std::string& node, const std::string& port);

    void deliver(const std::string& fromNode, const std::string& fromPort, MiniEnvelope env);
    void trace(const std::string& message, MiniEnvelope& env) const;

    std::map<std::string, std::unique_ptr<INode>> nodes_;
    std::map<std::string, std::vector<std::string>> wires_;
    std::queue<Pending> queue_;
};

struct BuildResult {
    RuntimeGraph graph;
    std::vector<std::string> producerIds;
};

[[nodiscard]] BuildResult buildCheckedGraph(const ModuleSpec& module, const AtomRegistry& registry);

void runModule(const ModuleSpec& module, flow::PipelineContext& ctx, const AtomRegistry& registry);

} // namespace flowmini

#endif // FLOWMINI_RUNTIME_H
