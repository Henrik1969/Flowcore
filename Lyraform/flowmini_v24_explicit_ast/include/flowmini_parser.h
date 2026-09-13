#ifndef FLOWMINI_PARSER_H
#define FLOWMINI_PARSER_H

#include "flowmini_lexer.h"
#include "flowmini_schema.h"

#include <iosfwd>
#include <vector>

namespace flowmini {

[[nodiscard]] ModuleSpec parseModule(const std::vector<Token>& tokens);
void writeFlowIr(const ModuleSpec& module, std::ostream& out);

} // namespace flowmini

#endif // FLOWMINI_PARSER_H
