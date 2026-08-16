#ifndef FLOWMINI_SYMBOL_PROJECTION_H
#define FLOWMINI_SYMBOL_PROJECTION_H

#include "flowmini_ast.h"

#include "symboltable/SymbolTable.hpp"

#include <string>
#include <vector>

namespace flowmini::ast {

struct SymbolAstOrigin final {
    symboltable::SymbolId symbol_id {};
    std::string ast_path;
};

struct ScopeAstOrigin final {
    symboltable::ScopeId scope_id {};
    std::string ast_path;
};

struct SymbolProjection final {
    symboltable::SymbolTable table;
    std::vector<SymbolAstOrigin> symbol_origins;
    std::vector<ScopeAstOrigin> scope_origins;
};

SymbolProjection build_symbol_projection(const AstModule& module);
symboltable::SymbolTable build_symbol_table_projection(const AstModule& module);

} // namespace flowmini::ast

#endif // FLOWMINI_SYMBOL_PROJECTION_H
