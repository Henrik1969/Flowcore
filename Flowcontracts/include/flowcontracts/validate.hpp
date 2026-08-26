#pragma once

#include <flowcontracts/artifacts.hpp>

#include <set>
#include <string>
#include <string_view>

namespace flowcontracts {

enum class ValidationClass { valid, blocked, invalid, unsupported };

struct ValidationResult {
    ValidationClass classification = ValidationClass::invalid;
    std::string format;
    json::Integer version = 0;
    std::string source;
    std::string path = "$";
    std::string reason;
};

inline void validate_identity_array(const json::Object& parent, std::string_view key,
                                    std::string_view identity, std::string_view path) {
    const auto& values = required_array(parent, key, path);
    std::set<json::Integer> identities;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto item_path = std::string(path) + "." + std::string(key) + "[" + std::to_string(index) + "]";
        const auto& item = json::object(values[index], item_path);
        const auto id = json::integer(json::required(item, identity, item_path), item_path + "." + std::string(identity));
        if (id < 0) throw json::Error(item_path + "." + std::string(identity), "identity must be non-negative");
        if (!identities.insert(id).second) throw json::Error(item_path + "." + std::string(identity), "duplicate identity");
    }
}

inline void validate_lowering_plan(const json::Value& value, std::string_view path = "$") {
    const auto& plan = json::object(value, path);
    if (json::string(json::required(plan, "format", path), std::string(path) + ".format") != "flowcore.lowering_plan")
        throw json::Error(std::string(path) + ".format", "unsupported lowering plan format");
    if (json::integer(json::required(plan, "version", path), std::string(path) + ".version") != 1)
        throw json::Error(std::string(path) + ".version", "unsupported lowering plan version");
    validate_identity_array(plan, "operations", "id", path);
}

inline void validate_frontend_bundle(const json::Value& value) {
    const auto& root = json::object(value);
    if (json::string(json::required(root, "format"), "$.format") != "flowmini.frontend_bundle") throw json::Error("$.format", "unsupported frontend bundle format");
    if (json::integer(json::required(root, "version"), "$.version") != 2) throw json::Error("$.version", "unsupported frontend bundle version");
    const auto& source = required_object(root, "source");
    (void)json::string(json::required(source, "path", "$.source"), "$.source.path");
    const auto& symbols = required_object(root, "symbol_table");
    validate_identity_array(symbols, "symbols", "id", "$.symbol_table");
    validate_identity_array(symbols, "scopes", "id", "$.symbol_table");
    const auto& ast = required_object(root, "ast");
    validate_identity_array(ast, "expression_pool", "id", "$.ast");
    validate_identity_array(ast, "statement_pool", "id", "$.ast");
    validate_identity_array(ast, "block_pool", "id", "$.ast");
    validate_identity_array(ast, "declaration_pool", "id", "$.ast");
}

inline void validate_binding_report(const json::Value& value) {
    const auto artifact = require_header(value, "flowbind.binding_report", 1);
    if (artifact.status != "ready") return;
    const auto& root = json::object(value);
    const auto& capabilities = required_array(root, "capabilities");
    for (std::size_t index = 0; index < capabilities.size(); ++index) {
        const auto path = "$.capabilities[" + std::to_string(index) + "]";
        const auto& item = json::object(capabilities[index], path);
        for (const auto field : {"contract", "library", "symbol", "convention", "effect", "parameter_types", "return_type", "status"})
            (void)json::string(json::required(item, field, path), path + "." + field);
    }
}

inline void validate_optimization_report(const json::Value& value) {
    const auto artifact = require_header(value, "flowoptimize.optimization_report", 1);
    if (artifact.status != "ready") return;
    const auto& root = json::object(value);
    validate_lowering_plan(json::required(root, "lowering_plan"), "$.lowering_plan");
    (void)required_array(root, "targets");
    const auto& transforms = required_array(root, "transforms");
    for (std::size_t index = 0; index < transforms.size(); ++index) {
        const auto path = "$.transforms[" + std::to_string(index) + "]";
        const auto& transform = json::object(transforms[index], path);
        (void)json::string(json::required(transform, "kind", path), path + ".kind");
        (void)json::boolean(json::required(transform, "semantics_preserved", path), path + ".semantics_preserved");
    }
    const auto& projections = required_array(root, "projections");
    for (std::size_t index = 0; index < projections.size(); ++index) {
        const auto path = "$.projections[" + std::to_string(index) + "]";
        const auto& projection = json::object(projections[index], path);
        const auto rows = json::integer(json::required(projection, "rows", path), path + ".rows");
        const auto columns = json::integer(json::required(projection, "columns", path), path + ".columns");
        if (rows < 0 || columns < 0) throw json::Error(path, "projection dimensions must be non-negative");
    }
}

inline void validate_lowering_report(const json::Value& value) {
    const auto artifact = require_header(value, "flowlower.lowering_report", 1);
    if (artifact.status == "blocked") return;
    const auto& root = json::object(value);
    const auto& backend = required_object(root, "backend");
    (void)json::string(json::required(backend, "name", "$.backend"), "$.backend.name");
    const auto& output = required_object(root, "artifact");
    (void)json::string(json::required(output, "backend", "$.artifact"), "$.artifact.backend");
    (void)json::string(json::required(output, "status", "$.artifact"), "$.artifact.status");
    const auto& ir = required_object(root, "ir");
    (void)json::string(json::required(ir, "format", "$.ir"), "$.ir.format");
}

inline void validate_abi_manifest(const json::Value& value) {
    const auto& root = json::object(value);
    if (json::integer(json::required(root, "version"), "$.version") != 1) throw json::Error("$.version", "unsupported ABI manifest version");
    (void)json::string(json::required(root, "provider"), "$.provider");
    const auto& types = required_array(root, "types");
    std::set<std::string> names;
    for (std::size_t index = 0; index < types.size(); ++index) {
        const auto path = "$.types[" + std::to_string(index) + "]";
        const auto& type = json::object(types[index], path);
        const auto name = json::string(json::required(type, "name", path), path + ".name");
        (void)json::integer(json::required(type, "size", path), path + ".size");
        (void)json::integer(json::required(type, "alignment", path), path + ".alignment");
        (void)required_array(type, "fields", path);
        if (!names.insert(name).second) throw json::Error(path + ".name", "duplicate ABI type identity");
    }
}

inline void validate_runtime_capabilities(const json::Value& value, std::string_view format) {
    const auto& root = json::object(value);
    if (json::integer(json::required(root, "version"), "$.version") != 1) throw json::Error("$.version", "unsupported runtime capability version");
    if (format == "frankencore.runtime_capabilities") {
        const auto& cuda = required_object(root, "cuda");
        (void)json::string(json::required(cuda, "status", "$.cuda"), "$.cuda.status");
        (void)json::integer(json::required(cuda, "device_count", "$.cuda"), "$.cuda.device_count");
    } else {
        (void)json::string(json::required(root, "status"), "$.status");
        (void)json::integer(json::required(root, "device_count"), "$.device_count");
    }
}

inline void validate_calibration(const json::Value& value, std::string_view format) {
    const auto artifact = require_header(value, format, 1);
    if (artifact.status == "verified") {
        const auto& root = json::object(value);
        const auto speedup = json::number(json::required(root, "end_to_end_speedup"), "$.end_to_end_speedup");
        if (speedup < 0.0) throw json::Error("$.end_to_end_speedup", "speedup must be non-negative");
    }
}

inline ValidationResult validate(const json::Value& value) {
    ValidationResult result;
    try {
        const auto& root = json::object(value);
        result.format = json::string(json::required(root, "format"), "$.format");
        result.version = json::integer(json::required(root, "version"), "$.version");
        if (const auto* source = json::optional(root, "source")) {
            const auto& object = json::object(*source, "$.source");
            if (const auto* path = json::optional(object, "path")) result.source = json::string(*path, "$.source.path");
        }
        if (result.format == "flowmini.frontend_bundle") validate_frontend_bundle(value);
        else if (result.format == "flowanalyst.semantic_report") (void)semantic_report(value);
        else if (result.format == "flowcore.lowering_plan") validate_lowering_plan(value);
        else if (result.format == "flowbind.binding_report") validate_binding_report(value);
        else if (result.format == "flowparallel.execution_plan") (void)execution_plan(value);
        else if (result.format == "flowparallel.graph_provider_decision") (void)provider_decision(value);
        else if (result.format == "flowoptimize.optimization_report") validate_optimization_report(value);
        else if (result.format == "flowlower.lowering_report") validate_lowering_report(value);
        else if (result.format == "flowcore.abi_manifest") validate_abi_manifest(value);
        else if (result.format == "frankencore.runtime_capabilities" || result.format == "flowcore.runtime_capabilities") validate_runtime_capabilities(value, result.format);
        else if (result.format == "flowparallel.matrix_benchmark" || result.format == "flowparallel.graph_cuda") validate_calibration(value, result.format);
        else return {ValidationClass::unsupported, result.format, result.version, result.source, "$.format", "artifact format is not supported"};
        if (const auto* status = json::optional(root, "status")) {
            const auto state = json::string(*status, "$.status");
            if (state == "blocked" || state == "error") return {ValidationClass::blocked, result.format, result.version, result.source, "$.status", "artifact reports " + state};
        }
        return {ValidationClass::valid, result.format, result.version, result.source, "$", "artifact is valid"};
    } catch (const json::Error& error) {
        return {ValidationClass::invalid, result.format, result.version, result.source, error.path(), error.reason()};
    } catch (const std::exception& error) {
        return {ValidationClass::invalid, result.format, result.version, result.source, "$", error.what()};
    }
}

inline std::string_view name(ValidationClass value) {
    switch (value) {
        case ValidationClass::valid: return "valid";
        case ValidationClass::blocked: return "blocked";
        case ValidationClass::invalid: return "invalid";
        case ValidationClass::unsupported: return "unsupported";
    }
    return "invalid";
}

} // namespace flowcontracts
