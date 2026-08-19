#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Requirement { std::string contract, library, convention, symbol, effect; };

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowbind [semantic-report.json]");
    std::ostringstream input;
    if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

std::string value(const std::string& object, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    return std::regex_search(object, match, pattern) ? match[1].str() : std::string{};
}

std::vector<Requirement> requirements(const std::string& report) {
    std::vector<Requirement> result;
    const auto begin = report.find("\"binding_requirements\"");
    if (begin == std::string::npos) return result;
    const auto open = report.find('[', begin);
    const auto close = report.find(']', open);
    if (open == std::string::npos || close == std::string::npos) return result;
    const std::string body = report.substr(open + 1, close - open - 1);
    std::size_t cursor = 0;
    while ((cursor = body.find('{', cursor)) != std::string::npos) {
        const auto end = body.find('}', cursor);
        if (end == std::string::npos) break;
        const auto object = body.substr(cursor, end - cursor + 1);
        result.push_back({value(object, "contract"), value(object, "library"), value(object, "convention"), value(object, "symbol"), value(object, "effect")});
        cursor = end + 1;
    }
    return result;
}

int verify(const std::string& report) {
    if (report.find("\"format\": \"flowanalyst.semantic_report\"") == std::string::npos && report.find("\"format\":\"flowanalyst.semantic_report\"") == std::string::npos) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (report.find("\"version\": 1") == std::string::npos && report.find("\"version\":1") == std::string::npos) throw std::runtime_error("unsupported Flowanalyst report version");
    if (report.find("\"status\": \"ok\"") == std::string::npos && report.find("\"status\":\"ok\"") == std::string::npos) {
        std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not ready\"\n}\n";
        return 2;
    }
    const auto needed = requirements(report);
    std::map<std::string, void*> handles;
    std::vector<std::string> failures;
    for (const auto& item : needed) {
        if (!handles.count(item.library)) handles[item.library] = dlopen(item.library.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handles[item.library]) { failures.push_back(item.library + ": library unavailable"); continue; }
        if (!dlsym(handles[item.library], item.symbol.c_str())) failures.push_back(item.library + ": symbol '" + item.symbol + "' unavailable");
    }
    for (const auto& [library, handle] : handles) if (handle) dlclose(handle);
    if (!failures.empty()) {
        std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"provider\": \"dlopen+dlsym\",\n  \"failures\": [";
        for (std::size_t i = 0; i < failures.size(); ++i) { if (i) std::cout << ','; std::cout << '"' << failures[i] << '"'; }
        std::cout << "]\n}\n";
        return 2;
    }
    std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"ready\",\n  \"provider\": {\"name\": \"dlopen+dlsym\", \"requirements\": " << needed.size() << "},\n  \"execution\": \"not-performed\"\n}\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") { std::cout << "flowbind - verify external provider bindings\n\nUsage: flowbind [semantic-report.json]\n       flowmini ... | flowanalyst | flowbind\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n\nMore help: Flowbind/README.md\n"; return 0; }
            if (option == "-a" || option == "--about") { std::cout << "Flowbind verifies declared external libraries and symbols without executing them.\nMore help: Flowbind/README.md\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        return verify(read_input(argc, argv));
    } catch (const std::exception& error) { std::cerr << "flowbind error: " << error.what() << '\n'; return 1; }
}
