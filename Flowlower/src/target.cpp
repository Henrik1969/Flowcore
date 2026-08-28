#include <flowcontracts/validate.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace flowcontracts;

std::string read(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("target policy is unavailable: " + path.string());
    std::ostringstream input; input << file.rdbuf(); return input.str();
}

bool valid_name(const std::string& name) {
    if (name.empty() || name.front() == '.' || name.back() == '.') return false;
    for (const unsigned char character : name)
        if (!(std::isalnum(character) || character == '-' || character == '_' || character == '.')) return false;
    return true;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--version") { std::cout << "0.1.0\n"; return 0; }
        if (argc != 4 || std::string(argv[1]) != "--policy-root")
            throw std::runtime_error("usage: flowtarget --policy-root DIRECTORY TARGET-NAME");
        const std::string name = argv[3];
        if (!valid_name(name)) throw std::runtime_error("invalid target policy name");
        const auto value = json::parse(read(std::filesystem::path(argv[2]) / (name + ".json")));
        validate_target_policy(value);
        const auto& root = json::object(value);
        if (json::string(json::required(root, "name"), "$.name") != name)
            throw json::Error("$.name", "resolved policy identity does not match requested target name");
        std::cout << json::serialize(value) << '\n';
        return 0;
    } catch (const flowcontracts::json::Error& error) {
        std::cerr << "flowtarget contract error: " << error.what() << '\n'; return 1;
    } catch (const std::exception& error) {
        std::cerr << "flowtarget error: " << error.what() << '\n'; return 1;
    }
}
