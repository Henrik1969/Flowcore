#ifndef FLOWMINI_STRUCTURAL_H
#define FLOWMINI_STRUCTURAL_H

#include "flowmini_lexer.h"
#include "flowmini_schema.h"

#include <iosfwd>
#include <vector>

namespace flowmini {

void writeTokenTreeDump(const std::vector<Token>& tokens, std::ostream& out);
void writeFlowIrSymbolTableDump(const ModuleSpec& module, std::ostream& out);

} // namespace flowmini

#endif // FLOWMINI_STRUCTURAL_H
