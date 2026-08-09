#ifndef FLOWMINI_TOKEN_TREE_BRIDGE_H
#define FLOWMINI_TOKEN_TREE_BRIDGE_H

#include "flowmini_lexer.h"

#include <iosfwd>
#include <vector>

namespace flowmini {

enum class TokenTreeBridgeDumpFormat {
    Json,
    Simple,
};

[[nodiscard]] TokenTreeBridgeDumpFormat parseTokenTreeBridgeDumpFormat(const std::string& value);
[[nodiscard]] const char* tokenTreeBridgeDumpFormatName(TokenTreeBridgeDumpFormat format) noexcept;

void writeTokenTreeBridgeDump(const std::vector<Token>& tokens,
                              TokenTreeBridgeDumpFormat format,
                              std::ostream& out);

} // namespace flowmini

#endif // FLOWMINI_TOKEN_TREE_BRIDGE_H
