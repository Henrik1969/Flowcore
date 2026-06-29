#include "symboltable/SymbolTable.hpp"

#include <stdexcept>
#include <utility>

namespace symboltable {

namespace {

[[nodiscard]] std::string_view toString(SymbolKind kind) noexcept {
    switch (kind) {
        case SymbolKind::Unknown: return "Unknown";
        case SymbolKind::Variable: return "Variable";
        case SymbolKind::Constant: return "Constant";
        case SymbolKind::Function: return "Function";
        case SymbolKind::Procedure: return "Procedure";
        case SymbolKind::Method: return "Method";
        case SymbolKind::Type: return "Type";
        case SymbolKind::Class: return "Class";
        case SymbolKind::Struct: return "Struct";
        case SymbolKind::Enum: return "Enum";
        case SymbolKind::EnumValue: return "EnumValue";
        case SymbolKind::Namespace: return "Namespace";
        case SymbolKind::Module: return "Module";
        case SymbolKind::Field: return "Field";
        case SymbolKind::Parameter: return "Parameter";
        case SymbolKind::Label: return "Label";
        case SymbolKind::Macro: return "Macro";
        case SymbolKind::Import: return "Import";
        case SymbolKind::Alias: return "Alias";
        case SymbolKind::Contract: return "Contract";
        case SymbolKind::Port: return "Port";
        case SymbolKind::Wire: return "Wire";
        case SymbolKind::Node: return "Node";
    }
    return "Unknown";
}

[[nodiscard]] std::string_view toString(ScopeKind kind) noexcept {
    switch (kind) {
        case ScopeKind::Global: return "Global";
        case ScopeKind::Module: return "Module";
        case ScopeKind::Namespace: return "Namespace";
        case ScopeKind::Type: return "Type";
        case ScopeKind::Function: return "Function";
        case ScopeKind::Block: return "Block";
        case ScopeKind::Class: return "Class";
        case ScopeKind::Struct: return "Struct";
        case ScopeKind::Enum: return "Enum";
        case ScopeKind::Contract: return "Contract";
        case ScopeKind::Node: return "Node";
        case ScopeKind::Anonymous: return "Anonymous";
    }
    return "Anonymous";
}

} // namespace

SymbolTable::SymbolTable()
{
    Scope global;
    global.id = ScopeId{1};
    global.kind = ScopeKind::Global;
    global.debugName = "global";
    scopes_.push_back(std::move(global));
    globalScope_ = ScopeId{1};
}

ScopeId SymbolTable::globalScope() const noexcept
{
    return globalScope_;
}

ScopeId SymbolTable::createScope(
    const ScopeKind kind,
    const std::optional<ScopeId> parent,
    const std::optional<SymbolId> ownerSymbol,
    std::string debugName
) {
    if (parent.has_value() && !isValidScopeId(*parent)) {
        throw std::out_of_range("invalid parent scope id");
    }

    if (ownerSymbol.has_value() && !isValidSymbolId(*ownerSymbol)) {
        throw std::out_of_range("invalid owner symbol id");
    }

    Scope scope;
    scope.id = makeScopeId(scopes_.size());
    scope.parent = parent;
    scope.kind = kind;
    scope.ownerSymbol = ownerSymbol;
    scope.debugName = std::move(debugName);

    const ScopeId id = scope.id;
    scopes_.push_back(std::move(scope));

    if (parent.has_value()) {
        this->scope(*parent).children.push_back(id);
    }

    if (ownerSymbol.has_value()) {
        this->symbol(*ownerSymbol).introducedScope = id;
    }

    return id;
}

SymbolId SymbolTable::insertSymbol(
    const ScopeId scopeId,
    std::string name,
    const SymbolKind kind
) {
    if (!isValidScopeId(scopeId)) {
        throw std::out_of_range("invalid owning scope id");
    }

    Symbol symbol;
    symbol.id = makeSymbolId(symbols_.size());
    symbol.owningScope = scopeId;
    symbol.name = std::move(name);
    symbol.kind = kind;

    const SymbolId id = symbol.id;
    const std::string indexName = symbol.name;

    symbols_.push_back(std::move(symbol));
    scope(scopeId).symbols.push_back(id);
    index_[scopeId][indexName].push_back(id);

    return id;
}

void SymbolTable::addFact(const SymbolId symbolId, Factoid fact)
{
    symbol(symbolId).facts.push_back(std::move(fact));
}

std::vector<SymbolId> SymbolTable::findLocal(
    const ScopeId scopeId,
    const std::string_view name
) const {
    if (!isValidScopeId(scopeId)) {
        throw std::out_of_range("invalid scope id");
    }

    const auto scopeIt = index_.find(scopeId);
    if (scopeIt == index_.end()) {
        return {};
    }

    const auto nameIt = scopeIt->second.find(std::string{name});
    if (nameIt == scopeIt->second.end()) {
        return {};
    }

    return nameIt->second;
}

std::vector<SymbolId> SymbolTable::resolveUpwards(
    const ScopeId fromScope,
    const std::string_view name
) const {
    if (!isValidScopeId(fromScope)) {
        throw std::out_of_range("invalid starting scope id");
    }

    std::vector<SymbolId> result;
    std::optional<ScopeId> current = fromScope;

    while (current.has_value()) {
        auto local = findLocal(*current, name);
        result.insert(result.end(), local.begin(), local.end());
        current = scope(*current).parent;
    }

    return result;
}

std::optional<SymbolId> SymbolTable::findExact(
    const ScopeId scopeId,
    const std::string_view name,
    const SymbolKind kind
) const {
    const auto local = findLocal(scopeId, name);
    for (const auto id : local) {
        if (symbol(id).kind == kind) {
            return id;
        }
    }
    return std::nullopt;
}

const Symbol& SymbolTable::symbol(const SymbolId id) const
{
    return symbols_.at(symbolIndex(id));
}

Symbol& SymbolTable::symbol(const SymbolId id)
{
    return symbols_.at(symbolIndex(id));
}

const Scope& SymbolTable::scope(const ScopeId id) const
{
    return scopes_.at(scopeIndex(id));
}

Scope& SymbolTable::scope(const ScopeId id)
{
    return scopes_.at(scopeIndex(id));
}

void SymbolTable::dump(std::ostream& out) const
{
    out << "SymbolTable dump\n";
    out << "Scopes: " << scopes_.size() << "\n";
    for (const auto& sc : scopes_) {
        out << "  Scope #" << sc.id.value
            << " kind=" << toString(sc.kind)
            << " name=" << sc.debugName
            << " symbols=" << sc.symbols.size()
            << " children=" << sc.children.size()
            << "\n";
    }

    out << "Symbols: " << symbols_.size() << "\n";
    for (const auto& sym : symbols_) {
        out << "  Symbol #" << sym.id.value
            << " scope=#" << sym.owningScope.value
            << " name=" << sym.name
            << " kind=" << toString(sym.kind)
            << " facts=" << sym.facts.size()
            << "\n";
    }
}

bool SymbolTable::isValidSymbolId(const SymbolId id) const noexcept
{
    return id.value > 0 && id.value <= symbols_.size();
}

bool SymbolTable::isValidScopeId(const ScopeId id) const noexcept
{
    return id.value > 0 && id.value <= scopes_.size();
}

SymbolId SymbolTable::makeSymbolId(const std::size_t index) const noexcept
{
    return SymbolId{static_cast<std::uint64_t>(index + 1)};
}

ScopeId SymbolTable::makeScopeId(const std::size_t index) const noexcept
{
    return ScopeId{static_cast<std::uint64_t>(index + 1)};
}

std::size_t SymbolTable::symbolIndex(const SymbolId id) const
{
    if (!isValidSymbolId(id)) {
        throw std::out_of_range("invalid symbol id");
    }
    return static_cast<std::size_t>(id.value - 1);
}

std::size_t SymbolTable::scopeIndex(const ScopeId id) const
{
    if (!isValidScopeId(id)) {
        throw std::out_of_range("invalid scope id");
    }
    return static_cast<std::size_t>(id.value - 1);
}

} // namespace symboltable
