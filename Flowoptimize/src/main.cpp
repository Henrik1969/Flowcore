#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <set>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Options { std::string input_path; std::string provider_decision_path; };

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--provider-decision") {
            if (++index >= argc) throw std::runtime_error("--provider-decision requires a file");
            options.provider_decision_path = argv[index];
        } else if (!argument.empty() && argument[0] == '-') {
            throw std::runtime_error("unknown option: " + argument);
        } else if (options.input_path.empty()) options.input_path = argument;
        else throw std::runtime_error("only one input report is accepted");
    }
    return options;
}

std::string read_input(const Options& options) {
    std::ostringstream input;
    if (!options.input_path.empty()) { std::ifstream file(options.input_path); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has_field(std::string_view input, std::string_view field, std::string_view value) {
    const std::string needle = "\"" + std::string(field) + "\": \"" + std::string(value) + "\"";
    const std::string compact = "\"" + std::string(field) + "\":\"" + std::string(value) + "\"";
    return input.find(needle) != std::string_view::npos || input.find(compact) != std::string_view::npos;
}

bool has_top_level_format(std::string_view input, std::string_view value) {
    const auto first_field = input.find("\"format\"");
    if (first_field == std::string_view::npos) return false;
    const std::string expected = "\"format\": \"" + std::string(value) + "\"";
    return input.find(expected, first_field) == first_field;
}

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (const char character : value) {
        if (character == '\\' || character == '"') result.push_back('\\');
        result.push_back(character);
    }
    result.push_back('"');
    return result;
}

std::string source_path(std::string_view input) {
    const auto key = input.find("\"source\":");
    if (key == std::string_view::npos) return {};
    const auto path = input.find("\"path\":", key);
    if (path == std::string_view::npos) return {};
    auto first = input.find('"', path + 7);
    if (first == std::string_view::npos) return {};
    ++first;
    std::string result;
    for (auto index = first; index < input.size(); ++index) {
        if (input[index] == '"' && (index == first || input[index - 1] != '\\')) return result;
        result.push_back(input[index]);
    }
    return {};
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

std::size_t number_after_from(std::string_view input, std::string_view marker, std::size_t start) {
    const auto found = input.find(marker, start);
    if (found == std::string_view::npos) return 0;
    auto position = found + marker.size();
    while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    std::size_t value = 0;
    while (position < input.size() && input[position] >= '0' && input[position] <= '9') { value = value * 10 + static_cast<std::size_t>(input[position] - '0'); ++position; }
    return value;
}

struct MatrixStats { std::size_t input_entries = 0; std::size_t unique_entries = 0; };

MatrixStats matrix_stats(std::string_view input) {
    const auto entries = input.find("\"entries\":[");
    if (entries == std::string_view::npos) return {};
    const auto end = input.find(']', entries);
    if (end == std::string_view::npos) return {};
    std::set<std::string> unique;
    std::size_t count = 0;
    for (auto position = entries; position < end;) {
        const auto row_marker = input.find("\"row\":", position);
        if (row_marker == std::string_view::npos || row_marker >= end) break;
        const auto column_marker = input.find("\"column\":", row_marker);
        if (column_marker == std::string_view::npos || column_marker >= end) break;
        unique.insert(std::to_string(number_after_from(input, "\"row\":", row_marker)) + ":" + std::to_string(number_after_from(input, "\"column\":", column_marker)));
        ++count;
        position = column_marker + 9;
    }
    return {count, unique.size()};
}

std::string string_after(std::string_view input, std::string_view marker) {
    const auto start = input.find(marker);
    if (start == std::string_view::npos) return {};
    auto position = start + marker.size();
    while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    if (position >= input.size() || input[position] != '"') return {};
    ++position;
    std::string value;
    for (; position < input.size(); ++position) {
        if (input[position] == '"' && input[position - 1] != '\\') return value;
        value.push_back(input[position]);
    }
    return {};
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

int analyze(std::string_view report, std::string_view provider_decision) {
    const bool semantic_input = has_top_level_format(report, "flowanalyst.semantic_report");
    const bool parallel_input = has_top_level_format(report, "flowparallel.execution_plan");
    if (!semantic_input && !parallel_input) throw std::runtime_error("input is not a Flowanalyst semantic report or Flowparallel execution plan");
    if (report.find("\"version\": 1") == std::string_view::npos && report.find("\"version\":1") == std::string_view::npos) throw std::runtime_error("unsupported FlowAnalyst semantic report version");
    const bool accepted = semantic_input ? has_field(report, "status", "ok") : has_field(report, "status", "ready");
    if (!accepted) {
        std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not accepted\",\n  \"transforms\": []\n}\n";
        return 2;
    }
    const bool has_decision = !provider_decision.empty();
    if (has_decision && (!has_field(provider_decision, "format", "flowparallel.graph_provider_decision") || !has_field(provider_decision, "status", "verified"))) {
        std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"provider decision is not an accepted verified Flowparallel decision\",\n  \"transforms\": []\n}\n";
        return 2;
    }
    if (has_decision) {
        const auto provider = string_after(provider_decision, "\"provider\":");
        const auto representation = string_after(provider_decision, "\"representation\":");
        if ((provider != "cpu.reference" && provider != "cuda.cublas.boolean_threshold") || (representation != "sparse" && representation != "dense")) {
            std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"provider decision names an unsupported provider or representation\",\n  \"transforms\": []\n}\n";
            return 2;
        }
    }
    // This is deliberately a boundary implementation. No optimization is
    // claimed until the accepted semantic bundle and transformation contract
    // are complete.
    std::string profile = "none";
    for (const auto candidate : {"empty_program_main", "abi_abs_main", "abi_strlen_main", "test_licbinds_main", "abi_kernel_getpid_main", "abi_kernel_clock_main", "abi_kernel_random_main", "abi_kernel_uname_main", "abi_kernel_openat_main", "abi_kernel_read_main", "abi_kernel_write_main", "abi_kernel_lseek_main", "abi_kernel_unlinkat_main", "abi_kernel_rmdir_main", "abi_kernel_pipe2_main", "abi_kernel_fork_main", "abi_kernel_waitpid_main", "abi_kernel_socketpair_main", "abi_kernel_socket_main", "abi_kernel_bind_main", "abi_kernel_listen_main", "abi_kernel_poll_main", "abi_kernel_accept4_main", "abi_kernel_connect_main", "abi_kernel_unshare_main", "abi_kernel_sethostname_main", "abi_kernel_gethostname_main", "flowcat_argv_main", "flowcat_file_main"}) if (has_field(report, "lowering_profile", candidate)) profile = candidate;
    const auto input_format = parallel_input ? "flowparallel.execution_plan" : "flowanalyst.semantic_report";
    const bool has_region_matrix = report.find("\"name\":\"region_dependency\"") != std::string_view::npos || report.find("\"name\": \"region_dependency\"") != std::string_view::npos;
    const auto matrix_rows = has_region_matrix ? number_after(report, "\"rows\":") : 0;
    const auto matrix_columns = has_region_matrix ? number_after(report, "\"columns\":") : 0;
    const auto matrix = matrix_stats(report);
    const bool deduplication_applied = matrix.input_entries > matrix.unique_entries;
    const auto transform_status = deduplication_applied ? "applied" : "not-needed";
    const auto selected_provider = has_decision ? string_after(provider_decision, "\"provider\":") : "";
    const auto representation = has_decision ? string_after(provider_decision, "\"representation\":") : "";
    const auto decision_reason = has_decision ? string_after(provider_decision, "\"reason\":") : "";
    std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n"
                 "  \"version\": 1,\n"
                 "  \"status\": \"ready\",\n"
                 "  \"source\": {\"path\": " << quote(source_path(report)) << "},\n"
                 "  \"targets\": " << targets_projection(report) << ",\n"
                 "  \"input\": {\"format\": \"" << input_format << "\", \"version\": 1},\n"
                 "  \"lowering_profile\": \"" << profile << "\",\n"
                 "  \"projections\": [{\"kind\": \"graph_to_matrix\", \"name\": \"region_dependency\", \"status\": \"" << (has_region_matrix ? "available" : "not-present") << "\", \"rows\": " << matrix_rows << ", \"columns\": " << matrix_columns << ", \"semiring\": \"boolean\", \"storage\": \"coo\"}],\n"
                 "  \"provider_policy\": {\"selection\": \"runtime\", \"candidates\": [\"cpu\", \"cuda\"], \"cuda\": {\"requires\": [\"runtime capability\", \"measured cost benefit\", \"provider contract\"], \"fallback\": \"cpu\"}, \"decision\": {\"status\": \"" << (has_decision ? "verified" : "deferred") << "\", \"provider\": " << quote(selected_provider) << ", \"representation\": " << quote(representation) << ", \"reason\": " << quote(decision_reason) << "}},\n"
                 "  \"state\": {\"canonical_graph\": \"unchanged\", \"transformation\": \"identity\", \"decision_effect\": \"advisory_policy_only\"},\n"
                 "  \"transforms\": [{\"kind\": \"coo_deduplicate\", \"status\": \"" << transform_status << "\", \"input_entries\": " << matrix.input_entries << ", \"output_entries\": " << matrix.unique_entries << ", \"semantics_preserved\": true, \"proof\": \"Boolean relation duplicate idempotence\"}],\n"
                 "  \"message\": \"optimization boundary reached; derived matrix views are available; provider selection remains runtime policy\"\n"
                 "}\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") {
                std::cout << "flowoptimize - optimization boundary for Flowanalyst reports\n\n"
                             "Usage: flowoptimize [semantic-report.json]\n"
                             "       flowmini ... | flowanalyst | flowoptimize\n\n"
                             "Options: -h, -?, --help  show help\n"
                             "         -a, --about    show about information\n"
                             "         -v, --version  print the raw version number\n";
                return 0;
            }
            if (option == "-a" || option == "--about") { std::cout << "Flowoptimize is the inspectable optimization-stage boundary after Flowanalyst.\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        const auto options = parse_options(argc, argv);
        std::string provider_decision;
        if (!options.provider_decision_path.empty()) { std::ifstream file(options.provider_decision_path); if (!file) throw std::runtime_error("cannot open provider decision"); std::ostringstream text; text << file.rdbuf(); provider_decision = text.str(); }
        return analyze(read_input(options), provider_decision);
    } catch (const std::exception& error) { std::cerr << "flowoptimize error: " << error.what() << '\n'; return 1; }
}
