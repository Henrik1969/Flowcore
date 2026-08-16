#pragma once

#include "symboltable/LookupDecision.hpp"
#include "symboltable/SymbolTable.hpp"
#include "symboltable/Visibility.hpp"

#include <string>

namespace symboltable {

class ISymbolPolicy {
public:
    virtual ~ISymbolPolicy() = default;

    [[nodiscard]] virtual LookupDecision inspect(
        const SymbolTable& table,
        SymbolId candidate,
        ScopeId viewpoint
    ) const = 0;
};

class ExposeAllPolicy final : public ISymbolPolicy {
public:
    [[nodiscard]] LookupDecision inspect(
        const SymbolTable& table,
        SymbolId candidate,
        ScopeId viewpoint
    ) const override;
};

class HidePrivatePolicy final : public ISymbolPolicy {
public:
    [[nodiscard]] LookupDecision inspect(
        const SymbolTable& table,
        SymbolId candidate,
        ScopeId viewpoint
    ) const override;
};

} // namespace symboltable
