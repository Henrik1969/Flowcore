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

struct Requirement { std::string contract, library, convention, symbol, effect, parameter_types, return_type; };
struct Grant { std::string library, symbol, convention, effect; };

struct Options { std::string report_path, policy_path, abi_manifest_path; };

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--policy") {
            if (++i >= argc) throw std::runtime_error("--policy requires a path");
            options.policy_path = argv[i];
        } else if (argument == "--abi-manifest") {
            if (++i >= argc) throw std::runtime_error("--abi-manifest requires a path");
            options.abi_manifest_path = argv[i];
        } else if (argument == "-h" || argument == "--help" || argument == "-?" || argument == "-a" || argument == "--about" || argument == "-v" || argument == "--version") {
            continue;
        } else if (!argument.empty() && argument.front() == '-') {
            throw std::runtime_error("unknown option '" + argument + "'");
        } else if (options.report_path.empty()) {
            options.report_path = argument;
        } else {
            throw std::runtime_error("too many input paths");
        }
    }
    return options;
}

std::string read_input(const Options& options) {
    std::ostringstream input;
    if (!options.report_path.empty()) { std::ifstream file(options.report_path); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

std::string read_path(const std::string& path, const char* description) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error(std::string("cannot open ") + description);
    std::ostringstream input;
    input << file.rdbuf();
    return input.str();
}

std::vector<Grant> read_policy(const std::string& path) {
    std::vector<Grant> grants;
    if (path.empty()) return grants;
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open binding policy");
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.front() == '#') continue;
        std::istringstream words(line);
        std::string verb; Grant grant;
        words >> verb >> grant.library >> grant.symbol >> grant.convention >> grant.effect;
        if (!words || verb != "allow") throw std::runtime_error("invalid binding policy line");
        grants.push_back(std::move(grant));
    }
    return grants;
}

bool granted(const std::vector<Grant>& grants, const Requirement& requirement) {
    for (const auto& grant : grants) if (grant.library == requirement.library && grant.symbol == requirement.symbol && grant.convention == requirement.convention && grant.effect == requirement.effect) return true;
    return false;
}

std::string value(const std::string& object, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    return std::regex_search(object, match, pattern) ? match[1].str() : std::string{};
}

std::string json_string(const std::string& text) {
    std::string result = "\"";
    for (const char character : text) {
        if (character == '\\' || character == '"') result += '\\';
        result += character;
    }
    result += '"';
    return result;
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
        result.push_back({value(object, "contract"), value(object, "library"), value(object, "convention"), value(object, "symbol"), value(object, "effect"), value(object, "parameter_types"), value(object, "return_type")});
        cursor = end + 1;
    }
    return result;
}

bool manifest_verifies_aggregates(const std::string& report, const std::string& manifest) {
    if (manifest.find("\"format\":\"flowcore.abi_manifest\"") == std::string::npos && manifest.find("\"format\": \"flowcore.abi_manifest\"") == std::string::npos) throw std::runtime_error("unsupported ABI manifest format");
    if (manifest.find("\"version\":1") == std::string::npos && manifest.find("\"version\": 1") == std::string::npos) throw std::runtime_error("unsupported ABI manifest version");
    const auto layouts = report.find("\"aggregate_abi_layouts\"");
    if (layouts == std::string::npos) return false;
    const auto point = report.find("\"name\":\"Point\"", layouts);
    if (point != std::string::npos && manifest.find("\"name\":\"Point\"") == std::string::npos && manifest.find("\"name\": \"Point\"") == std::string::npos) throw std::runtime_error("ABI manifest does not declare aggregate Point");
    return point != std::string::npos;
}

int verify(const std::string& report, const std::string& policy_path, const std::string& abi_manifest_path) {
    if (report.find("\"format\": \"flowanalyst.semantic_report\"") == std::string::npos && report.find("\"format\":\"flowanalyst.semantic_report\"") == std::string::npos) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (report.find("\"version\": 1") == std::string::npos && report.find("\"version\":1") == std::string::npos) throw std::runtime_error("unsupported Flowanalyst report version");
    if (report.find("\"status\": \"ok\"") == std::string::npos && report.find("\"status\":\"ok\"") == std::string::npos) {
        std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not ready\"\n}\n";
        return 2;
    }
    const auto needed = requirements(report);
    const bool aggregate_manifest_verified = !abi_manifest_path.empty() && manifest_verifies_aggregates(report, read_path(abi_manifest_path, "ABI manifest"));
    const auto profile = value(report, "lowering_profile");
    const Requirement* lowering_requirement = nullptr;
    if (profile == "abi_abs_main") for (const auto& item : needed) if (item.symbol == "abs") lowering_requirement = &item;
    if (profile == "abi_strlen_main") for (const auto& item : needed) if (item.symbol == "strlen") lowering_requirement = &item;
    if (profile == "abi_kernel_getpid_main") for (const auto& item : needed) if (item.symbol == "getpid") lowering_requirement = &item;
    if (profile == "abi_kernel_clock_main") for (const auto& item : needed) if (item.symbol == "clock_gettime") lowering_requirement = &item;
    if (profile == "abi_kernel_random_main") for (const auto& item : needed) if (item.symbol == "getrandom") lowering_requirement = &item;
    if (profile == "abi_kernel_uname_main") for (const auto& item : needed) if (item.symbol == "uname") lowering_requirement = &item;
    if (profile == "abi_kernel_openat_main") for (const auto& item : needed) if (item.symbol == "openat") lowering_requirement = &item;
    if (profile == "abi_kernel_read_main") for (const auto& item : needed) if (item.symbol == "read") lowering_requirement = &item;
    if (profile == "abi_kernel_write_main") for (const auto& item : needed) if (item.symbol == "write") lowering_requirement = &item;
    if (profile == "abi_kernel_lseek_main") for (const auto& item : needed) if (item.symbol == "lseek") lowering_requirement = &item;
    if (profile == "abi_kernel_unlinkat_main") for (const auto& item : needed) if (item.symbol == "unlinkat") lowering_requirement = &item;
    for (const auto& item : needed) if (item.symbol == "rmdir" || item.symbol == "pipe2" || item.symbol == "fork" || item.symbol == "waitpid" || item.symbol == "socketpair" || item.symbol == "socket" || item.symbol == "bind" || item.symbol == "listen" || item.symbol == "poll" || item.symbol == "accept4" || item.symbol == "connect" || item.symbol == "unshare" || item.symbol == "sethostname" || item.symbol == "gethostname") if (profile.find("abi_kernel_") == 0) lowering_requirement = &item;
    if (profile == "flowcat_argv_main") for (const auto& item : needed) if (item.symbol == "puts") lowering_requirement = &item;
    const auto grants = read_policy(policy_path);
    const std::vector<std::string> supported_types = {"c_int", "c_long", "c_size_t", "c_string", "c_pointer"};
    auto supported_type = [&](const std::string& type) { for (const auto& candidate : supported_types) if (candidate == type) return true; return false; };
    std::map<std::string, void*> handles;
    std::vector<std::string> failures;
    for (const auto& item : needed) {
        if (!granted(grants, item)) failures.push_back(item.library + ": symbol '" + item.symbol + "' denied by capability policy");
        if (item.convention != "c") failures.push_back(item.symbol + ": unsupported calling convention '" + item.convention + "'");
        if (!supported_type(item.return_type)) failures.push_back(item.symbol + ": unsupported return ABI type '" + item.return_type + "'");
        std::size_t start = 0;
        while (start < item.parameter_types.size()) {
            const auto end = item.parameter_types.find(',', start);
            const auto type = item.parameter_types.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!supported_type(type)) {
                if (aggregate_manifest_verified && type == "Point") failures.push_back(item.symbol + ": aggregate ABI manifest verified; aggregate call lowering is not implemented");
                else failures.push_back(item.symbol + ": unsupported parameter ABI type '" + type + "'");
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
        if (!granted(grants, item)) continue;
        if (!handles.count(item.library)) handles[item.library] = dlopen(item.library.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handles[item.library]) { failures.push_back(item.library + ": library unavailable"); continue; }
        if (!dlsym(handles[item.library], item.symbol.c_str())) failures.push_back(item.library + ": symbol '" + item.symbol + "' unavailable");
    }
    for (const auto& [library, handle] : handles) if (handle) dlclose(handle);
    if (!failures.empty()) {
        std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"provider\": \"dlopen+dlsym\",\n  \"failures\": [";
        for (std::size_t i = 0; i < failures.size(); ++i) { if (i) std::cout << ','; std::cout << '"' << failures[i] << '"'; }
        std::cout << "]";
        if (aggregate_manifest_verified) std::cout << ",\n  \"aggregate_abi\": \"verified\"";
        std::cout << "\n}\n";
        return 2;
    }
    std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"ready\",\n  \"lowering_profile\": " << (profile.empty() ? "\"none\"" : "\"" + profile + "\"") << ",\n  \"provider\": {\"name\": \"dlopen+dlsym\", \"requirements\": " << needed.size() << "},\n  \"symbols\": [";
    for (std::size_t i = 0; i < needed.size(); ++i) { if (i) std::cout << ','; std::cout << '"' << needed[i].symbol << '"'; }
    const bool file_profile = profile == "flowcat_file_main";
    std::cout << "],\n  \"capabilities\": [";
    for (std::size_t i = 0; i < needed.size(); ++i) {
        if (i) std::cout << ',';
        const auto& item = needed[i];
        std::cout << "{\"contract\":" << json_string(item.contract)
                  << ",\"library\":" << json_string(item.library)
                  << ",\"symbol\":" << json_string(item.symbol)
                  << ",\"convention\":" << json_string(item.convention)
                  << ",\"effect\":" << json_string(item.effect)
                  << ",\"parameter_types\":" << json_string(item.parameter_types)
                  << ",\"return_type\":" << json_string(item.return_type)
                  << ",\"status\":\"authorized\"}";
    }
    std::cout << "],\n  \"lowering_plan\": {\"kind\": " << (file_profile ? "\"capability_sequence\"" : (lowering_requirement ? "\"external_call\"" : "\"none\""));
    if (lowering_requirement) std::cout << ",\"symbol\":" << json_string(lowering_requirement->symbol) << ",\"parameter_types\":" << json_string(lowering_requirement->parameter_types) << ",\"return_type\":" << json_string(lowering_requirement->return_type);
    std::cout << "},\n  \"policy\": {\"status\": \"authorized\", \"grants\": " << grants.size() << "},\n  \"abi\": {\"convention\": \"c\", \"signature_verified\": true, \"sizeof_int\": " << sizeof(int) << ", \"sizeof_long\": " << sizeof(long) << ", \"sizeof_size_t\": " << sizeof(std::size_t) << ", \"sizeof_pointer\": " << sizeof(void*) << "},\n  \"execution\": \"not-performed\"\n}\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        if (argc >= 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") { std::cout << "flowbind - verify and authorize external provider bindings\n\nUsage: flowbind [--policy policy.conf] [--abi-manifest manifest.json] [semantic-report.json]\n       flowmini ... | flowanalyst | flowbind --policy policy.conf\n\nPolicy: one exact grant per line: allow LIBRARY SYMBOL CONVENTION EFFECT\nABI manifest: provider-owned aggregate layout evidence\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n\nMore help: Flowbind/README.md\n"; return 0; }
            if (option == "-a" || option == "--about") { std::cout << "Flowbind verifies declared external libraries and symbols without executing them.\nMore help: Flowbind/README.md\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        return verify(read_input(options), options.policy_path, options.abi_manifest_path);
    } catch (const std::exception& error) { std::cerr << "flowbind error: " << error.what() << '\n'; return 1; }
}
