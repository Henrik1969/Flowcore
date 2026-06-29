//
// Created by henrik on 30.06.2026.
//

#ifndef FLOWMINI_FLOWMINI_AST_BUILDER_H
#define FLOWMINI_FLOWMINI_AST_BUILDER_H

#include "flowmini_ast.h"
#include "flowmini_lexer.h"

#include <vector>

namespace flowmini::ast {

AstModule build_source_header_ast(const std::vector<flowmini::Token>& tokens);

} // namespace flowmini::ast

#endif // FLOWMINI_FLOWMINI_AST_BUILDER_H
