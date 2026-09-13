#pragma once

#include "symboltable/DefinitionState.hpp"
#include "symboltable/Factoid.hpp"
#include "symboltable/IdTypes.hpp"
#include "symboltable/SourceLocation.hpp"
#include "symboltable/SymbolKind.hpp"
#include "symboltable/Visibility.hpp"

#include <optional>
#include <string>
#include <vector>

namespace symboltable {

struct Symbol final {
    SymbolId id {};
    ScopeId owningScope {};

    std::string name;
    SymbolKind kind {SymbolKind::Unknown};
    Visibility visibility {Visibility::Unspecified};
    DefinitionState definitionState {DefinitionState::Unknown};

    std::optional<ScopeId> introducedScope;
    std::optional<SourceLocation> declarationLocation;
    std::optional<SourceLocation> definitionLocation;

    std::vector<Factoid> facts;
};

} // namespace symboltable
