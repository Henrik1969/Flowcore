#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr const char* native_ls = "/usr/bin/ls";

bool is_control_option(const std::string& argument, const char* name) {
    return argument == name || argument.rfind(std::string(name) + "=", 0) == 0;
}

int unresolved_extension(const std::string& policy, const std::string& schema) {
    std::fprintf(stderr,
                 "franken-ls: policy/schema extension is unresolved: policy='%s' schema='%s'\n",
                 policy.c_str(), schema.c_str());
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    std::string policy;
    std::string schema;
    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (is_control_option(argument, "--Use_call_Policy")) {
            const auto equals = argument.find('=');
            if (equals == std::string::npos || equals + 1 == argument.size()) {
                if (index + 1 >= argc) {
                    std::fputs("franken-ls: --Use_call_Policy requires a value\n", stderr);
                    return 2;
                }
                policy = argv[++index];
            } else {
                policy = argument.substr(equals + 1);
            }
        } else if (is_control_option(argument, "--Use_call_Schema")) {
            const auto equals = argument.find('=');
            if (equals == std::string::npos || equals + 1 == argument.size()) {
                if (index + 1 >= argc) {
                    std::fputs("franken-ls: --Use_call_Schema requires a value\n", stderr);
                    return 2;
                }
                schema = argv[++index];
            } else {
                schema = argument.substr(equals + 1);
            }
        }
    }
    if (!policy.empty() || !schema.empty()) {
        return unresolved_extension(policy, schema);
    }

    std::vector<char*> native_arguments;
    native_arguments.reserve(static_cast<std::size_t>(argc) + 1);
    native_arguments.push_back(const_cast<char*>(native_ls));
    for (int index = 1; index < argc; ++index) native_arguments.push_back(argv[index]);
    native_arguments.push_back(nullptr);
    execv(native_ls, native_arguments.data());
    std::fprintf(stderr, "franken-ls: unable to execute %s: %s\n",
                 native_ls, std::strerror(errno));
    return errno == ENOENT ? 127 : 126;
}
