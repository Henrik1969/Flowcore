#pragma once

namespace symboltable {

enum class ScopeKind {
    Global,
    Module,
    Namespace,
    Type,
    Function,
    Block,
    Class,
    Struct,
    Enum,
    Contract,
    Node,
    Anonymous
};

} // namespace symboltable
