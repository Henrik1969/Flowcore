#pragma once

#include <flowcontracts/json.hpp>

#include <set>
#include <string>
#include <string_view>

namespace flowcontracts {

struct Header { std::string format; json::Integer version = 0; std::string status; };

inline Header header(const json::Value& value) {
    const auto& root = json::object(value);
    return {json::string(json::required(root, "format"), "$.format"),
            json::integer(json::required(root, "version"), "$.version"),
            json::string(json::required(root, "status"), "$.status")};
}
inline Header require_header(const json::Value& value, std::string_view format, json::Integer version) {
    auto result = header(value);
    if (result.format != format) throw json::Error("$.format", "unsupported artifact format '" + result.format + "'");
    if (result.version != version) throw json::Error("$.version", "unsupported " + result.format + " version " + std::to_string(result.version));
    return result;
}

struct MatrixEntry { json::Integer row = 0; json::Integer column = 0; bool value = true; };
struct MatrixView { std::string name; json::Integer rows = 0; json::Integer columns = 0; std::string semiring; std::string storage; std::vector<MatrixEntry> entries; };
struct SemanticReport {
    Header artifact; std::string source_path; json::Array targets; json::Array external_operations;
    json::Array abi_type_contracts; json::Value lowering_plan; std::size_t proven_pure_count = 0;
    std::size_t independent_candidate_count = 0; MatrixView dependency_matrix;
};

inline const json::Object& required_object(const json::Object& parent, std::string_view key, std::string_view path = "$") {
    return json::object(json::required(parent, key, path), std::string(path) + "." + std::string(key));
}
inline const json::Array& required_array(const json::Object& parent, std::string_view key, std::string_view path = "$") {
    return json::array(json::required(parent, key, path), std::string(path) + "." + std::string(key));
}

inline MatrixView parse_matrix(const json::Object& root) {
    const auto& graph = required_object(root, "analysis_graph");
    if (json::string(json::required(graph, "format", "$.analysis_graph"), "$.analysis_graph.format") != "flowanalyst.analysis_graph") throw json::Error("$.analysis_graph.format", "unsupported analysis graph format");
    if (json::integer(json::required(graph, "version", "$.analysis_graph"), "$.analysis_graph.version") != 1) throw json::Error("$.analysis_graph.version", "unsupported analysis graph version");
    const auto& views = required_array(graph, "matrix_views", "$.analysis_graph");
    const json::Object* selected = nullptr; std::size_t selected_index = 0;
    for (std::size_t index = 0; index < views.size(); ++index) {
        const auto path = "$.analysis_graph.matrix_views[" + std::to_string(index) + "]";
        const auto& view = json::object(views[index], path);
        if (json::string(json::required(view, "name", path), path + ".name") == "region_dependency") {
            if (selected) throw json::Error("$.analysis_graph.matrix_views", "duplicate region_dependency matrix view");
            selected = &view; selected_index = index;
        }
    }
    if (!selected) throw json::Error("$.analysis_graph.matrix_views", "required region_dependency matrix view is missing");
    const auto path = "$.analysis_graph.matrix_views[" + std::to_string(selected_index) + "]";
    MatrixView result;
    result.name = "region_dependency";
    result.rows = json::integer(json::required(*selected, "rows", path), path + ".rows");
    result.columns = json::integer(json::required(*selected, "columns", path), path + ".columns");
    if (result.rows < 0 || result.columns < 0) throw json::Error(path, "matrix dimensions must be non-negative");
    result.semiring = json::string(json::required(*selected, "semiring", path), path + ".semiring");
    result.storage = json::string(json::required(*selected, "storage", path), path + ".storage");
    const auto& entries = required_array(*selected, "entries", path);
    std::set<std::pair<json::Integer, json::Integer>> coordinates;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto entry_path = path + ".entries[" + std::to_string(index) + "]";
        const auto& entry = json::object(entries[index], entry_path);
        MatrixEntry parsed;
        parsed.row = json::integer(json::required(entry, "row", entry_path), entry_path + ".row");
        parsed.column = json::integer(json::required(entry, "column", entry_path), entry_path + ".column");
        if (const auto* item = json::optional(entry, "value")) parsed.value = json::boolean(*item, entry_path + ".value");
        if (parsed.row < 0 || parsed.row >= result.rows) throw json::Error(entry_path + ".row", "matrix row is outside declared dimensions");
        if (parsed.column < 0 || parsed.column >= result.columns) throw json::Error(entry_path + ".column", "matrix column is outside declared dimensions");
        if (!coordinates.emplace(parsed.row, parsed.column).second) throw json::Error(entry_path, "duplicate matrix coordinate");
        result.entries.push_back(parsed);
    }
    return result;
}

inline SemanticReport semantic_report(const json::Value& value) {
    SemanticReport result; result.artifact = require_header(value, "flowanalyst.semantic_report", 1);
    if (result.artifact.status != "ok") return result;
    const auto& root = json::object(value);
    const auto& source = required_object(root, "source");
    result.source_path = json::string(json::required(source, "path", "$.source"), "$.source.path");
    result.targets = required_array(root, "targets");
    result.external_operations = required_array(root, "external_operations");
    result.abi_type_contracts = required_array(root, "abi_type_contracts");
    result.lowering_plan = json::required(root, "lowering_plan");
    const auto& plan = json::object(result.lowering_plan, "$.lowering_plan");
    if (json::string(json::required(plan, "format", "$.lowering_plan"), "$.lowering_plan.format") != "flowcore.lowering_plan") throw json::Error("$.lowering_plan.format", "unsupported lowering plan format");
    if (json::integer(json::required(plan, "version", "$.lowering_plan"), "$.lowering_plan.version") != 1) throw json::Error("$.lowering_plan.version", "unsupported lowering plan version");
    const auto& operations = required_array(plan, "operations", "$.lowering_plan");
    std::set<json::Integer> operation_ids;
    for (std::size_t index = 0; index < operations.size(); ++index) {
        const auto path = "$.lowering_plan.operations[" + std::to_string(index) + "]";
        const auto& operation = json::object(operations[index], path);
        const auto id = json::integer(json::required(operation, "id", path), path + ".id");
        if (id < 0) throw json::Error(path + ".id", "operation identity must be non-negative");
        if (!operation_ids.insert(id).second) throw json::Error(path + ".id", "duplicate operation identity");
    }
    for (const auto& item : required_array(root, "effect_facts")) {
        const auto& fact = json::object(item, "$.effect_facts[]");
        if (const auto* certainty = json::optional(fact, "certainty")) if (json::string(*certainty, "$.effect_facts[].certainty") == "proven") ++result.proven_pure_count;
    }
    for (const auto& item : required_array(root, "parallel_candidates")) {
        const auto& candidate = json::object(item, "$.parallel_candidates[]");
        if (const auto* proof = json::optional(candidate, "proof")) if (json::string(*proof, "$.parallel_candidates[].proof") == "pure-callee-disjoint-inputs") ++result.independent_candidate_count;
    }
    result.dependency_matrix = parse_matrix(root);
    return result;
}

inline json::Value matrix_entries(const MatrixView& matrix) {
    json::Array entries;
    for (const auto& entry : matrix.entries) entries.emplace_back(json::Object{{"column", entry.column}, {"row", entry.row}, {"value", entry.value}});
    return entries;
}

struct ExecutionPlan {
    Header artifact; std::string source_path; json::Array targets; json::Array external_operations;
    json::Array abi_type_contracts; json::Value lowering_plan; MatrixView dependency_matrix;
};

inline MatrixView execution_matrix(const json::Object& root) {
    const auto& view = required_object(root, "graph_projection");
    MatrixView result;
    result.name = json::string(json::required(view, "name", "$.graph_projection"), "$.graph_projection.name");
    if (result.name != "region_dependency") throw json::Error("$.graph_projection.name", "unsupported graph projection");
    result.rows = json::integer(json::required(view, "rows", "$.graph_projection"), "$.graph_projection.rows");
    result.columns = json::integer(json::required(view, "columns", "$.graph_projection"), "$.graph_projection.columns");
    if (result.rows < 0 || result.columns < 0) throw json::Error("$.graph_projection", "matrix dimensions must be non-negative");
    result.semiring = json::string(json::required(view, "semiring", "$.graph_projection"), "$.graph_projection.semiring");
    result.storage = json::string(json::required(view, "storage", "$.graph_projection"), "$.graph_projection.storage");
    const auto& entries = required_array(view, "entries", "$.graph_projection");
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto path = "$.graph_projection.entries[" + std::to_string(index) + "]";
        const auto& entry = json::object(entries[index], path);
        MatrixEntry parsed;
        parsed.row = json::integer(json::required(entry, "row", path), path + ".row");
        parsed.column = json::integer(json::required(entry, "column", path), path + ".column");
        if (const auto* item = json::optional(entry, "value")) parsed.value = json::boolean(*item, path + ".value");
        if (parsed.row < 0 || parsed.row >= result.rows) throw json::Error(path + ".row", "matrix row is outside declared dimensions");
        if (parsed.column < 0 || parsed.column >= result.columns) throw json::Error(path + ".column", "matrix column is outside declared dimensions");
        result.entries.push_back(parsed);
    }
    return result;
}

inline ExecutionPlan execution_plan(const json::Value& value) {
    ExecutionPlan result; result.artifact = require_header(value, "flowparallel.execution_plan", 1);
    if (result.artifact.status != "ready") return result;
    const auto& root = json::object(value);
    const auto& source = required_object(root, "source");
    result.source_path = json::string(json::required(source, "path", "$.source"), "$.source.path");
    result.targets = required_array(root, "targets");
    result.external_operations = required_array(root, "external_operations");
    result.abi_type_contracts = required_array(root, "abi_type_contracts");
    result.lowering_plan = json::required(root, "lowering_plan");
    const auto& plan = json::object(result.lowering_plan, "$.lowering_plan");
    if (json::string(json::required(plan, "format", "$.lowering_plan"), "$.lowering_plan.format") != "flowcore.lowering_plan") throw json::Error("$.lowering_plan.format", "unsupported lowering plan format");
    if (json::integer(json::required(plan, "version", "$.lowering_plan"), "$.lowering_plan.version") != 1) throw json::Error("$.lowering_plan.version", "unsupported lowering plan version");
    result.dependency_matrix = execution_matrix(root);
    return result;
}

struct ProviderDecision { Header artifact; std::string provider; std::string representation; std::string reason; std::string fallback; };
inline ProviderDecision provider_decision(const json::Value& value) {
    ProviderDecision result; result.artifact = require_header(value, "flowparallel.graph_provider_decision", 1);
    if (result.artifact.status != "verified") throw json::Error("$.status", "provider decision is not verified");
    const auto& root = json::object(value);
    result.provider = json::string(json::required(root, "provider"), "$.provider");
    result.representation = json::string(json::required(root, "representation"), "$.representation");
    if (const auto* reason = json::optional(root, "reason")) result.reason = json::string(*reason, "$.reason");
    if (const auto* fallback = json::optional(root, "fallback")) result.fallback = json::string(*fallback, "$.fallback");
    return result;
}

} // namespace flowcontracts
