#include "flowmini_token_tree_bridge.h"

#include "tokentree.h"

#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace flowmini {
namespace {

enum GroupId : std::uint32_t {
    GroupNone = 0,
    GroupParen = 1,
    GroupBracket = 2,
    GroupBrace = 3,
};

[[nodiscard]] TokenTreeTokenClass tokenTreeClassFor(const TokenKind kind) noexcept
{
    switch (kind) {
    case TokenKind::LeftParen:
    case TokenKind::LeftBracket:
    case TokenKind::LeftBrace:
        return TOKENTREE_TOKEN_CLASS_OPEN_GROUP;
    case TokenKind::RightParen:
    case TokenKind::RightBracket:
    case TokenKind::RightBrace:
        return TOKENTREE_TOKEN_CLASS_CLOSE_GROUP;
    default:
        return TOKENTREE_TOKEN_CLASS_ATOM;
    }
}

[[nodiscard]] std::uint32_t groupIdFor(const TokenKind kind) noexcept
{
    switch (kind) {
    case TokenKind::LeftParen:
    case TokenKind::RightParen:
        return GroupParen;
    case TokenKind::LeftBracket:
    case TokenKind::RightBracket:
        return GroupBracket;
    case TokenKind::LeftBrace:
    case TokenKind::RightBrace:
        return GroupBrace;
    default:
        return GroupNone;
    }
}

[[nodiscard]] const char* nodeKindName(const TokenTreeNodeKind kind) noexcept
{
    switch (kind) {
    case TOKENTREE_NODE_ROOT:
        return "root";
    case TOKENTREE_NODE_TOKEN:
        return "token";
    case TOKENTREE_NODE_GROUP:
        return "group";
    }
    return "unknown";
}

[[nodiscard]] const char* tokenClassName(const TokenKind kind) noexcept
{
    switch (tokenTreeClassFor(kind)) {
    case TOKENTREE_TOKEN_CLASS_ATOM:
        return "atom";
    case TOKENTREE_TOKEN_CLASS_OPEN_GROUP:
        return "open_group";
    case TOKENTREE_TOKEN_CLASS_CLOSE_GROUP:
        return "close_group";
    }
    return "unknown";
}

[[nodiscard]] TokenTreeTokenView readFlowminiTokenForBridge(const void* token, void*)
{
    const auto* stored = static_cast<const Token*>(token);

    TokenTreeTokenView view{};
    view.kind = static_cast<std::uint32_t>(stored->kind);
    view.token_class = tokenTreeClassFor(stored->kind);
    view.group_id = groupIdFor(stored->kind);
    view.text = TokenTreeText{stored->text.data(), stored->text.size()};
    view.range = TokenTreeSourceRange{
        static_cast<std::uint32_t>(stored->line),
        static_cast<std::uint32_t>(stored->column),
        static_cast<std::uint32_t>(stored->line),
        static_cast<std::uint32_t>(stored->column + static_cast<int>(stored->text.size()))
    };
    return view;
}

class TokenTreeHandle final {
public:
    explicit TokenTreeHandle(const std::vector<Token>& tokens)
    {
        TokenTreeTokenAbi abi{};
        abi.token_size = sizeof(Token);
        abi.read_token = readFlowminiTokenForBridge;
        abi.user_data = nullptr;

        TokenTreeStatus status = tokentree_context_create(&abi, &context_);
        if (status != TOKENTREE_OK) {
            throw std::runtime_error(std::string{"TokenTree bridge context error: "} + tokentree_status_message(status));
        }

        status = tokentree_from_tokens(context_, tokens.data(), tokens.size(), &tree_);
        if (status != TOKENTREE_OK) {
            throw std::runtime_error(std::string{"TokenTree bridge build error: "} + tokentree_status_message(status));
        }
    }

    ~TokenTreeHandle()
    {
        tokentree_destroy(tree_);
        tokentree_context_destroy(context_);
    }

    TokenTreeHandle(const TokenTreeHandle&) = delete;
    TokenTreeHandle& operator=(const TokenTreeHandle&) = delete;

    [[nodiscard]] const TokenTree* get() const noexcept { return tree_; }

private:
    TokenTreeContext* context_ = nullptr;
    TokenTree* tree_ = nullptr;
};

void writeJsonEscaped(std::ostream& out, const std::string_view text)
{
    out << '"';
    for (const char c : text) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20U) {
                out << "\\u00";
                constexpr char hex[] = "0123456789abcdef";
                const auto value = static_cast<unsigned char>(c);
                out << hex[(value >> 4U) & 0x0FU] << hex[value & 0x0FU];
            } else {
                out << c;
            }
            break;
        }
    }
    out << '"';
}

[[nodiscard]] TokenTreeNodeInfo nodeInfoOrThrow(const TokenTree* tree, const TokenTreeNodeId node)
{
    TokenTreeNodeInfo info{};
    const TokenTreeStatus status = tokentree_node_info(tree, node, &info);
    if (status != TOKENTREE_OK) {
        throw std::runtime_error(std::string{"TokenTree bridge node error: "} + tokentree_status_message(status));
    }
    return info;
}

void writeJsonNode(const TokenTree* tree, const TokenTreeNodeId node, const unsigned depth, std::ostream& out)
{
    const auto info = nodeInfoOrThrow(tree, node);
    const std::string indent(depth * 2U, ' ');
    const std::string childIndent((depth + 1U) * 2U, ' ');
    const auto childCount = tokentree_node_child_count(tree, node);

    out << indent << "{\n";
    out << childIndent << "\"id\": " << node << ",\n";
    out << childIndent << "\"node_kind\": ";
    writeJsonEscaped(out, nodeKindName(info.kind));

    if (info.kind != TOKENTREE_NODE_ROOT) {
        const auto tokenKind = static_cast<TokenKind>(info.token_kind);
        out << ",\n";
        out << childIndent << "\"token_index\": " << info.token_index << ",\n";
        out << childIndent << "\"token_kind\": ";
        writeJsonEscaped(out, tokenKindName(tokenKind));
        out << ",\n";
        out << childIndent << "\"token_class\": ";
        writeJsonEscaped(out, tokenClassName(tokenKind));
        out << ",\n";
        out << childIndent << "\"group_id\": " << groupIdFor(tokenKind) << ",\n";
        out << childIndent << "\"text\": ";
        writeJsonEscaped(out, std::string_view{info.text.data, info.text.size});
        out << ",\n";
        out << childIndent << "\"range\": {"
            << "\"start_line\": " << info.range.start_line << ", "
            << "\"start_column\": " << info.range.start_column << ", "
            << "\"end_line\": " << info.range.end_line << ", "
            << "\"end_column\": " << info.range.end_column << "}";
    }

    out << ",\n";
    out << childIndent << "\"children\": [";
    if (childCount != 0U) { out << '\n'; }
    for (size_t i = 0; i < childCount; ++i) {
        writeJsonNode(tree, tokentree_node_child(tree, node, i), depth + 2U, out);
        if (i + 1U != childCount) { out << ','; }
        out << '\n';
    }
    if (childCount != 0U) { out << childIndent; }
    out << "]\n";
    out << indent << '}';
}

void writeJsonDump(const TokenTree* tree, const std::vector<Token>& tokens, std::ostream& out)
{
    out << "{\n";
    out << "  \"format\": \"flowmini.token_tree_bridge.v1\",\n";
    out << "  \"policy\": \"json\",\n";
    out << "  \"token_count\": " << tokens.size() << ",\n";
    out << "  \"node_count\": " << tokentree_node_count(tree) << ",\n";
    out << "  \"root\": ";
    writeJsonNode(tree, tokentree_root(tree), 1U, out);
    out << "\n}\n";
}

void writeSimpleNode(const TokenTree* tree, const TokenTreeNodeId node, const unsigned depth, std::ostream& out)
{
    const auto info = nodeInfoOrThrow(tree, node);
    out << std::string(depth * 2U, ' ');

    if (info.kind == TOKENTREE_NODE_ROOT) {
        out << "root";
    } else {
        const auto tokenKind = static_cast<TokenKind>(info.token_kind);
        out << nodeKindName(info.kind)
            << ' ' << tokenKindName(tokenKind)
            << " \"" << std::string(info.text.data, info.text.size) << "\""
            << " @" << info.range.start_line << ':' << info.range.start_column;
    }
    out << '\n';

    const auto childCount = tokentree_node_child_count(tree, node);
    for (size_t i = 0; i < childCount; ++i) {
        writeSimpleNode(tree, tokentree_node_child(tree, node, i), depth + 1U, out);
    }
}

void writeSimpleDump(const TokenTree* tree, const std::vector<Token>& tokens, std::ostream& out)
{
    out << "Flowmini TokenTree bridge dump\n";
    out << "policy: simple\n";
    out << "tokens: " << tokens.size() << "\n";
    out << "nodes: " << tokentree_node_count(tree) << "\n";
    writeSimpleNode(tree, tokentree_root(tree), 0U, out);
}

} // namespace

TokenTreeBridgeDumpFormat parseTokenTreeBridgeDumpFormat(const std::string& value)
{
    if (value.empty() || value == "json") { return TokenTreeBridgeDumpFormat::Json; }
    if (value == "simple") { return TokenTreeBridgeDumpFormat::Simple; }
    throw std::runtime_error{"unknown token-tree bridge dump format '" + value + "'; expected: json or simple"};
}

const char* tokenTreeBridgeDumpFormatName(const TokenTreeBridgeDumpFormat format) noexcept
{
    switch (format) {
    case TokenTreeBridgeDumpFormat::Json:
        return "json";
    case TokenTreeBridgeDumpFormat::Simple:
        return "simple";
    }
    return "json";
}

void writeTokenTreeBridgeDump(const std::vector<Token>& tokens,
                              const TokenTreeBridgeDumpFormat format,
                              std::ostream& out)
{
    const TokenTreeHandle tree{tokens};
    switch (format) {
    case TokenTreeBridgeDumpFormat::Json:
        writeJsonDump(tree.get(), tokens, out);
        return;
    case TokenTreeBridgeDumpFormat::Simple:
        writeSimpleDump(tree.get(), tokens, out);
        return;
    }
}

} // namespace flowmini
