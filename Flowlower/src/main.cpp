#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Options { std::string optimization_path, binding_path, llvm_path; };

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--emit-llvm") { if (++i >= argc) throw std::runtime_error("--emit-llvm requires a path"); options.llvm_path = argv[i]; }
        else if (argument == "--binding-report") { if (++i >= argc) throw std::runtime_error("--binding-report requires a path"); options.binding_path = argv[i]; }
        else if (!argument.empty() && argument.front() == '-') throw std::runtime_error("unknown option '" + argument + "'");
        else if (options.optimization_path.empty()) options.optimization_path = argument;
        else throw std::runtime_error("too many input paths");
    }
    return options;
}

std::string read_file_or_stdin(const std::string& path) {
    std::ostringstream input;
    if (!path.empty()) { std::ifstream file(path); if (!file) throw std::runtime_error("cannot open report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has(std::string_view input, std::string_view fragment) { return input.find(fragment) != std::string_view::npos; }

int lower(std::string_view report, const std::string& llvm_path = {}, std::string_view binding_report = {}) {
    if (!has(report, "\"format\": \"flowoptimize.optimization_report\"") && !has(report, "\"format\":\"flowoptimize.optimization_report\"")) throw std::runtime_error("input is not a Flowoptimize optimization report");
    if (!has(report, "\"version\": 1") && !has(report, "\"version\":1")) throw std::runtime_error("unsupported Flowoptimize report version");
    if (!has(report, "\"status\": \"ready\"") && !has(report, "\"status\":\"ready\"")) {
        std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"optimization stage is not ready\"\n}\n";
        return 2;
    }
    const bool trial_profile = has(report, "\"lowering_profile\": \"empty_program_main\"") || has(report, "\"lowering_profile\":\"empty_program_main\"");
    const bool abi_abs_profile = has(report, "\"lowering_profile\": \"abi_abs_main\"") || has(report, "\"lowering_profile\":\"abi_abs_main\"");
    if (abi_abs_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_abs_main\"") || !has(binding_report, "\"abs\""))) throw std::runtime_error("ABI binding report does not authorize the abi_abs_main lowering profile");
    if (!llvm_path.empty() && !trial_profile && !abi_abs_profile) throw std::runtime_error("LLVM emission requires an accepted lowering profile");
    if (!llvm_path.empty()) {
        std::ofstream llvm(llvm_path); if (!llvm) throw std::runtime_error("cannot open LLVM output");
        if (abi_abs_profile) {
            llvm << "; Flowcore ABI trial lowering: abs\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @abs(i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %result = call i32 @abs(i32 -42)\n"
                    "  ret i32 %result\n"
                    "}\n";
        } else llvm << "; Flowcore trial lowering: empty_program_main\n"
                "target triple = \"x86_64-pc-linux-gnu\"\n"
                "define i32 @main() {\n"
                "entry:\n"
                "  ret i32 0\n"
                "}\n";
    }
    std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n"
                 "  \"version\": 1,\n"
                 "  \"status\": \"ready\",\n"
                 "  \"backend\": {\"name\": \"llvm\", \"provider_status\": \"available\"},\n"
                 "  \"ir\": {\"format\": \"llvm-ir\", \"status\": \"" << (llvm_path.empty() ? "not-emitted" : "emitted") << "\"},\n"
                 "  \"message\": \"LLVM lowering boundary reached; source lowering is the next implementation\"\n"
                 "}\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") {
                std::cout << "flowlower - target lowering boundary\n\n"
                             "Usage: flowlower [--binding-report report.json] [optimization-report.json]\n"
                             "       flowlower --emit-llvm output.ll [--binding-report report.json] < optimization-report.json\n"
                             "       flowmini ... | flowanalyst | flowoptimize | flowlower\n\n"
                             "Options: -h, -?, --help  show help\n"
                             "         -a, --about    show about information\n"
                             "         -v, --version  print the raw version number\n";
                return 0;
            }
            if (option == "-a" || option == "--about") { std::cout << "Flowlower projects optimized Flowcore state onto target backends.\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        const auto options = parse_options(argc, argv);
        const auto optimization_report = read_file_or_stdin(options.optimization_path);
        const auto binding_report = options.binding_path.empty() ? std::string{} : read_file_or_stdin(options.binding_path);
        return lower(optimization_report, options.llvm_path, binding_report);
    } catch (const std::exception& error) { std::cerr << "flowlower error: " << error.what() << '\n'; return 1; }
}
