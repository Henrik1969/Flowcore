#pragma once

#include "symboltable/Factoid.hpp"
#include "symboltable/IdTypes.hpp"
#include "symboltable/Scope.hpp"
#include "symboltable/Symbol.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace symboltable {

class SymbolTable final {
public:
    SymbolTable();

    [[nodiscard]] ScopeId globalScope() const noexcept;

    [[nodiscard]] ScopeId createScope(
        ScopeKind kind,
        std::optional<ScopeId> parent = std::nullopt,
        std::optional<SymbolId> ownerSymbol = std::nullopt,
        std::string debugName = {}
    );

    [[nodiscard]] SymbolId insertSymbol(
        ScopeId scope,
        std::string name,
        SymbolKind kind = SymbolKind::Unknown
    );

    void addFact(SymbolId symbol, Factoid fact);

    [[nodiscard]] std::vector<SymbolId> findLocal(
        ScopeId scope,
        std::string_view name
    ) const;

    [[nodiscard]] std::vector<SymbolId> resolveUpwards(
        ScopeId fromScope,
        std::string_view name
    ) const;

    [[nodiscard]] std::optional<SymbolId> findExact(
        ScopeId scope,
        std::string_view name,
        SymbolKind kind
    ) const;

    [[nodiscard]] const Symbol& symbol(SymbolId id) const;
    [[nodiscard]] Symbol& symbol(SymbolId id);

    [[nodiscard]] const Scope& scope(ScopeId id) const;
    [[nodiscard]] Scope& scope(ScopeId id);

    void dump(std::ostream& out) const;

private:
    [[nodiscard]] bool isValidSymbolId(SymbolId id) const noexcept;
    [[nodiscard]] bool isValidScopeId(ScopeId id) const noexcept;

    [[nodiscard]] SymbolId makeSymbolId(std::size_t index) const noexcept;
    [[nodiscard]] ScopeId makeScopeId(std::size_t index) const noexcept;

    [[nodiscard]] std::size_t symbolIndex(SymbolId id) const;
    [[nodiscard]] std::size_t scopeIndex(ScopeId id) const;

    ScopeId globalScope_ {};
    std::vector<Symbol> symbols_;
    std::vector<Scope> scopes_;

    std::unordered_map<ScopeId, std::unordered_map<std::string, std::vector<SymbolId>>> index_;
};

} // namespace symboltable
