#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view version = "0.1.0";

struct Options {
    std::string plan_path;
    std::string capabilities_path;
    std::string calibration_path;
    double minimum_speedup = 1.25;
};

std::string read_file(const std::string& path, const char* label) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error(std::string("cannot open ") + label);
    std::ostringstream input;
    input << file.rdbuf();
    return input.str();
}

bool has_field(std::string_view input, std::string_view field, std::string_view value) {
    const std::string spaced = "\"" + std::string(field) + "\": \"" + std::string(value) + "\"";
    const std::string compact = "\"" + std::string(field) + "\":\"" + std::string(value) + "\"";
    return input.find(spaced) != std::string_view::npos || input.find(compact) != std::string_view::npos;
}

bool has_text(std::string_view input, std::string_view text) { return input.find(text) != std::string_view::npos; }

double number_after(std::string_view input, std::string_view marker) {
    const auto start = input.find(marker);
    if (start == std::string_view::npos) return 0.0;
    auto position = start + marker.size();
    while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    try { return std::stod(std::string(input.substr(position))); }
    catch (...) { return 0.0; }
}

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (char character : value) {
        if (character == '\\' || character == '"') result.push_back('\\');
        result.push_back(character);
    }
    result.push_back('"');
    return result;
}

Options parse(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto value = [&](const char* name) -> std::string {
            if (++index >= argc) throw std::runtime_error(std::string(name) + " requires a value");
            return argv[index];
        };
        if (argument == "--plan") options.plan_path = value("--plan");
        else if (argument == "--capabilities") options.capabilities_path = value("--capabilities");
        else if (argument == "--calibration") options.calibration_path = value("--calibration");
        else if (argument == "--min-speedup") options.minimum_speedup = std::stod(value("--min-speedup"));
        else if (argument == "-h" || argument == "-?" || argument == "--help") {
            std::cout << "flowparallel_runtime_planner - explainable CPU/CUDA provider decision\n\n"
                         "Options: --plan plan.json --capabilities capabilities.json\n"
                         "         [--calibration benchmark.json] [--min-speedup N]\n"
                         "         -h, -?, --help  show help\n"
                         "         -a, --about    show about information\n"
                         "         -v, --version  print the raw version number\n";
            std::exit(0);
        } else if (argument == "-a" || argument == "--about") {
            std::cout << "Flowparallel resolves a runtime CPU/CUDA choice from plan, facts, policy, and calibration.\n";
            std::exit(0);
        } else if (argument == "-v" || argument == "--version") {
            std::cout << version << '\n';
            std::exit(0);
        } else throw std::runtime_error("unknown option '" + argument + "'");
    }
    if (options.plan_path.empty() || options.capabilities_path.empty()) throw std::runtime_error("--plan and --capabilities are required");
    if (options.minimum_speedup <= 0.0) throw std::runtime_error("--min-speedup must be positive");
    return options;
}

int run(const Options& options) {
    const auto plan = read_file(options.plan_path, "execution plan");
    const auto capabilities = read_file(options.capabilities_path, "runtime capabilities");
    const auto calibration = options.calibration_path.empty() ? std::string{} : read_file(options.calibration_path, "calibration report");
    if (!has_field(plan, "format", "flowparallel.execution_plan")) throw std::runtime_error("input is not a Flowparallel execution plan");
    if (!has_field(plan, "status", "ready")) {
        std::cout << "{\n  \"format\": \"flowparallel.provider_decision\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"execution plan is not ready\"\n}\n";
        return 2;
    }
    if (!has_field(capabilities, "format", "frankencore.runtime_capabilities") ||
        (!has_text(capabilities, "\"version\": 1") && !has_text(capabilities, "\"version\":1")))
        throw std::runtime_error("unsupported runtime capability snapshot");

    const bool cuda_available = has_field(capabilities, "status", "available") && has_text(capabilities, "\"device_count\":") && number_after(capabilities, "\"device_count\":") >= 1.0;
    const bool calibration_verified = has_field(calibration, "status", "verified") && has_text(calibration, "\"end_to_end_speedup\":");
    const double measured_speedup = calibration_verified ? number_after(calibration, "\"end_to_end_speedup\":") : 0.0;
    const bool cost_beneficial = calibration_verified && measured_speedup >= options.minimum_speedup;
    const bool cuda_selected = cuda_available && cost_beneficial;
    std::string reason;
    if (cuda_selected) reason = "CUDA available and calibrated end-to-end benefit meets policy";
    else if (!cuda_available) reason = "CUDA is unavailable or reported no device";
    else if (!calibration_verified) reason = "CUDA availability exists but verified calibration is missing";
    else reason = "measured CUDA benefit is below the active minimum-speedup policy";

    std::cout << std::setprecision(10)
              << "{\n"
              << "  \"format\": \"flowparallel.provider_decision\",\n"
              << "  \"version\": 1,\n"
              << "  \"status\": \"selected\",\n"
              << "  \"plan\": {\"format\": \"flowparallel.execution_plan\", \"version\": 1},\n"
              << "  \"policy\": {\"minimum_speedup\": " << options.minimum_speedup << "},\n"
              << "  \"evidence\": {\"cuda_available\": " << (cuda_available ? "true" : "false")
              << ", \"calibration_verified\": " << (calibration_verified ? "true" : "false")
              << ", \"measured_end_to_end_speedup\": " << measured_speedup << "},\n"
              << "  \"selection\": {\"provider\": " << quote(cuda_selected ? "cuda.cublas" : "cpu.serial")
              << ", \"reason\": " << quote(reason) << "},\n"
              << "  \"fallback\": {\"provider\": \"cpu.serial\", \"required\": true}\n"
              << "}\n";
    return 0;
}
}

int main(int argc, char** argv) {
    try { return run(parse(argc, argv)); }
    catch (const std::exception& error) { std::cerr << "flowparallel_runtime_planner error: " << error.what() << '\n'; return 1; }
}
