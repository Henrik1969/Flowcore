#include "symboltable/SymbolPolicy.hpp"

#include <string>
#include <utility>

namespace symboltable {

namespace {

[[nodiscard]] bool isSameOrDescendantScope(
    const SymbolTable& table,
    const ScopeId possibleDescendant,
    const ScopeId possibleAncestor
) {
    auto current = possibleDescendant;
    while (true) {
        if (current == possibleAncestor) {
            return true;
        }

        const auto& scope = table.scope(current);
        if (!scope.parent.has_value()) {
            return false;
        }
        current = *scope.parent;
    }
}

} // namespace

LookupDecision ExposeAllPolicy::inspect(
    const SymbolTable& table,
    const SymbolId candidate,
    const ScopeId viewpoint
) const {
    [[maybe_unused]] const auto& sym = table.symbol(candidate);
    [[maybe_unused]] const auto& viewScope = table.scope(viewpoint);

    return LookupDecision{
        .symbol = candidate,
        .kind = LookupDecisionKind::Visible,
        .reason = "exposed by ExposeAllPolicy"
    };
}

LookupDecision HidePrivatePolicy::inspect(
    const SymbolTable& table,
    const SymbolId candidate,
    const ScopeId viewpoint
) const {
    const auto& sym = table.symbol(candidate);

    if (sym.visibility != Visibility::Private) {
        return LookupDecision{
            .symbol = candidate,
            .kind = LookupDecisionKind::Visible,
            .reason = "not private"
        };
    }

    // Minimal demonstration rule: private facts are visible from inside the
    // same owning scope or any nested scope below it. This is deliberately a
    // sample policy, not a rule baked into SymbolTable.
    if (isSameOrDescendantScope(table, viewpoint, sym.owningScope)) {
        return LookupDecision{
            .symbol = candidate,
            .kind = LookupDecisionKind::Visible,
            .reason = "private symbol viewed from owning scope or descendant scope"
        };
    }

    return LookupDecision{
        .symbol = candidate,
        .kind = LookupDecisionKind::Hidden,
        .reason = "private symbol hidden from this viewpoint"
    };
}

} // namespace symboltable
