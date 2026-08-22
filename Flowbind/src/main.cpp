#include <dlfcn.h>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Requirement { std::string contract, library, convention, symbol, effect, parameter_types, return_type; };
struct Grant { std::string library, symbol, convention, effect, parameter_types, return_type; bool exact_signature = false; };

struct Options { std::string report_path, policy_path, abi_manifest_path; };

struct Json;
using JsonArray = std::vector<Json>;
using JsonObject = std::map<std::string, Json>;
struct Json : std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> {
    using variant::variant;
};

class JsonParser {
public:
    explicit JsonParser(std::string text) : text_(std::move(text)) {}
    Json parse() { skip(); Json result = value(); skip(); if (at() != '\0') fail("trailing input"); return result; }
private:
    std::string text_;
    std::size_t position_ = 0;
    char at() const { return position_ < text_.size() ? text_[position_] : '\0'; }
    void skip() { while (std::isspace(static_cast<unsigned char>(at()))) ++position_; }
    [[noreturn]] void fail(const std::string& message) const { throw std::runtime_error("JSON: " + message); }
    void expect(char expected) { if (at() != expected) fail(std::string("expected '") + expected + "'"); ++position_; }
    void literal(std::string_view expected) { if (text_.compare(position_, expected.size(), expected) != 0) fail("invalid literal"); position_ += expected.size(); }
    Json value() {
        skip();
        switch (at()) {
            case '{': return object();
            case '[': return array();
            case '"': return string();
            case 't': literal("true"); return true;
            case 'f': literal("false"); return false;
            case 'n': literal("null"); return nullptr;
            default: return number();
        }
    }
    Json object() {
        JsonObject result; expect('{'); skip(); if (at() == '}') { ++position_; return result; }
        for (;;) {
            skip(); if (at() != '"') fail("object key must be a string");
            auto key = std::get<std::string>(string()); skip(); expect(':');
            if (!result.emplace(std::move(key), value()).second) fail("duplicate object key");
            skip(); if (at() == '}') { ++position_; return result; } expect(',');
        }
    }
    Json array() {
        JsonArray result; expect('['); skip(); if (at() == ']') { ++position_; return result; }
        for (;;) { result.push_back(value()); skip(); if (at() == ']') { ++position_; return result; } expect(','); }
    }
    Json string() {
        expect('"'); std::string result;
        while (at() != '"') {
            if (at() == '\0') fail("unterminated string");
            if (at() == '\\') {
                ++position_;
                switch (at()) {
                    case '"': case '\\': case '/': result += at(); ++position_; break;
                    case 'n': result += '\n'; ++position_; break;
                    case 'r': result += '\r'; ++position_; break;
                    case 't': result += '\t'; ++position_; break;
                    default: fail("unsupported string escape");
                }
            } else { result += at(); ++position_; }
        }
        ++position_; return result;
    }
    Json number() {
        const auto begin = position_; if (at() == '-') ++position_;
        while (std::isdigit(static_cast<unsigned char>(at()))) ++position_;
        if (at() == '.') { ++position_; while (std::isdigit(static_cast<unsigned char>(at()))) ++position_; }
        if (at() == 'e' || at() == 'E') { ++position_; if (at() == '+' || at() == '-') ++position_; while (std::isdigit(static_cast<unsigned char>(at()))) ++position_; }
        if (begin == position_) fail("expected value");
        return std::stod(text_.substr(begin, position_ - begin));
    }
};

const Json* json_field(const Json& value, std::string_view name) {
    const auto* object = std::get_if<JsonObject>(&value);
    if (!object) return nullptr;
    const auto it = object->find(std::string{name});
    return it == object->end() ? nullptr : &it->second;
}

std::string json_text(const Json* value) {
    return value && std::holds_alternative<std::string>(*value) ? std::get<std::string>(*value) : std::string{};
}

long long json_integer(const Json* value, const char* field_name) {
    if (!value || !std::holds_alternative<double>(*value)) throw std::runtime_error(std::string("JSON field '") + field_name + "' must be numeric");
    return static_cast<long long>(std::get<double>(*value));
}

const JsonArray& json_array(const Json* value, const char* field_name) {
    if (!value || !std::holds_alternative<JsonArray>(*value)) throw std::runtime_error(std::string("JSON field '") + field_name + "' must be an array");
    return std::get<JsonArray>(*value);
}

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
        const bool has_parameter_types = static_cast<bool>(words >> grant.parameter_types);
        const bool has_return_type = static_cast<bool>(words >> grant.return_type);
        grant.exact_signature = has_parameter_types || has_return_type;
        if (!grant.parameter_types.empty() && grant.parameter_types == "-") grant.parameter_types.clear();
        if (!grant.return_type.empty() && grant.return_type == "-") grant.return_type.clear();
        grants.push_back(std::move(grant));
    }
    return grants;
}

bool granted(const std::vector<Grant>& grants, const Requirement& requirement) {
    for (const auto& grant : grants) if (grant.library == requirement.library && grant.symbol == requirement.symbol && grant.convention == requirement.convention && grant.effect == requirement.effect && (!grant.exact_signature || (grant.parameter_types == requirement.parameter_types && grant.return_type == requirement.return_type))) return true;
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
    const Json root = JsonParser{report}.parse();
    std::vector<Requirement> result;
    for (const auto& item : json_array(json_field(root, "binding_requirements"), "binding_requirements")) {
        result.push_back({
            json_text(json_field(item, "contract")),
            json_text(json_field(item, "library")),
            json_text(json_field(item, "convention")),
            json_text(json_field(item, "symbol")),
            json_text(json_field(item, "effect")),
            json_text(json_field(item, "parameter_types")),
            json_text(json_field(item, "return_type"))
        });
    }
    return result;
}

void validate_lowering_plan(const std::string& report, const std::vector<Requirement>& needed) {
    const Json root = JsonParser{report}.parse();
    const Json* plan = json_field(root, "lowering_plan");
    if (plan == nullptr) return;
    if (json_text(json_field(*plan, "format")) != "flowcore.lowering_plan" ||
        json_integer(json_field(*plan, "version"), "lowering_plan.version") != 1) {
        throw std::runtime_error("unsupported lowering plan contract");
    }
    const auto& operations = json_array(json_field(*plan, "operations"), "lowering_plan.operations");
    for (const auto& operation : operations) {
        for (const auto& operand : json_array(json_field(operation, "operands"), "lowering operation operands")) {
            if (json_text(json_field(operand, "kind")) != "writable_storage") continue;
            if (json_text(json_field(operand, "type")) != "c_pointer") throw std::runtime_error("writable storage must have c_pointer carrier type");
            const Json* storage = json_field(operand, "storage");
            if (storage == nullptr || json_integer(json_field(*storage, "bytes"), "writable storage bytes") <= 0 ||
                json_text(json_field(*storage, "access")) != "read_write" ||
                json_text(json_field(*storage, "lifetime")) != "call") {
                throw std::runtime_error("invalid writable storage descriptor");
            }
        }
        if (json_text(json_field(operation, "kind")) != "external_call") continue;
        const Json* provider = json_field(operation, "provider");
        if (provider == nullptr) throw std::runtime_error("external lowering operation has no provider identity");
        const auto contract = json_text(json_field(*provider, "contract"));
        const auto library = json_text(json_field(*provider, "library"));
        const auto convention = json_text(json_field(*provider, "convention"));
        const auto symbol = json_text(json_field(*provider, "symbol"));
        const auto effect = json_text(json_field(*provider, "effect"));
        const auto parameter_types = json_text(json_field(*provider, "parameter_types"));
        const auto return_type = json_text(json_field(*provider, "return_type"));
        bool found = false;
        for (const auto& requirement : needed) {
            if (requirement.contract == contract && requirement.library == library && requirement.convention == convention &&
                requirement.symbol == symbol && requirement.effect == effect &&
                requirement.parameter_types == parameter_types && requirement.return_type == return_type) {
                found = true;
                break;
            }
        }
        if (!found) throw std::runtime_error("lowering operation provider does not match a semantic binding requirement");
        std::vector<std::string> expected_parameters;
        for (std::size_t start = 0; start < parameter_types.size();) {
            const auto end = parameter_types.find(',', start);
            expected_parameters.push_back(parameter_types.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
        const auto& operands = json_array(json_field(operation, "operands"), "external lowering operation operands");
        if (operands.size() != expected_parameters.size())
            throw std::runtime_error("external lowering operation operand count does not match its ABI contract");
        for (std::size_t index = 0; index < operands.size(); ++index) {
            if (json_text(json_field(operands[index], "type")) != expected_parameters[index])
                throw std::runtime_error("external lowering operation operand carrier does not match its ABI contract");
        }
        const Json* expected_resource = nullptr;
        for (const auto& type : json_array(json_field(root, "abi_type_contracts"), "abi_type_contracts")) {
            if (json_text(json_field(type, "contract")) == contract && json_text(json_field(type, "name")) == return_type &&
                !json_text(json_field(type, "cleanup")).empty()) {
                expected_resource = &type;
                break;
            }
        }
        const Json* resource = json_field(operation, "result_resource");
        if (expected_resource != nullptr) {
            if (resource == nullptr || json_text(json_field(*resource, "type")) != return_type ||
                json_text(json_field(*resource, "ownership")) != json_text(json_field(*expected_resource, "ownership")) ||
                json_text(json_field(*resource, "access")) != json_text(json_field(*expected_resource, "access")) ||
                json_text(json_field(*resource, "lifetime")) != json_text(json_field(*expected_resource, "lifetime")) ||
                json_text(json_field(*resource, "nullable")) != json_text(json_field(*expected_resource, "nullable")) ||
                json_text(json_field(*resource, "opaque")) != json_text(json_field(*expected_resource, "opaque")) ||
                json_text(json_field(*resource, "cleanup_capability")) != json_text(json_field(*expected_resource, "cleanup"))) {
                throw std::runtime_error("lowering result resource does not match its ABI type contract");
            }
            bool cleanup_present = false;
            const auto cleanup = json_text(json_field(*expected_resource, "cleanup"));
            for (const auto& candidate : operations) {
                const Json* candidate_provider = json_field(candidate, "provider");
                if (candidate_provider != nullptr && json_text(json_field(*candidate_provider, "contract")) == contract &&
                    json_text(json_field(*candidate_provider, "symbol")) == cleanup) {
                    cleanup_present = true;
                    break;
                }
            }
            if (!cleanup_present) throw std::runtime_error("lowering plan acquires a resource without its declared cleanup capability");
        } else if (resource != nullptr) {
            throw std::runtime_error("lowering operation invents an undeclared result resource");
        }
    }
}

bool manifest_verifies_aggregates(const std::string& report, const std::string& manifest) {
    const Json report_root = JsonParser{report}.parse();
    const Json manifest_root = JsonParser{manifest}.parse();
    if (json_text(json_field(manifest_root, "format")) != "flowcore.abi_manifest") throw std::runtime_error("unsupported ABI manifest format");
    if (json_text(json_field(manifest_root, "provider")) != "flowmini_testabi") throw std::runtime_error("unsupported ABI manifest provider");
    if (json_integer(json_field(manifest_root, "version"), "version") != 1) throw std::runtime_error("unsupported ABI manifest version");

    const Json* point_layout = nullptr;
    for (const auto& layout : json_array(json_field(report_root, "aggregate_abi_layouts"), "aggregate_abi_layouts")) {
        if (json_text(json_field(layout, "name")) == "Point") {
            if (point_layout != nullptr) throw std::runtime_error("semantic report contains duplicate Point layouts");
            point_layout = &layout;
        }
    }
    if (point_layout == nullptr) return false;

    const auto& types = json_array(json_field(manifest_root, "types"), "types");
    if (types.size() != 1 || json_text(json_field(types.front(), "name")) != "Point") throw std::runtime_error("ABI manifest must contain exactly one Point layout");
    const Json& point = types.front();
    if (json_integer(json_field(point, "size"), "size") != static_cast<long long>(2 * sizeof(int)) ||
        json_integer(json_field(point, "alignment"), "alignment") != static_cast<long long>(alignof(int))) {
        throw std::runtime_error("ABI manifest Point size/alignment does not match the provider ABI");
    }

    const auto& manifest_fields = json_array(json_field(point, "fields"), "fields");
    if (manifest_fields.size() != 2) throw std::runtime_error("ABI manifest Point must contain exactly two fields");
    const std::vector<std::tuple<std::string, std::string, long long>> expected = {
        {"x", "c_int", 0}, {"y", "c_int", static_cast<long long>(sizeof(int))}
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& field = manifest_fields[index];
        if (json_text(json_field(field, "name")) != std::get<0>(expected[index]) ||
            json_text(json_field(field, "type")) != std::get<1>(expected[index]) ||
            json_integer(json_field(field, "offset"), "offset") != std::get<2>(expected[index])) {
            throw std::runtime_error("ABI manifest Point fields do not match ordered provider layout");
        }
    }

    const auto& semantic_fields = json_array(json_field(*point_layout, "fields"), "fields");
    if (semantic_fields.size() != manifest_fields.size()) throw std::runtime_error("ABI manifest field count does not match semantic aggregate layout");
    for (std::size_t index = 0; index < semantic_fields.size(); ++index) {
        if (json_text(json_field(semantic_fields[index], "name")) != json_text(json_field(manifest_fields[index], "name")) ||
            json_text(json_field(semantic_fields[index], "type")) != json_text(json_field(manifest_fields[index], "type"))) {
            throw std::runtime_error("ABI manifest fields do not match semantic aggregate layout");
        }
    }
    return true;
}

int verify(const std::string& report, const std::string& policy_path, const std::string& abi_manifest_path) {
    if (report.find("\"format\": \"flowanalyst.semantic_report\"") == std::string::npos && report.find("\"format\":\"flowanalyst.semantic_report\"") == std::string::npos) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (report.find("\"version\": 1") == std::string::npos && report.find("\"version\":1") == std::string::npos) throw std::runtime_error("unsupported Flowanalyst report version");
    if (report.find("\"status\": \"ok\"") == std::string::npos && report.find("\"status\":\"ok\"") == std::string::npos) {
        std::cout << "{\n  \"format\": \"flowbind.binding_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not ready\"\n}\n";
        return 2;
    }
    const auto needed = requirements(report);
    validate_lowering_plan(report, needed);
    const bool aggregate_manifest_verified = !abi_manifest_path.empty() && manifest_verifies_aggregates(report, read_path(abi_manifest_path, "ABI manifest"));
    const auto profile = value(report, "lowering_profile");
    const auto grants = read_policy(policy_path);
    const std::vector<std::string> supported_types = {"c_int", "c_long", "c_ulong", "c_size_t", "c_string", "c_pointer"};
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
    std::size_t generic_operation_count = 0;
    try {
        const Json semantic_root = JsonParser{report}.parse();
        if (const auto* plan = json_field(semantic_root, "lowering_plan")) {
            generic_operation_count = json_array(json_field(*plan, "operations"), "lowering_plan.operations").size();
        }
    } catch (const std::exception&) {
        generic_operation_count = 0;
    }
    std::cout << "],\n  \"lowering_plan\": {\"kind\": \"generic\""
              << ",\"contract\":\"flowcore.lowering_plan\",\"operation_count\":" << generic_operation_count;
    std::cout << "},\n  \"policy\": {\"status\": \"authorized\", \"grants\": " << grants.size() << "},\n  \"abi\": {\"convention\": \"c\", \"carrier_types_supported\": true, \"provider_signature_evidence\": \"not-provided\", \"sizeof_int\": " << sizeof(int) << ", \"sizeof_long\": " << sizeof(long) << ", \"sizeof_size_t\": " << sizeof(std::size_t) << ", \"sizeof_pointer\": " << sizeof(void*) << "},\n  \"execution\": \"not-performed\"\n}\n";
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
