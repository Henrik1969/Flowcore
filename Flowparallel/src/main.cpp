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

bool has_field(std::string_view input, std::string_view field, std::string_view value) {
    const std::string spaced = "\"" + std::string(field) + "\": \"" + std::string(value) + "\"";
    const std::string compact = "\"" + std::string(field) + "\":\"" + std::string(value) + "\"";
    return input.find(spaced) != std::string_view::npos || input.find(compact) != std::string_view::npos;
}

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (const char character : value) { if (character == '\\' || character == '"') result.push_back('\\'); result.push_back(character); }
    result.push_back('"');
    return result;
}

std::string json_array_field(std::string_view input, std::string_view field) {
    const auto marker = input.find("\"" + std::string(field) + "\"");
    if (marker == std::string_view::npos) return "[]";
    auto position = input.find('[', marker); if (position == std::string_view::npos) return "[]";
    const auto start = position; int depth = 0; bool string = false; bool escaped = false;
    for (; position < input.size(); ++position) {
        const char c = input[position];
        if (string) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') string = false; continue; }
        if (c == '"') string = true; else if (c == '[') ++depth; else if (c == ']' && --depth == 0) return std::string(input.substr(start, position - start + 1));
    }
    return "[]";
}

std::string source_path(std::string_view input) {
    const auto source = input.find("\"source\":"); if (source == std::string_view::npos) return {};
    const auto path = input.find("\"path\":", source); if (path == std::string_view::npos) return {};
    auto first = input.find('"', path + 7); if (first == std::string_view::npos) return {};
    ++first; std::string result;
    for (auto index = first; index < input.size(); ++index) { if (input[index] == '"' && (index == first || input[index - 1] != '\\')) return result; result.push_back(input[index]); }
    return {};
}

std::string lowering_profile(std::string_view input) {
    for (const auto candidate : {"empty_program_main", "abi_abs_main", "abi_strlen_main", "test_licbinds_main", "abi_ncurses_main", "sel_main", "abi_kernel_getpid_main", "abi_kernel_clock_main", "abi_kernel_random_main", "abi_kernel_uname_main", "abi_kernel_openat_main", "abi_kernel_read_main", "abi_kernel_write_main", "abi_kernel_lseek_main", "abi_kernel_unlinkat_main", "abi_kernel_rmdir_main", "abi_kernel_pipe2_main", "abi_kernel_fork_main", "abi_kernel_waitpid_main", "abi_kernel_socketpair_main", "abi_kernel_socket_main", "abi_kernel_bind_main", "abi_kernel_listen_main", "abi_kernel_poll_main", "abi_kernel_accept4_main", "abi_kernel_connect_main", "abi_kernel_unshare_main", "abi_kernel_sethostname_main", "abi_kernel_gethostname_main", "flowcat_argv_main", "flowcat_file_main"}) if (has_field(input, "lowering_profile", candidate)) return candidate;
    return "none";
}

std::size_t proven_pure_count(std::string_view input) {
    std::size_t count = 0;
    for (const auto& marker : {std::string("\"certainty\": \"proven\""), std::string("\"certainty\":\"proven\"")})
        for (std::size_t position = 0; (position = input.find(marker, position)) != std::string_view::npos; position += marker.size()) ++count;
    return count;
}

std::size_t independent_candidate_count(std::string_view input) {
    const std::string marker = "\"proof\":\"pure-callee-disjoint-inputs\"";
    std::size_t count = 0;
    for (std::size_t position = 0; (position = input.find(marker, position)) != std::string_view::npos; position += marker.size()) ++count;
    return count;
}

std::size_t number_after(std::string_view input, std::string_view marker) {
    const auto start = input.find(marker);
    if (start == std::string_view::npos) return 0;
    auto position = start + marker.size();
    while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    std::size_t value = 0;
    while (position < input.size() && input[position] >= '0' && input[position] <= '9') {
        value = value * 10 + static_cast<std::size_t>(input[position] - '0');
        ++position;
    }
    return value;
}

std::string matrix_entries(std::string_view input) {
    const auto matrix = input.find("\"name\":\"region_dependency\"");
    if (matrix == std::string_view::npos) return "[]";
    const auto marker = input.find("\"entries\":[", matrix);
    if (marker == std::string_view::npos) return "[]";
    const auto first = input.find('[', marker);
    const auto end = input.find(']', first);
    if (first == std::string_view::npos || end == std::string_view::npos) return "[]";
    return std::string(input.substr(first, end - first + 1));
}

std::string targets_projection(std::string_view input) {
    auto marker = input.find("\"targets\":[");
    if (marker == std::string_view::npos) marker = input.find("\"targets\": [");
    if (marker == std::string_view::npos) return "[]";
    const auto first = input.find('[', marker);
    const auto end = input.find(']', first);
    if (first == std::string_view::npos || end == std::string_view::npos) return "[]";
    return std::string(input.substr(first, end - first + 1));
}

int analyze(std::string_view report) {
    if (!has_field(report, "format", "flowanalyst.semantic_report")) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (report.find("\"version\": 1") == std::string_view::npos && report.find("\"version\":1") == std::string_view::npos) throw std::runtime_error("unsupported Flowanalyst semantic report version");
    if (!has_field(report, "status", "ok")) {
        std::cout << "{\n  \"format\": \"flowparallel.execution_plan\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not accepted\"\n}\n";
        return 2;
    }
    const auto pure_count = proven_pure_count(report);
    const auto independent_count = independent_candidate_count(report);
    const auto external_operations = json_array_field(report, "external_operations");
    const bool has_region_matrix = report.find("\"name\":\"region_dependency\"") != std::string_view::npos || report.find("\"name\": \"region_dependency\"") != std::string_view::npos;
    const auto matrix_rows = has_region_matrix ? number_after(report, "\"rows\":") : 0;
    const auto matrix_columns = has_region_matrix ? number_after(report, "\"columns\":") : 0;
    std::cout << "{\n"
                 "  \"format\": \"flowparallel.execution_plan\",\n"
                 "  \"version\": 1,\n"
                 "  \"status\": \"ready\",\n"
                 "  \"source\": {\"path\": " << quote(source_path(report)) << "},\n"
                 "  \"targets\": " << targets_projection(report) << ",\n"
                 "  \"input\": {\"format\": \"flowanalyst.semantic_report\", \"version\": 1},\n"
                 "  \"lowering_profile\": " << quote(lowering_profile(report)) << ",\n"
                 "  \"external_operations\": " << external_operations << ",\n"
                 "  \"dependency_analysis\": {\"status\": \"available\", \"pure_callables\": " << pure_count << ", \"parallel_candidates\": " << independent_count << ", \"candidate_kind\": \"pure-callee-disjoint-inputs\"},\n"
                 "  \"graph_projection\": {\"kind\": \"graph_to_matrix\", \"name\": \"region_dependency\", \"status\": \"" << (has_region_matrix ? "available" : "not-present") << "\", \"rows\": " << matrix_rows << ", \"columns\": " << matrix_columns << ", \"semiring\": \"boolean\", \"storage\": \"coo\", \"entries\": " << matrix_entries(report) << "},\n"
                 "  \"cost_model\": {\"status\": \"deferred\", \"work_units\": \"runtime\", \"minimum_speedup\": 1.25, \"minimum_duration_ns\": \"policy\", \"calibration\": \"runtime\"},\n"
                 "  \"provider_selection\": {\"status\": \"deferred\", \"policy\": \"runtime\"},\n"
                 "  \"runtime\": {\"capabilities_format\": \"frankencore.runtime_capabilities\", \"required\": true},\n"
                 "  \"fallback\": {\"required\": true, \"provider\": \"cpu.serial\"},\n"
                 "  \"message\": \"parallel execution is policy- and runtime-deferred; no unsafe candidates emitted\"\n"
                 "}\n";
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
    } catch (const std::exception& error) { std::cerr << "flowparallel error: " << error.what() << '\n'; return 1; }
}
