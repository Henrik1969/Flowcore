#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view VERSION = "0.1.0";

std::string read_input(int argc, char** argv) {
    if (argc > 2 && !(argc == 3 && std::string(argv[1]) == "--emit-llvm")) throw std::runtime_error("usage: flowlower [optimization-report.json] or flowlower --emit-llvm path");
    std::ostringstream input;
    if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open optimization report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has(std::string_view input, std::string_view fragment) { return input.find(fragment) != std::string_view::npos; }

int lower(std::string_view report, const std::string& llvm_path = {}) {
    if (!has(report, "\"format\": \"flowoptimize.optimization_report\"") && !has(report, "\"format\":\"flowoptimize.optimization_report\"")) throw std::runtime_error("input is not a Flowoptimize optimization report");
    if (!has(report, "\"version\": 1") && !has(report, "\"version\":1")) throw std::runtime_error("unsupported Flowoptimize report version");
    if (!has(report, "\"status\": \"ready\"") && !has(report, "\"status\":\"ready\"")) {
        std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"optimization stage is not ready\"\n}\n";
        return 2;
    }
    const bool trial_profile = has(report, "\"lowering_profile\": \"empty_program_main\"") || has(report, "\"lowering_profile\":\"empty_program_main\"");
    if (!llvm_path.empty() && !trial_profile) throw std::runtime_error("LLVM emission requires an accepted lowering profile");
    if (!llvm_path.empty()) {
        std::ofstream llvm(llvm_path); if (!llvm) throw std::runtime_error("cannot open LLVM output");
        llvm << "; Flowcore trial lowering: empty_program_main\n"
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
                             "Usage: flowlower [optimization-report.json]\n"
                             "       flowlower --emit-llvm output.ll < optimization-report.json\n"
                             "       flowmini ... | flowanalyst | flowoptimize | flowlower\n\n"
                             "Options: -h, -?, --help  show help\n"
                             "         -a, --about    show about information\n"
                             "         -v, --version  print the raw version number\n";
                return 0;
            }
            if (option == "-a" || option == "--about") { std::cout << "Flowlower projects optimized Flowcore state onto target backends.\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        const std::string llvm_path = argc == 3 ? argv[2] : std::string{};
        return lower(read_input(argc, argv), llvm_path);
    } catch (const std::exception& error) { std::cerr << "flowlower error: " << error.what() << '\n'; return 1; }
}
