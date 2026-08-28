#pragma once

namespace symboltable {

enum class DefinitionState {
    Unknown,
    Declared,
    Defined,
    ForwardDeclared,
    Imported,
    External
};

} // namespace symboltable
