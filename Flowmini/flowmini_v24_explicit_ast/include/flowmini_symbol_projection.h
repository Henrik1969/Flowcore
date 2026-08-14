#ifndef FLOWMINI_SYMBOL_PROJECTION_H
#define FLOWMINI_SYMBOL_PROJECTION_H

#include "flowmini_ast.h"

#include "symboltable/SymbolTable.hpp"

namespace flowmini::ast {

symboltable::SymbolTable build_symbol_table_projection(const AstModule& module);

} // namespace flowmini::ast

#endif // FLOWMINI_SYMBOL_PROJECTION_H
