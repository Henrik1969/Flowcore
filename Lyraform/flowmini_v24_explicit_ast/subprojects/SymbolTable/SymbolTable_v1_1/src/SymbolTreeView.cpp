#include "symboltable/SymbolTreeView.hpp"

#include <stdexcept>
#include <utility>

namespace symboltable {

SymbolTreeView::SymbolTreeView(
    const SymbolTable& table,
    const ISymbolPolicy& policy,
    const ScopeId viewpoint
)
    : table_{table}, policy_{policy}, viewpoint_{viewpoint}
{
    [[maybe_unused]] const auto& checked = table_.scope(viewpoint_);
}

ScopeId SymbolTreeView::viewpoint() const noexcept
{
    return viewpoint_;
}

std::vector<SymbolId> SymbolTreeView::resolveVisible(const std::string_view name) const
{
    std::vector<SymbolId> visible;

    for (const auto id : table_.resolveUpwards(viewpoint_, name)) {
        const auto decision = policy_.inspect(table_, id, viewpoint_);
        if (decision.isVisible()) {
            visible.push_back(id);
        }
    }

    return visible;
}

std::vector<LookupDecision> SymbolTreeView::inspectResolution(const std::string_view name) const
{
    std::vector<LookupDecision> decisions;

    for (const auto id : table_.resolveUpwards(viewpoint_, name)) {
        decisions.push_back(policy_.inspect(table_, id, viewpoint_));
    }

    return decisions;
}

std::vector<SymbolId> SymbolTreeView::visibleSymbolsIn(const ScopeId scopeId) const
{
    std::vector<SymbolId> visible;

    for (const auto id : table_.scope(scopeId).symbols) {
        const auto decision = policy_.inspect(table_, id, viewpoint_);
        if (decision.isVisible()) {
            visible.push_back(id);
        }
    }

    return visible;
}

std::vector<LookupDecision> SymbolTreeView::inspectSymbolsIn(const ScopeId scopeId) const
{
    std::vector<LookupDecision> decisions;

    for (const auto id : table_.scope(scopeId).symbols) {
        decisions.push_back(policy_.inspect(table_, id, viewpoint_));
    }

    return decisions;
}

} // namespace symboltable
