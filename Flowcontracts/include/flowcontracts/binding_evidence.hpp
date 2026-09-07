#pragma once

#include <flowcontracts/json.hpp>

namespace flowcontracts {

// Optional only for handwritten compatibility declarations. Generated identities
// bind the exact specification bytes and provider bytes; they are not signatures.
inline std::string binding_evidence(const json::Object& object, const std::string& path) {
    const auto* field = json::optional(object, "evidence");
    if (!field) return {};
    const auto value = json::string(*field, path + ".evidence");
    if (value.empty()) return value;
    const std::string prefix = "flowcore.generated_binding.v1:";
    if (value.size() != prefix.size() + 129 || value.compare(0, prefix.size(), prefix) != 0 ||
        value[prefix.size() + 64] != ':')
        throw json::Error(path + ".evidence", "unsupported generated binding evidence identity");
    for (std::size_t i = prefix.size(); i < value.size(); ++i) {
        if (i == prefix.size() + 64) continue;
        if (!((value[i] >= '0' && value[i] <= '9') || (value[i] >= 'a' && value[i] <= 'f')))
            throw json::Error(path + ".evidence", "invalid evidence SHA-256 digest");
    }
    return value;
}

} // namespace flowcontracts
