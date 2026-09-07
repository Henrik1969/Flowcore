#include <flowcontracts/artifacts.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view VERSION = "0.1.0";

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowparallel [semantic-report.json]");
    std::ostringstream input;
    if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

flowcontracts::json::Value text(std::string value) { return flowcontracts::json::Value{std::move(value)}; }

void validate_approved_candidate(const flowcontracts::json::Object& candidate) {
    using namespace flowcontracts;
    using namespace flowcontracts::json;
    constexpr std::string_view path = "$.parallel_candidates[]";
    (void)integer(required(candidate, "call_expression", path), "$.parallel_candidates[].call_expression");
    (void)integer(required(candidate, "statement_id", path), "$.parallel_candidates[].statement_id");
    (void)string(required(candidate, "callee", path), "$.parallel_candidates[].callee");
    const auto& provenance = object(required(candidate, "provenance", path), "$.parallel_candidates[].provenance");
    (void)string(required(provenance, "source", "$.parallel_candidates[].provenance"), "$.parallel_candidates[].provenance.source");
    (void)string(required(provenance, "ast_path", "$.parallel_candidates[].provenance"), "$.parallel_candidates[].provenance.ast_path");
    const auto& evidence = object(required(candidate, "evidence", path), "$.parallel_candidates[].evidence");
    (void)boolean(required(evidence, "dependency_independent", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.dependency_independent");
    (void)boolean(required(evidence, "effect_compatible", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.effect_compatible");
    (void)boolean(required(evidence, "mutation_conflict", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.mutation_conflict");
    (void)string(required(evidence, "resource_compatibility", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.resource_compatibility");
    (void)array(required(evidence, "input_symbols", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.input_symbols");
    (void)integer(required(evidence, "output_symbol", "$.parallel_candidates[].evidence"), "$.parallel_candidates[].evidence.output_symbol");
    (void)array(required(candidate, "independent_with", path), "$.parallel_candidates[].independent_with");
}

int analyze(std::string_view input) {
    using namespace flowcontracts;
    using namespace flowcontracts::json;
    const auto report = semantic_report(parse(input));
    if (report.artifact.status != "ok") {
        std::cout << serialize(Object{{"format", text("flowparallel.execution_plan")},
                                     {"reason", text("semantic report is not accepted")},
                                     {"status", text("blocked")}, {"version", Integer{1}}}) << '\n';
        return 2;
    }
    const auto& matrix = report.dependency_matrix;
    json::Array approved_candidates;
    for (const auto& candidate : report.parallel_candidates) {
        const auto& object = json::object(candidate, "$.parallel_candidates[]");
        const auto proof = json::string(json::required(object, "proof", "$.parallel_candidates[]"), "$.parallel_candidates[].proof");
        const auto* proof_status = json::optional(object, "proof_status");
        if (proof != "pure-callee-disjoint-inputs" || !proof_status || json::string(*proof_status, "$.parallel_candidates[].proof_status") != "proven") continue;
        validate_approved_candidate(object);
        approved_candidates.push_back(candidate);
    }
    Object output{
        {"abi_type_contracts", report.abi_type_contracts},
        {"cost_model", Object{{"calibration", text("runtime")}, {"minimum_duration_ns", text("policy")},
                              {"minimum_speedup", 1.25}, {"status", text("deferred")}, {"work_units", text("runtime")}}},
        {"dependency_analysis", Object{{"candidate_kind", text("pure-callee-disjoint-inputs")},
                                       {"parallel_candidates", Integer{static_cast<Integer>(approved_candidates.size())}},
                                       {"pure_callables", Integer{static_cast<Integer>(report.proven_pure_count)}}, {"status", text("available")}}},
        {"external_operations", report.external_operations},
        {"fallback", Object{{"provider", text("cpu.serial")}, {"required", true}}},
        {"format", text("flowparallel.execution_plan")},
        {"graph_projection", Object{{"columns", matrix.columns}, {"entries", matrix_entries(matrix)},
                                    {"kind", text("graph_to_matrix")}, {"name", text(matrix.name)}, {"rows", matrix.rows},
                                    {"semiring", text(matrix.semiring)}, {"status", text("available")}, {"storage", text(matrix.storage)}}},
        {"input", Object{{"format", text("flowanalyst.semantic_report")}, {"version", Integer{1}}}},
        {"match_facts", report.match_facts},
        {"match_operations", report.match_facts},
        {"parallel_candidates", approved_candidates},
        {"parallel_rejections", report.parallel_rejections},
        {"lowering_plan", report.lowering_plan},
        {"message", text("parallel execution is policy- and runtime-deferred; no unsafe candidates emitted")},
        {"provider_selection", Object{{"policy", text("runtime")}, {"status", text("deferred")}}},
        {"runtime", Object{{"capabilities_format", text("frankencore.runtime_capabilities")}, {"required", true}}},
        {"source", Object{{"path", text(report.source_path)}}}, {"status", text("ready")},
        {"targets", report.targets}, {"version", Integer{1}}
    };
    std::cout << serialize(output) << '\n';
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") { std::cout << "flowparallel - runtime-deferred parallel execution planning\n\nUsage: flowparallel [semantic-report.json]\n       flowmini ... | flowanalyst | flowparallel\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n"; return 0; }
            if (option == "-a" || option == "--about") { std::cout << "Flowparallel derives a conservative, inspectable execution plan after semantic analysis.\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        return analyze(read_input(argc, argv));
    } catch (const flowcontracts::json::Error& error) {
        std::cerr << "flowparallel contract error: " << error.what() << '\n'; return 1;
    } catch (const std::exception& error) { std::cerr << "flowparallel error: " << error.what() << '\n'; return 1; }
}
