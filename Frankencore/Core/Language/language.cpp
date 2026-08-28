#include "frankencore/language.hpp"

namespace frankencore::language {

Resolution resolve_moniker(const contracts::LanguageMap& map,
                           const std::string& input) {
    Resolution result;
    result.input = input;
    const auto validation = contracts::validate(map);
    if (!validation.valid) {
        result.diagnostic = validation.error;
        return result;
    }
    if (input.empty()) {
        result.diagnostic = "empty moniker";
        return result;
    }

    for (const auto& [identity, aliases] : map.monikers) {
        if (identity == input) {
            result.resolved = true;
            result.canonical = identity;
            return result;
        }
        for (const auto& alias : aliases) {
            if (alias != input) continue;
            if (!result.canonical.empty()) {
                result.diagnostic = "moniker resolves to multiple canonical identities";
                result.canonical.clear();
                return result;
            }
            result.canonical = identity;
        }
    }
    if (result.canonical.empty()) {
        result.diagnostic = "moniker is not declared by language map";
        return result;
    }
    result.resolved = true;
    return result;
}

} // namespace frankencore::language
