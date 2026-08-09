#pragma once

namespace symboltable {

enum class SymbolKind {
    Unknown,
    Variable,
    Constant,
    Function,
    Procedure,
    Method,
    Type,
    Class,
    Struct,
    Enum,
    EnumValue,
    Namespace,
    Module,
    Field,
    Parameter,
    Label,
    Macro,
    Import,
    Alias,
    Contract,
    Port,
    Wire,
    Node
};

} // namespace symboltable
