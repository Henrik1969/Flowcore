#ifndef FLOWMINI_FRONTEND_BUNDLE_H
#define FLOWMINI_FRONTEND_BUNDLE_H

#include "flowmini_ast.h"
#include "flowmini_symbol_projection.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace flowmini::ast {

struct FrontendSourceLineOrigin final {
    std::string source_path;
    std::size_t source_line {0};
};

void dump_frontend_bundle_json(std::ostream& out,
                               const AstModule& module,
                               const SymbolProjection& projection,
                               std::string_view sourcePath,
                               const std::vector<FrontendSourceLineOrigin>& lineOrigins);

} // namespace flowmini::ast

#endif // FLOWMINI_FRONTEND_BUNDLE_H
