#ifndef FLOWMINI_PARSER_H
#define FLOWMINI_PARSER_H

#include "flowmini_lexer.h"
#include "flowmini_schema.h"

#include <vector>

namespace flowmini {

[[nodiscard]] ModuleSpec parseModule(const std::vector<Token>& tokens);

} // namespace flowmini

#endif // FLOWMINI_PARSER_H
