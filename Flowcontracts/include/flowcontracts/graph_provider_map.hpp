#pragma once

#include <flowcontracts/json.hpp>
#include <set>

namespace flowcontracts {

struct GraphProviderSelection {
    std::string implementation, source_callable;
};

// Explicit provider selection, never capability authorization. The bounded
// adapter invokes a zero-argument provider once at startup and emits one value.
inline std::vector<GraphProviderSelection> graph_provider_map(const json::Value& value) {
    using namespace json;
    const auto& root = object(value);
    if (string(required(root, "format"), "$.format") != "flowcore.graph_provider_map" ||
        integer(required(root, "version"), "$.version") != 1)
        throw Error("$", "unsupported graph provider map contract");
    std::vector<GraphProviderSelection> result;
    std::set<std::string> names;
    for (const auto& value : array(required(root, "providers"), "$.providers")) {
        const auto path = "$.providers[" + std::to_string(result.size()) + "]";
        const auto& item = object(value, path);
        GraphProviderSelection selection{
            string(required(item, "implementation", path), path + ".implementation"),
            string(required(item, "source_callable", path), path + ".source_callable")};
        if (selection.implementation.empty() || selection.source_callable.empty() ||
            !names.insert(selection.implementation).second)
            throw Error(path, "empty or duplicate graph provider selection");
        if (string(required(item, "activation", path), path + ".activation") != "startup_once" ||
            string(required(item, "output_port", path), path + ".output_port") != "out")
            throw Error(path, "unsupported graph provider activation or output port");
        result.push_back(std::move(selection));
    }
    return result;
}

} // namespace flowcontracts
