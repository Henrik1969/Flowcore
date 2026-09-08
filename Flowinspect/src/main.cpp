#include <flowcontracts/artifacts.hpp>
#include <flowcontracts/validate.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using flowcontracts::json::Array;
using flowcontracts::json::Integer;
using flowcontracts::json::Object;
using flowcontracts::json::Value;

constexpr std::string_view VERSION = "0.1.0";

const Value* field(const Value& value, std::string_view name) {
    if (const auto* object = std::get_if<Object>(&value)) {
        const auto found = object->find(std::string(name));
        return found == object->end() ? nullptr : &found->second;
    }
    return nullptr;
}

std::string string_field(const Value& value, std::string_view name, std::string fallback = {}) {
    const auto* item = field(value, name);
    if (item && std::holds_alternative<std::string>(*item)) return std::get<std::string>(*item);
    return fallback;
}

const Array& array_field(const Value& value, std::string_view name) {
    static const Array empty;
    const auto* item = field(value, name);
    return item && std::holds_alternative<Array>(*item) ? std::get<Array>(*item) : empty;
}

Integer integer_field(const Value& value, std::string_view name, Integer fallback = 0) {
    const auto* item = field(value, name);
    if (!item || !std::holds_alternative<flowcontracts::json::Number>(*item)) return fallback;
    const auto& number = std::get<flowcontracts::json::Number>(*item);
    return std::holds_alternative<Integer>(number) ? std::get<Integer>(number) : fallback;
}

std::string source_path(const Value& root) {
    const auto* source = field(root, "source");
    return source ? string_field(*source, "path") : std::string{};
}

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowinspect [artifact.json]");
    std::ostringstream input;
    if (argc == 2) {
        std::ifstream file(argv[1]);
        if (!file) throw std::runtime_error("cannot open artifact");
        input << file.rdbuf();
    } else {
        input << std::cin.rdbuf();
    }
    return input.str();
}

std::string artifact_header(const Value& root, std::string_view name) {
    const auto* item = field(root, name);
    if (!item || !std::holds_alternative<std::string>(*item)) throw flowcontracts::json::Error("$." + std::string(name), "required string field is missing");
    return std::get<std::string>(*item);
}

void print_common(const std::string& format, Integer version, const std::string& status, const std::string& source) {
    std::cout << "Artifact\n"
              << "  format:   " << format << "\n"
              << "  version:  " << version << "\n"
              << "  source:   " << (source.empty() ? "(not declared)" : source) << "\n"
              << "  status:   " << (status.empty() ? "(not declared)" : status) << "\n";
}

void print_reason_summary(const Array& rejections) {
    std::map<std::string, std::size_t> reasons;
    for (const auto& item : rejections) {
        const auto reason = string_field(item, "reason", "unknown");
        ++reasons[reason];
    }
    if (reasons.empty()) return;
    std::cout << "\nRejections\n";
    for (const auto& [reason, count] : reasons) std::cout << "  " << reason << ": " << count << "\n";
}

void inspect_semantic(const Value& root) {
    const auto report = flowcontracts::semantic_report(root);
    const auto& facts = array_field(root, "facts");
    const Integer symbols = facts.empty() ? 0 : integer_field(facts.front(), "symbols");
    const auto& regions = array_field(root, "analysis_regions");
    print_common(report.artifact.format, report.artifact.version, report.artifact.status, report.source_path);
    std::cout << "\nSummary\n"
              << "  symbols:               " << symbols << "\n"
              << "  analysis regions:      " << regions.size() << "\n"
              << "  external operations:   " << report.external_operations.size() << "\n"
              << "  parallel candidates:   " << report.parallel_candidates.size() << "\n"
              << "  parallel rejections:   " << report.parallel_rejections.size() << "\n"
              << "  diagnostics:           " << array_field(root, "diagnostics").size() << "\n";
    print_reason_summary(report.parallel_rejections);
}

void inspect_execution(const Value& root) {
    const auto plan = flowcontracts::execution_plan(root);
    print_common(plan.artifact.format, plan.artifact.version, plan.artifact.status, plan.source_path);
    std::cout << "\nSummary\n"
              << "  external operations:   " << plan.external_operations.size() << "\n"
              << "  parallel candidates:   " << plan.parallel_candidates.size() << "\n"
              << "  parallel rejections:   " << plan.parallel_rejections.size() << "\n"
              << "  dependency entries:    " << plan.dependency_matrix.entries.size() << "\n"
              << "  fallback:              cpu.serial\n";
    print_reason_summary(plan.parallel_rejections);
}

void inspect_optimization(const Value& root) {
    flowcontracts::validate_optimization_report(root);
    const auto source = source_path(root);
    const auto& lowering = *field(root, "lowering_plan");
    print_common(artifact_header(root, "format"), integer_field(root, "version"), string_field(root, "status"), source);
    std::cout << "\nSummary\n"
              << "  operations:            " << array_field(lowering, "operations").size() << "\n"
              << "  transforms:            " << array_field(root, "transforms").size() << "\n"
              << "  projections:           " << array_field(root, "projections").size() << "\n"
              << "  targets:               " << array_field(root, "targets").size() << "\n";
}

void inspect_lowering(const Value& root) {
    flowcontracts::validate_lowering_plan(root);
    const auto source = source_path(root);
    print_common(artifact_header(root, "format"), integer_field(root, "version"), string_field(root, "status"), source);
    std::cout << "\nSummary\n"
              << "  operations:            " << array_field(root, "operations").size() << "\n"
              << "  functions:             " << array_field(root, "functions").size() << "\n";
}

void inspect_frontend(const Value& root) {
    const auto version = integer_field(root, "version");
    if (version != 2) throw flowcontracts::json::Error("$.version", "unsupported flowmini.frontend_bundle version " + std::to_string(version));
    const auto* ast = field(root, "ast");
    const auto* symbols = field(root, "symbol_table");
    if (!ast || !std::holds_alternative<Object>(*ast)) throw flowcontracts::json::Error("$.ast", "required object is missing");
    if (!symbols || !std::holds_alternative<Object>(*symbols)) throw flowcontracts::json::Error("$.symbol_table", "required object is missing");
    print_common(artifact_header(root, "format"), version, "available", source_path(root));
    std::cout << "\nSummary\n"
              << "  declarations:          " << array_field(*ast, "declaration_pool").size() << "\n"
              << "  expressions:           " << array_field(*ast, "expression_pool").size() << "\n"
              << "  symbols:               " << array_field(*symbols, "symbols").size() << "\n"
              << "  scopes:                " << array_field(*symbols, "scopes").size() << "\n";
}

int run(const Value& root) {
    const auto format = artifact_header(root, "format");
    const auto* version_value = field(root, "version");
    if (!version_value || !std::holds_alternative<flowcontracts::json::Number>(*version_value))
        throw flowcontracts::json::Error("$.version", "required integer field is missing");
    const auto version_number = std::get<flowcontracts::json::Number>(*version_value);
    if (!std::holds_alternative<Integer>(version_number))
        throw flowcontracts::json::Error("$.version", "required integer field is not an integer");
    const auto version = std::get<Integer>(version_number);
    const auto unsupported_version = [&](Integer expected) {
        if (version != expected) throw std::runtime_error("unsupported " + format + " version " + std::to_string(version));
    };
    if (format == "flowanalyst.semantic_report") { unsupported_version(1); inspect_semantic(root); }
    else if (format == "flowparallel.execution_plan") { unsupported_version(1); inspect_execution(root); }
    else if (format == "flowoptimize.optimization_report") { unsupported_version(1); inspect_optimization(root); }
    else if (format == "flowcore.lowering_plan") {
        if (version != 1 && version != 2) throw std::runtime_error("unsupported " + format + " version " + std::to_string(version));
        inspect_lowering(root);
    }
    else if (format == "flowmini.frontend_bundle") { unsupported_version(2); inspect_frontend(root); }
    else throw std::runtime_error("unsupported artifact format '" + format + "'");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help")) {
            std::cout << "flowinspect " << VERSION << " - inspect versioned Flowcore artifacts\n\n"
                         "Usage: flowinspect [artifact.json]\n"
                         "       flowmini ... | flowanalyst | flowinspect\n\n"
                         "Exit status: 0 inspected, 1 usage/I/O, 2 malformed, 3 unsupported format/version.\n";
            return 0;
        }
        return run(flowcontracts::json::parse(read_input(argc, argv)));
    } catch (const flowcontracts::json::Error& error) {
        std::cerr << "flowinspect malformed artifact: " << error.what() << '\n';
        return 2;
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        std::cerr << "flowinspect error: " << message << '\n';
        return message.rfind("unsupported artifact format", 0) == 0 || message.rfind("unsupported ", 0) == 0 ? 3 : 1;
    } catch (const std::exception& error) {
        std::cerr << "flowinspect error: " << error.what() << '\n';
        return 1;
    }
}
