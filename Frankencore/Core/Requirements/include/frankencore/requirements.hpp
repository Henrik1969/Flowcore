#pragma once

#include <string>

namespace frankencore::requirements {

struct VersionResult {
    bool valid = false;
    std::string diagnostic;
};

struct MatchResult {
    bool valid = false;
    bool matches = false;
    std::string diagnostic;
};

// Deliberately small initial grammar: dotted non-negative integers compared
// by whitespace-separated operators: >= <= > < =. Unknown syntax is rejected.
VersionResult validate_version(const std::string& version);
MatchResult satisfies(const std::string& version, const std::string& expression);

} // namespace frankencore::requirements
