#include "flowmini_structural.h"

#include "tokentree.h"
#include "symboltable/Factoid.hpp"
#include "symboltable/ScopeKind.hpp"
#include "symboltable/SymbolKind.hpp"
#include "symboltable/SymbolTable.hpp"

#include <cstdint>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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

[[nodiscard]] TokenTreeTokenView readFlowminiToken(const void* token, void*)
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
        abi.read_token = readFlowminiToken;
        abi.user_data = nullptr;

        TokenTreeStatus status = tokentree_context_create(&abi, &context_);
        if (status != TOKENTREE_OK) {
            throw std::runtime_error(std::string{"TokenTree context error: "} + tokentree_status_message(status));
        }

        status = tokentree_from_tokens(context_, tokens.data(), tokens.size(), &tree_);
        if (status != TOKENTREE_OK) {
            throw std::runtime_error(std::string{"TokenTree build error: "} + tokentree_status_message(status));
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

void dumpTokenTreeNode(const TokenTree* tree, const TokenTreeNodeId node, const unsigned depth, std::ostream& out)
{
    TokenTreeNodeInfo info{};
    const TokenTreeStatus status = tokentree_node_info(tree, node, &info);
    if (status != TOKENTREE_OK) {
        out << std::string(depth * 2U, ' ') << "<invalid node " << node << ">\n";
        return;
    }

    out << std::string(depth * 2U, ' ');
    if (info.kind == TOKENTREE_NODE_ROOT) {
        out << "root";
    } else {
        out << "node=" << node
            << " kind=" << (info.kind == TOKENTREE_NODE_GROUP ? "group" : "token")
            << " token_kind=" << tokenKindName(static_cast<TokenKind>(info.token_kind))
            << " text=\"" << std::string(info.text.data, info.text.size) << "\""
            << " @" << info.range.start_line << ':' << info.range.start_column;
    }
    out << '\n';

    const size_t count = tokentree_node_child_count(tree, node);
    for (size_t i = 0; i < count; ++i) {
        dumpTokenTreeNode(tree, tokentree_node_child(tree, node, i), depth + 1U, out);
    }
}

void addFact(symboltable::SymbolTable& table,
             const symboltable::SymbolId id,
             symboltable::FactoidKind kind,
             std::string key,
             std::string value)
{
    table.addFact(id, symboltable::Factoid{
        .kind = kind,
        .key = std::move(key),
        .value = std::move(value)
    });
}

[[nodiscard]] std::string endpointText(const Endpoint& endpoint)
{
    return endpoint.node + "." + endpoint.port;
}

[[nodiscard]] symboltable::SymbolKind roleToSymbolKind(const std::string& role)
{
    if (role == "wire") { return symboltable::SymbolKind::Wire; }
    if (role == "producer" || role == "node" || role == "sink") { return symboltable::SymbolKind::Node; }
    return symboltable::SymbolKind::Unknown;
}

[[nodiscard]] symboltable::SymbolTable buildFlowIrSymbolTable(const ModuleSpec& module)
{
    symboltable::SymbolTable table;
    const auto global = table.globalScope();

    const auto moduleSymbol = table.insertSymbol(global, module.name, symboltable::SymbolKind::Module);
    const auto moduleScope = table.createScope(symboltable::ScopeKind::Module, global, moduleSymbol, module.name);

    for (const auto& node : module.nodes) {
        const auto symbol = table.insertSymbol(moduleScope, node.id, roleToSymbolKind(node.role));
        addFact(table, symbol, symboltable::FactoidKind::UserTag, "flowmini.role", node.role);
        addFact(table, symbol, symboltable::FactoidKind::TypeReference, "flowmini.atom_kind", node.kind);
    }

    for (const auto& wire : module.wires) {
        const std::string name = endpointText(wire.from) + " => " + endpointText(wire.to);
        const auto symbol = table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Wire);
        addFact(table, symbol, symboltable::FactoidKind::Relation, "flowmini.from", endpointText(wire.from));
        addFact(table, symbol, symboltable::FactoidKind::Relation, "flowmini.to", endpointText(wire.to));
    }

    for (const auto& policy : module.policies) {
        const std::string name = policy.node + "." + policy.key;
        const auto symbol = table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Field);
        addFact(table, symbol, symboltable::FactoidKind::Relation, "flowmini.policy_node", policy.node);
        addFact(table, symbol, symboltable::FactoidKind::UserTag, "flowmini.policy_key", policy.key);
    }

    return table;
}

} // namespace

void writeTokenTreeDump(const std::vector<Token>& tokens, std::ostream& out)
{
    TokenTreeHandle tree{tokens};
    out << "Flowmini TokenTree dump\n";
    out << "tokens: " << tokens.size() << "\n";
    out << "nodes: " << tokentree_node_count(tree.get()) << "\n";
    dumpTokenTreeNode(tree.get(), tokentree_root(tree.get()), 0, out);
}

void writeFlowIrSymbolTableDump(const ModuleSpec& module, std::ostream& out)
{
    auto table = buildFlowIrSymbolTable(module);
    table.dump(out);
}

} // namespace flowmini
