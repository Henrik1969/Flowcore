#include "symboltable/SymbolTable.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <type_traits>
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

[[nodiscard]] std::string_view toString(FactoidKind kind) noexcept {
    switch (kind) {
        case FactoidKind::Visibility:       return "Visibility";
        case FactoidKind::StorageClass:     return "StorageClass";
        case FactoidKind::TypeReference:    return "TypeReference";
        case FactoidKind::AbiTag:           return "AbiTag";
        case FactoidKind::SourceLocation:   return "SourceLocation";
        case FactoidKind::DeclarationState: return "DeclarationState";
        case FactoidKind::ImportOrigin:     return "ImportOrigin";
        case FactoidKind::ExportMarker:     return "ExportMarker";
        case FactoidKind::GeneratedBy:      return "GeneratedBy";
        case FactoidKind::Deprecated:       return "Deprecated";
        case FactoidKind::UserTag:          return "UserTag";
        case FactoidKind::Relation:         return "Relation";
        case FactoidKind::Custom:           return "Custom";
    }
    return "Custom";
}

[[nodiscard]] std::string_view toString(Visibility visibility) noexcept {
    switch (visibility) {
        case Visibility::Unspecified: return "Unspecified";
        case Visibility::Public:      return "Public";
        case Visibility::Protected:   return "Protected";
        case Visibility::Private:     return "Private";
        case Visibility::Internal:    return "Internal";
        case Visibility::Local:       return "Local";
    }
    return "Unspecified";
}

[[nodiscard]] std::string_view toString(DefinitionState state) noexcept {
    switch (state) {
        case DefinitionState::Unknown:         return "Unknown";
        case DefinitionState::Declared:        return "Declared";
        case DefinitionState::Defined:         return "Defined";
        case DefinitionState::ForwardDeclared: return "ForwardDeclared";
        case DefinitionState::Imported:        return "Imported";
        case DefinitionState::External:        return "External";
    }
    return "Unknown";
}

void dumpJsonString(std::ostream& out, const std::string_view value)
{
    out << '"';
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u00"
                        << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned int>(ch)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    out << '"';
}

void dumpJsonLocation(std::ostream& out, const SourceLocation& location)
{
    out << "{\"file\": ";
    dumpJsonString(out, location.file);
    out << ", \"line\": " << location.line
        << ", \"column\": " << location.column << '}';
}

template <typename Id>
void dumpJsonOptionalId(std::ostream& out, const std::optional<Id>& id)
{
    if (id) {
        out << id->value;
    } else {
        out << "null";
    }
}

template <typename Id>
void dumpJsonIds(std::ostream& out, const std::vector<Id>& ids)
{
    out << '[';
    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index != 0) {
            out << ", ";
        }
        out << ids[index].value;
    }
    out << ']';
}

void dumpJsonFactoidValue(std::ostream& out, const FactoidValue& factValue)
{
    std::visit([&out](const auto& value) {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, std::monostate>) {
            out << "{\"type\": \"null\", \"value\": null}";
        } else if constexpr (std::is_same_v<Value, bool>) {
            out << "{\"type\": \"bool\", \"value\": "
                << (value ? "true" : "false") << '}';
        } else if constexpr (std::is_same_v<Value, std::int64_t>) {
            out << "{\"type\": \"int64\", \"value\": " << value << '}';
        } else if constexpr (std::is_same_v<Value, double>) {
            out << "{\"type\": \"double\", \"value\": ";
            if (std::isfinite(value)) {
                out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
            } else if (std::isnan(value)) {
                out << "\"nan\"";
            } else if (value > 0.0) {
                out << "\"positive_infinity\"";
            } else {
                out << "\"negative_infinity\"";
            }
            out << '}';
        } else if constexpr (std::is_same_v<Value, std::string>) {
            out << "{\"type\": \"string\", \"value\": ";
            dumpJsonString(out, value);
            out << '}';
        } else if constexpr (std::is_same_v<Value, SymbolId>) {
            out << "{\"type\": \"symbol_id\", \"value\": " << value.value << '}';
        } else if constexpr (std::is_same_v<Value, ScopeId>) {
            out << "{\"type\": \"scope_id\", \"value\": " << value.value << '}';
        } else if constexpr (std::is_same_v<Value, SourceLocation>) {
            out << "{\"type\": \"source_location\", \"value\": ";
            dumpJsonLocation(out, value);
            out << '}';
        }
    }, factValue);
}

void dumpSourceLocation(std::ostream& out, const SourceLocation& location)
{
    out << (location.file.empty() ? "<source>" : location.file)
        << ':' << location.line
        << ':' << location.column;
}

void dumpFactoidValue(std::ostream& out, const FactoidValue& factValue)
{
    std::visit([&out](const auto& value) {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, std::monostate>) {
            out << "null";
        } else if constexpr (std::is_same_v<Value, bool>) {
            out << (value ? "true" : "false");
        } else if constexpr (std::is_same_v<Value, std::string>) {
            out << std::quoted(value);
        } else if constexpr (std::is_same_v<Value, SymbolId> ||
                             std::is_same_v<Value, ScopeId>) {
            out << '#' << value.value;
        } else if constexpr (std::is_same_v<Value, SourceLocation>) {
            dumpSourceLocation(out, value);
        } else {
            out << value;
        }
    }, factValue);
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

        if (sym.declarationLocation) {
            out << "    declaration_location=";
            dumpSourceLocation(out, *sym.declarationLocation);
            out << "\n";
        }

        for (const auto& fact : sym.facts) {
            out << "    Fact kind=" << toString(fact.kind)
                << " key=" << fact.key
                << " value=";
            dumpFactoidValue(out, fact.value);
            out << "\n";
        }
    }
}

void SymbolTable::dumpJson(std::ostream& out) const
{
    out << "{\n"
        << "  \"format\": \"symboltable.snapshot\",\n"
        << "  \"version\": 1,\n"
        << "  \"global_scope_id\": " << globalScope_.value << ",\n"
        << "  \"scopes\": [\n";

    for (std::size_t index = 0; index < scopes_.size(); ++index) {
        const auto& sc = scopes_[index];
        out << "    {\"id\": " << sc.id.value << ", \"parent_id\": ";
        dumpJsonOptionalId(out, sc.parent);
        out << ", \"kind\": ";
        dumpJsonString(out, toString(sc.kind));
        out << ", \"owner_symbol_id\": ";
        dumpJsonOptionalId(out, sc.ownerSymbol);
        out << ", \"debug_name\": ";
        dumpJsonString(out, sc.debugName);
        out << ", \"child_scope_ids\": ";
        dumpJsonIds(out, sc.children);
        out << ", \"symbol_ids\": ";
        dumpJsonIds(out, sc.symbols);
        out << '}';
        if (index + 1 < scopes_.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ],\n"
        << "  \"symbols\": [\n";

    for (std::size_t index = 0; index < symbols_.size(); ++index) {
        const auto& sym = symbols_[index];
        out << "    {\"id\": " << sym.id.value
            << ", \"owning_scope_id\": " << sym.owningScope.value
            << ", \"name\": ";
        dumpJsonString(out, sym.name);
        out << ", \"kind\": ";
        dumpJsonString(out, toString(sym.kind));
        out << ", \"visibility\": ";
        dumpJsonString(out, toString(sym.visibility));
        out << ", \"definition_state\": ";
        dumpJsonString(out, toString(sym.definitionState));
        out << ", \"introduced_scope_id\": ";
        dumpJsonOptionalId(out, sym.introducedScope);
        out << ", \"declaration_location\": ";
        if (sym.declarationLocation) {
            dumpJsonLocation(out, *sym.declarationLocation);
        } else {
            out << "null";
        }
        out << ", \"definition_location\": ";
        if (sym.definitionLocation) {
            dumpJsonLocation(out, *sym.definitionLocation);
        } else {
            out << "null";
        }
        out << ", \"facts\": [";
        for (std::size_t factIndex = 0; factIndex < sym.facts.size(); ++factIndex) {
            const auto& fact = sym.facts[factIndex];
            if (factIndex != 0) {
                out << ", ";
            }
            out << "{\"kind\": ";
            dumpJsonString(out, toString(fact.kind));
            out << ", \"key\": ";
            dumpJsonString(out, fact.key);
            out << ", \"value\": ";
            dumpJsonFactoidValue(out, fact.value);
            out << '}';
        }
        out << "]}";
        if (index + 1 < symbols_.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n"
        << "}";
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
