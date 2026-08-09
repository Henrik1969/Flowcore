#pragma once

#include "symboltable/IdTypes.hpp"

#include <string>

namespace symboltable {

enum class LookupDecisionKind {
    Visible,
    Hidden,
    Rejected,
    Shadowed,
    Ambiguous,
    NotFound
};

struct LookupDecision final {
    SymbolId symbol {};
    LookupDecisionKind kind {LookupDecisionKind::NotFound};
    std::string reason;

    [[nodiscard]] constexpr bool isVisible() const noexcept {
        return kind == LookupDecisionKind::Visible;
    }
};

} // namespace symboltable
