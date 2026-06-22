#pragma once

#include "symboltable/IdTypes.hpp"
#include "symboltable/SourceLocation.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace symboltable {

enum class FactoidKind {
    Visibility,
    StorageClass,
    TypeReference,
    AbiTag,
    SourceLocation,
    DeclarationState,
    ImportOrigin,
    ExportMarker,
    GeneratedBy,
    Deprecated,
    UserTag,
    Relation,
    Custom
};

using FactoidValue = std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string,
    SymbolId,
    ScopeId,
    SourceLocation
>;

struct Factoid final {
    FactoidKind kind {FactoidKind::Custom};
    std::string key;
    FactoidValue value {};
};

} // namespace symboltable
