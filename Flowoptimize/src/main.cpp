#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view VERSION = "0.1.0";

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowoptimize [semantic-report.json]");
    std::ostringstream input;
    if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has_field(std::string_view input, std::string_view field, std::string_view value) {
    const std::string needle = "\"" + std::string(field) + "\": \"" + std::string(value) + "\"";
    const std::string compact = "\"" + std::string(field) + "\":\"" + std::string(value) + "\"";
    return input.find(needle) != std::string_view::npos || input.find(compact) != std::string_view::npos;
}

int analyze(std::string_view report) {
    if (!has_field(report, "format", "flowanalyst.semantic_report")) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (report.find("\"version\": 1") == std::string_view::npos && report.find("\"version\":1") == std::string_view::npos) throw std::runtime_error("unsupported FlowAnalyst semantic report version");
    if (!has_field(report, "status", "ok")) {
        std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not accepted\",\n  \"transforms\": []\n}\n";
        return 2;
    }
    // This is deliberately a boundary implementation. No optimization is
    // claimed until the accepted semantic bundle and transformation contract
    // are complete.
    std::string profile = "none";
    for (const auto candidate : {"empty_program_main", "abi_abs_main", "abi_strlen_main"}) if (has_field(report, "lowering_profile", candidate)) profile = candidate;
    std::cout << "{\n  \"format\": \"flowoptimize.optimization_report\",\n"
                 "  \"version\": 1,\n"
                 "  \"status\": \"ready\",\n"
                 "  \"input\": {\"format\": \"flowanalyst.semantic_report\", \"version\": 1},\n"
                 "  \"lowering_profile\": \"" << profile << "\",\n"
                 "  \"transforms\": [],\n"
                 "  \"message\": \"optimization boundary reached; no transforms enabled\"\n"
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
        return analyze(read_input(argc, argv));
    } catch (const std::exception& error) { std::cerr << "flowoptimize error: " << error.what() << '\n'; return 1; }
}
