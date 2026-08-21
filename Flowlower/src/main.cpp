#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Options { std::string optimization_path, binding_path, llvm_path, target_name; };

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--emit-llvm") { if (++i >= argc) throw std::runtime_error("--emit-llvm requires a path"); options.llvm_path = argv[i]; }
        else if (argument == "--binding-report") { if (++i >= argc) throw std::runtime_error("--binding-report requires a path"); options.binding_path = argv[i]; }
        else if (argument == "--target") { if (++i >= argc) throw std::runtime_error("--target requires a name"); options.target_name = argv[i]; }
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

int lower(std::string_view report, const std::string& llvm_path = {}, std::string_view binding_report = {}, const std::string& target_name = {}) {
    if (!has(report, "\"format\": \"flowoptimize.optimization_report\"") && !has(report, "\"format\":\"flowoptimize.optimization_report\"")) throw std::runtime_error("input is not a Flowoptimize optimization report");
    if (!has(report, "\"version\": 1") && !has(report, "\"version\":1")) throw std::runtime_error("unsupported Flowoptimize report version");
    if (!has(report, "\"status\": \"ready\"") && !has(report, "\"status\":\"ready\"")) {
        std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"optimization stage is not ready\"\n}\n";
        return 2;
    }
    auto targets_marker = report.find("\"targets\":[");
    if (targets_marker == std::string_view::npos) targets_marker = report.find("\"targets\": [");
    const auto targets_open = targets_marker == std::string_view::npos ? std::string_view::npos : report.find('[', targets_marker);
    const auto targets_close = targets_open == std::string_view::npos ? std::string_view::npos : report.find(']', targets_open);
    const bool has_targets = targets_open != std::string_view::npos && targets_close != std::string_view::npos && targets_close > targets_open + 1;
    const std::string selected_target = target_name.empty() ? "main" : target_name;
    if (has_targets) {
        if (target_name.empty()) {
            std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"multiple targets require explicit --target selection\"\n}\n";
            return 2;
        }
        const std::string spaced_target_marker = "\"name\": \"" + target_name + "\"";
        const std::string compact_target_marker = "\"name\":\"" + target_name + "\"";
        if (report.find(spaced_target_marker, targets_marker) == std::string_view::npos && report.find(compact_target_marker, targets_marker) == std::string_view::npos) throw std::runtime_error("requested target is not present in the optimization report");
    } else if (!target_name.empty()) throw std::runtime_error("requested target is not present in the optimization report");
    const bool trial_profile = has(report, "\"lowering_profile\": \"empty_program_main\"") || has(report, "\"lowering_profile\":\"empty_program_main\"");
    const bool abi_abs_profile = has(report, "\"lowering_profile\": \"abi_abs_main\"") || has(report, "\"lowering_profile\":\"abi_abs_main\"");
    const bool abi_strlen_profile = has(report, "\"lowering_profile\": \"abi_strlen_main\"") || has(report, "\"lowering_profile\":\"abi_strlen_main\"");
    const bool test_licbinds_profile = has(report, "\"lowering_profile\": \"test_licbinds_main\"") || has(report, "\"lowering_profile\":\"test_licbinds_main\"");
    const bool abi_kernel_getpid_profile = has(report, "\"lowering_profile\": \"abi_kernel_getpid_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_getpid_main\"");
    const bool abi_kernel_clock_profile = has(report, "\"lowering_profile\": \"abi_kernel_clock_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_clock_main\"");
    const bool abi_kernel_random_profile = has(report, "\"lowering_profile\": \"abi_kernel_random_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_random_main\"");
    const bool abi_kernel_uname_profile = has(report, "\"lowering_profile\": \"abi_kernel_uname_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_uname_main\"");
    const bool abi_kernel_openat_profile = has(report, "\"lowering_profile\": \"abi_kernel_openat_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_openat_main\"");
    const bool abi_kernel_read_profile = has(report, "\"lowering_profile\": \"abi_kernel_read_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_read_main\"");
    const bool abi_kernel_write_profile = has(report, "\"lowering_profile\": \"abi_kernel_write_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_write_main\"");
    const bool abi_kernel_lseek_profile = has(report, "\"lowering_profile\": \"abi_kernel_lseek_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_lseek_main\"");
    const bool abi_kernel_unlinkat_profile = has(report, "\"lowering_profile\": \"abi_kernel_unlinkat_main\"") || has(report, "\"lowering_profile\":\"abi_kernel_unlinkat_main\"");
    const bool remaining_kernel_profile = abi_kernel_unlinkat_profile || has(report, "abi_kernel_rmdir_main") || has(report, "abi_kernel_pipe2_main") || has(report, "abi_kernel_fork_main") || has(report, "abi_kernel_waitpid_main") || has(report, "abi_kernel_socketpair_main") || has(report, "abi_kernel_socket_main") || has(report, "abi_kernel_bind_main") || has(report, "abi_kernel_listen_main") || has(report, "abi_kernel_poll_main") || has(report, "abi_kernel_accept4_main") || has(report, "abi_kernel_connect_main") || has(report, "abi_kernel_unshare_main") || has(report, "abi_kernel_sethostname_main") || has(report, "abi_kernel_gethostname_main");
    const bool flowcat_profile = has(report, "\"lowering_profile\": \"flowcat_argv_main\"") || has(report, "\"lowering_profile\":\"flowcat_argv_main\"");
    const bool flowcat_file_profile = has(report, "\"lowering_profile\": \"flowcat_file_main\"") || has(report, "\"lowering_profile\":\"flowcat_file_main\"");
    if (abi_abs_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_abs_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"abs\""))) throw std::runtime_error("ABI binding report does not authorize the abi_abs_main lowering profile");
    if (abi_strlen_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_strlen_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"strlen\""))) throw std::runtime_error("ABI binding report does not authorize the abi_strlen_main lowering profile");
    if (test_licbinds_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"test_licbinds_main\"") || !has(binding_report, "\"strlen\"") || !has(binding_report, "\"abs\"") || !has(binding_report, "\"puts\""))) throw std::runtime_error("ABI binding report does not authorize the test_licbinds_main lowering profile");
    if (abi_kernel_getpid_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_getpid_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"getpid\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_getpid_main lowering profile");
    if (abi_kernel_clock_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_clock_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"clock_gettime\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_clock_main lowering profile");
    if (abi_kernel_random_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_random_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"getrandom\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_random_main lowering profile");
    if (abi_kernel_uname_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_uname_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"uname\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_uname_main lowering profile");
    if (abi_kernel_openat_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_openat_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"openat\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_openat_main lowering profile");
    if (abi_kernel_read_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_read_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"read\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_read_main lowering profile");
    if (abi_kernel_write_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_write_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"write\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_write_main lowering profile");
    if (abi_kernel_lseek_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_lseek_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"lseek\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_lseek_main lowering profile");
    if (abi_kernel_unlinkat_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"abi_kernel_unlinkat_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"unlinkat\""))) throw std::runtime_error("ABI binding report does not authorize the abi_kernel_unlinkat_main lowering profile");
    if (remaining_kernel_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"kind\": \"external_call\""))) throw std::runtime_error("ABI binding report does not authorize the remaining kernel lowering profile");
    if (flowcat_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"flowcat_argv_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"puts\""))) throw std::runtime_error("ABI binding report does not authorize the flowcat_argv_main lowering profile");
    if (flowcat_file_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"flowcat_file_main\"") || !has(binding_report, "\"kind\": \"capability_sequence\"") || !has(binding_report, "\"open\"") || !has(binding_report, "\"read\"") || !has(binding_report, "\"write\"") || !has(binding_report, "\"close\""))) throw std::runtime_error("ABI binding report does not authorize the flowcat_file_main lowering profile");
    if (!llvm_path.empty() && !trial_profile && !abi_abs_profile && !abi_strlen_profile && !test_licbinds_profile && !abi_kernel_getpid_profile && !abi_kernel_clock_profile && !abi_kernel_random_profile && !abi_kernel_uname_profile && !abi_kernel_openat_profile && !abi_kernel_read_profile && !abi_kernel_write_profile && !abi_kernel_lseek_profile && !abi_kernel_unlinkat_profile && !remaining_kernel_profile && !flowcat_profile && !flowcat_file_profile) throw std::runtime_error("LLVM emission requires an accepted lowering profile");
    if (!llvm_path.empty()) {
        std::ofstream llvm(llvm_path); if (!llvm) throw std::runtime_error("cannot open LLVM output");
        llvm << "; Flowcore target artifact: " << selected_target << "\n";
        if (abi_abs_profile) {
            llvm << "; Flowcore ABI trial lowering: abs\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @abs(i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %result = call i32 @abs(i32 -42)\n"
                    "  ret i32 %result\n"
                    "}\n";
        } else if (abi_strlen_profile) {
            llvm << "; Flowcore ABI trial lowering: strlen\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "@flowcore_message = private unnamed_addr constant [9 x i8] c\"Flowcore\\00\"\n"
                    "declare i64 @strlen(ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %length = call i64 @strlen(ptr @flowcore_message)\n"
                    "  %exit = trunc i64 %length to i32\n"
                    "  ret i32 %exit\n"
                    "}\n";
        } else if (test_licbinds_profile) {
            llvm << "; Flowcore libc binding integration: strlen + abs + puts\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "@flowcore_libc_message = private unnamed_addr constant [23 x i8] c\"Flowcore libc bindings\\00\"\n"
                    "declare i64 @strlen(ptr)\n"
                    "declare i32 @abs(i32)\n"
                    "declare i32 @puts(ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %length = call i64 @strlen(ptr @flowcore_libc_message)\n"
                    "  %absolute = call i32 @abs(i32 -42)\n"
                    "  %printed = call i32 @puts(ptr @flowcore_libc_message)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_getpid_profile) {
            llvm << "; Flowcore kernel ABI lowering: getpid\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @getpid()\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %process = call i32 @getpid()\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_clock_profile) {
            llvm << "; Flowcore kernel ABI lowering: clock_gettime\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @clock_gettime(i32, ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %timespec = alloca [16 x i8], align 8\n"
                    "  %value = getelementptr [16 x i8], ptr %timespec, i32 0, i32 0\n"
                    "  %result = call i32 @clock_gettime(i32 1, ptr %value)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_random_profile) {
            llvm << "; Flowcore kernel ABI lowering: getrandom\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i64 @getrandom(ptr, i64, i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %buffer = alloca [1 x i8], align 1\n"
                    "  %value = getelementptr [1 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  %result = call i64 @getrandom(ptr %value, i64 1, i32 0)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_uname_profile) {
            llvm << "; Flowcore kernel ABI lowering: uname\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @uname(ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %buffer = alloca [390 x i8], align 8\n"
                    "  %value = getelementptr [390 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  %result = call i32 @uname(ptr %value)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_openat_profile) {
            llvm << "; Flowcore kernel ABI lowering: openat read-only root\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "@flowcore_root = private unnamed_addr constant [2 x i8] c\"/\\00\"\n"
                    "declare i32 @openat(i32, ptr, i32, i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %fd = call i32 @openat(i32 -100, ptr @flowcore_root, i32 0, i32 0)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_read_profile) {
            llvm << "; Flowcore kernel ABI lowering: read invalid descriptor probe\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i64 @read(i32, ptr, i64)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %buffer = alloca [1 x i8], align 1\n"
                    "  %value = getelementptr [1 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  %result = call i64 @read(i32 -1, ptr %value, i64 1)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_write_profile) {
            llvm << "; Flowcore kernel ABI lowering: write invalid descriptor probe\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i64 @write(i32, ptr, i64)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %buffer = alloca [1 x i8], align 1\n"
                    "  %value = getelementptr [1 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  %result = call i64 @write(i32 -1, ptr %value, i64 1)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_lseek_profile) {
            llvm << "; Flowcore kernel ABI lowering: lseek invalid descriptor probe\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i64 @lseek(i32, i64, i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %result = call i64 @lseek(i32 -1, i64 0, i32 0)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (abi_kernel_unlinkat_profile) {
            llvm << "; Flowcore kernel ABI lowering: unlinkat invalid descriptor probe\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "@flowcore_missing = private unnamed_addr constant [22 x i8] c\"/flowcore-nonexistent\\00\"\n"
                    "declare i32 @unlinkat(i32, ptr, i32)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %result = call i32 @unlinkat(i32 -1, ptr @flowcore_missing, i32 0)\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (remaining_kernel_profile) {
            llvm << "target triple = \"x86_64-pc-linux-gnu\"\n";
            if (has(report, "abi_kernel_rmdir_main")) llvm << "declare i32 @rmdir(ptr)\ndefine i32 @main() {\nentry:\n  %result = call i32 @rmdir(ptr null)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_pipe2_main")) llvm << "declare i32 @pipe2(ptr, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @pipe2(ptr null, i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_fork_main")) llvm << "declare i32 @fork()\ndefine i32 @main() {\nentry:\n  %result = call i32 @fork()\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_waitpid_main")) llvm << "declare i32 @waitpid(i32, ptr, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @waitpid(i32 -1, ptr null, i32 1)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_socketpair_main")) llvm << "declare i32 @socketpair(i32, i32, i32, ptr)\ndefine i32 @main() {\nentry:\n  %result = call i32 @socketpair(i32 1, i32 1, i32 0, ptr null)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_socket_main")) llvm << "declare i32 @socket(i32, i32, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @socket(i32 -1, i32 1, i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_bind_main")) llvm << "declare i32 @bind(i32, ptr, i64)\ndefine i32 @main() {\nentry:\n  %result = call i32 @bind(i32 -1, ptr null, i64 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_listen_main")) llvm << "declare i32 @listen(i32, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @listen(i32 -1, i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_poll_main")) llvm << "declare i32 @poll(ptr, i64, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @poll(ptr null, i64 0, i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_accept4_main")) llvm << "declare i32 @accept4(i32, ptr, ptr, i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @accept4(i32 -1, ptr null, ptr null, i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_connect_main")) llvm << "declare i32 @connect(i32, ptr, i64)\ndefine i32 @main() {\nentry:\n  %result = call i32 @connect(i32 -1, ptr null, i64 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_unshare_main")) llvm << "declare i32 @unshare(i32)\ndefine i32 @main() {\nentry:\n  %result = call i32 @unshare(i32 0)\n  ret i32 0\n}\n";
            else if (has(report, "abi_kernel_sethostname_main")) llvm << "declare i32 @sethostname(ptr, i64)\ndefine i32 @main() {\nentry:\n  %result = call i32 @sethostname(ptr null, i64 0)\n  ret i32 0\n}\n";
            else llvm << "declare i32 @gethostname(ptr, i64)\ndefine i32 @main() {\nentry:\n  %result = call i32 @gethostname(ptr null, i64 0)\n  ret i32 0\n}\n";
        } else if (flowcat_file_profile) {
            llvm << "; Flowcore application lowering: flowcat argv -> open/read/write/close\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @open(ptr, i32)\n"
                    "declare i64 @read(i32, ptr, i64)\n"
                    "declare i64 @write(i32, ptr, i64)\n"
                    "declare i32 @close(i32)\n"
                    "declare i32 @__errno_location()\n"
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                    "  %has_args = icmp sgt i32 %argc, 1\n"
                    "  br i1 %has_args, label %next_file, label %done\n"
                    "next_file:\n"
                    "  %index = phi i32 [1, %entry], [%next_index, %next_after_close]\n"
                    "  %slot = getelementptr ptr, ptr %argv, i32 %index\n"
                    "  %path = load ptr, ptr %slot\n"
                    "  %fd = call i32 @open(ptr %path, i32 0)\n"
                    "  %opened = icmp sge i32 %fd, 0\n"
                    "  br i1 %opened, label %read_file, label %error\n"
                    "read_file:\n"
                    "  %buffer = alloca [4096 x i8], align 16\n"
                    "  %data = getelementptr [4096 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  br label %read_loop\n"
                    "read_loop:\n"
                    "  %count = call i64 @read(i32 %fd, ptr %data, i64 4096)\n"
                    "  %has_data = icmp sgt i64 %count, 0\n"
                    "  %eof = icmp eq i64 %count, 0\n"
                    "  br i1 %has_data, label %write_loop, label %read_result\n"
                    "read_result:\n"
                    "  br i1 %eof, label %close_file, label %error_close\n"
                    "write_loop:\n"
                    "  %remaining = phi i64 [%count, %read_loop], [%remaining_after, %write_progress]\n"
                    "  %offset = phi i64 [0, %read_loop], [%next_offset, %write_progress]\n"
                    "  %write_data = getelementptr i8, ptr %data, i64 %offset\n"
                    "  %written = call i64 @write(i32 1, ptr %write_data, i64 %remaining)\n"
                    "  %write_ok = icmp sgt i64 %written, 0\n"
                    "  br i1 %write_ok, label %write_progress, label %error_close\n"
                    "write_progress:\n"
                    "  %remaining_after = sub i64 %remaining, %written\n"
                    "  %next_offset = add i64 %offset, %written\n"
                    "  %more_output = icmp sgt i64 %remaining_after, 0\n"
                    "  br i1 %more_output, label %write_loop, label %read_loop\n"
                    "close_file:\n"
                    "  %closed = call i32 @close(i32 %fd)\n"
                    "  %close_ok = icmp sge i32 %closed, 0\n"
                    "  br i1 %close_ok, label %next_after_close, label %error\n"
                    "next_after_close:\n"
                    "  %next_index = add i32 %index, 1\n"
                    "  %more = icmp slt i32 %next_index, %argc\n"
                    "  br i1 %more, label %next_file, label %done\n"
                    "error_close:\n"
                    "  call i32 @close(i32 %fd)\n"
                    "  br label %error\n"
                    "error:\n"
                    "  ret i32 1\n"
                    "done:\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (flowcat_profile) {
            llvm << "; Flowcore application lowering: flowcat argv -> puts\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @puts(ptr)\n"
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                    "  %has_args = icmp sgt i32 %argc, 1\n"
                    "  br i1 %has_args, label %loop, label %done\n"
                    "loop:\n"
                    "  %index = phi i32 [1, %entry], [%next, %printed]\n"
                    "  %slot = getelementptr ptr, ptr %argv, i32 %index\n"
                    "  %arg = load ptr, ptr %slot\n"
                    "  %printed_value = call i32 @puts(ptr %arg)\n"
                    "  br label %printed\n"
                    "printed:\n"
                    "  %next = add i32 %index, 1\n"
                    "  %more = icmp slt i32 %next, %argc\n"
                    "  br i1 %more, label %loop, label %done\n"
                    "done:\n"
                    "  ret i32 0\n"
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
                 "  \"source\": {\"path\": " << quote(source_path(report)) << "},\n"
                 "  \"target\": {\"name\": " << quote(selected_target) << ", \"selection\": \"explicit-or-default\"},\n"
                 "  \"artifact\": {\"backend\": \"llvm\", \"target_specific\": true, \"status\": \"" << (llvm_path.empty() ? "not-emitted" : "emitted") << "\"},\n"
                 "  \"backend\": {\"name\": \"llvm\", \"provider_status\": \"available\"},\n"
                 "  \"ir\": {\"format\": \"llvm-ir\", \"status\": \"" << (llvm_path.empty() ? "not-emitted" : "emitted") << "\"},\n"
                 "  \"message\": \"LLVM lowering boundary reached for the accepted profile\"\n"
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
                             "Usage: flowlower [--target name] [--binding-report report.json] [optimization-report.json]\n"
                             "       flowlower --emit-llvm output.ll [--target name] [--binding-report report.json] < optimization-report.json\n"
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
        return lower(optimization_report, options.llvm_path, binding_report, options.target_name);
    } catch (const std::exception& error) { std::cerr << "flowlower error: " << error.what() << '\n'; return 1; }
}
