#pragma once

#include "symboltable/IdTypes.hpp"
#include "symboltable/ScopeKind.hpp"

#include <optional>
#include <string>
#include <vector>

namespace symboltable {

struct Scope final {
    ScopeId id {};
    std::optional<ScopeId> parent;
    ScopeKind kind {ScopeKind::Anonymous};
    std::optional<SymbolId> ownerSymbol;
    std::string debugName;
    std::vector<ScopeId> children;
    std::vector<SymbolId> symbols;
};

} // namespace symboltable
