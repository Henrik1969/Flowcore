#pragma once

#include "symboltable/LookupDecision.hpp"
#include "symboltable/SymbolPolicy.hpp"
#include "symboltable/SymbolTable.hpp"

#include <string_view>
#include <vector>

namespace symboltable {

class SymbolTreeView final {
public:
    SymbolTreeView(
        const SymbolTable& table,
        const ISymbolPolicy& policy,
        ScopeId viewpoint
    );

    [[nodiscard]] ScopeId viewpoint() const noexcept;

    [[nodiscard]] std::vector<SymbolId> resolveVisible(std::string_view name) const;

    [[nodiscard]] std::vector<LookupDecision> inspectResolution(std::string_view name) const;

    [[nodiscard]] std::vector<SymbolId> visibleSymbolsIn(ScopeId scope) const;

    [[nodiscard]] std::vector<LookupDecision> inspectSymbolsIn(ScopeId scope) const;

private:
    const SymbolTable& table_;
    const ISymbolPolicy& policy_;
    ScopeId viewpoint_ {};
};

} // namespace symboltable
