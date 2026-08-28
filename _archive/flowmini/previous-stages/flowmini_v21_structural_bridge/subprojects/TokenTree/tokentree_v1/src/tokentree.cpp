#include "tokentree.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace {

struct Node {
    TokenTreeNodeKind kind = TOKENTREE_NODE_TOKEN;
    uint32_t token_kind = 0;
    size_t token_index = 0;
    TokenTreeText text{nullptr, 0};
    TokenTreeSourceRange range{0, 0, 0, 0};
    TokenTreeNodeId parent = TOKENTREE_INVALID_NODE;
    std::vector<TokenTreeNodeId> children;
};

bool is_valid_node(const TokenTree *tree, TokenTreeNodeId node);
TokenTreeStatus add_node(TokenTree &tree, Node node, TokenTreeNodeId *out_node);

} // namespace

struct TokenTreeContext {
    TokenTreeTokenAbi token_abi{};
};

struct TokenTree {
    std::vector<Node> nodes;
};

namespace {

bool is_valid_node(const TokenTree *tree, const TokenTreeNodeId node)
{
    return tree != nullptr && node < tree->nodes.size();
}

TokenTreeStatus add_node(TokenTree &tree, Node node, TokenTreeNodeId *out_node)
{
    if (tree.nodes.size() >= TOKENTREE_INVALID_NODE) {
        return TOKENTREE_ERROR_LIMIT_EXCEEDED;
    }

    const auto id = static_cast<TokenTreeNodeId>(tree.nodes.size());
    tree.nodes.push_back(std::move(node));
    *out_node = id;
    return TOKENTREE_OK;
}

void append_child_unchecked(TokenTree &tree, const TokenTreeNodeId parent, const TokenTreeNodeId child)
{
    tree.nodes[parent].children.push_back(child);
    tree.nodes[child].parent = parent;
}

bool would_create_cycle(const TokenTree &tree, const TokenTreeNodeId parent, const TokenTreeNodeId child)
{
    for (auto cursor = parent; cursor != TOKENTREE_INVALID_NODE; cursor = tree.nodes[cursor].parent) {
        if (cursor == child) {
            return true;
        }
    }

    return false;
}

} // namespace

extern "C" {

TokenTreeStatus tokentree_context_create(const TokenTreeTokenAbi *token_abi,
                                         TokenTreeContext **out_context)
{
    if (out_context == nullptr) {
        return TOKENTREE_ERROR_INVALID_ARGUMENT;
    }

    *out_context = nullptr;

    if (token_abi == nullptr || token_abi->token_size == 0 || token_abi->read_token == nullptr) {
        return TOKENTREE_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto context = std::make_unique<TokenTreeContext>();
        context->token_abi = *token_abi;
        *out_context = context.release();
        return TOKENTREE_OK;
    } catch (const std::bad_alloc &) {
        return TOKENTREE_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        return TOKENTREE_ERROR_INVALID_OPERATION;
    }
}

void tokentree_context_destroy(const TokenTreeContext *context)
{
    delete context;
}

TokenTreeStatus tokentree_from_tokens(const TokenTreeContext *context,
                                      const void *tokens,
                                      const size_t token_count,
                                      TokenTree **out_tree)
{
    if (out_tree == nullptr) {
        return TOKENTREE_ERROR_INVALID_ARGUMENT;
    }

    *out_tree = nullptr;

    if (context == nullptr || (tokens == nullptr && token_count != 0)) {
        return TOKENTREE_ERROR_INVALID_ARGUMENT;
    }

    try {
        if (token_count >= std::numeric_limits<TokenTreeNodeId>::max()) {
            return TOKENTREE_ERROR_LIMIT_EXCEEDED;
        }

        auto tree = std::make_unique<TokenTree>();
        tree->nodes.reserve(token_count + 1);

        Node root;
        root.kind = TOKENTREE_NODE_ROOT;
        root.token_index = static_cast<size_t>(-1);
        TokenTreeNodeId root_id = TOKENTREE_INVALID_NODE;
        TokenTreeStatus status = add_node(*tree, root, &root_id);
        if (status != TOKENTREE_OK) {
            return status;
        }

        std::vector<TokenTreeNodeId> parents;
        std::vector<uint32_t> group_ids;
        parents.push_back(root_id);

        const auto *cursor = static_cast<const unsigned char *>(tokens);

        for (size_t i = 0; i < token_count; ++i) {
            const TokenTreeTokenView token =
                context->token_abi.read_token(cursor + (i * context->token_abi.token_size),
                                              context->token_abi.user_data);

            Node node;
            node.kind = token.token_class == TOKENTREE_TOKEN_CLASS_OPEN_GROUP
                ? TOKENTREE_NODE_GROUP
                : TOKENTREE_NODE_TOKEN;
            node.token_kind = token.kind;
            node.token_index = i;
            node.text = token.text;
            node.range = token.range;

            TokenTreeNodeId node_id = TOKENTREE_INVALID_NODE;
            status = add_node(*tree, node, &node_id);
            if (status != TOKENTREE_OK) {
                return status;
            }

            append_child_unchecked(*tree, parents.back(), node_id);

            if (token.token_class == TOKENTREE_TOKEN_CLASS_OPEN_GROUP) {
                parents.push_back(node_id);
                group_ids.push_back(token.group_id);
            } else if (token.token_class == TOKENTREE_TOKEN_CLASS_CLOSE_GROUP &&
                       !group_ids.empty() && group_ids.back() == token.group_id) {
                parents.pop_back();
                group_ids.pop_back();
            }
        }

        *out_tree = tree.release();
        return TOKENTREE_OK;
    } catch (const std::bad_alloc &) {
        return TOKENTREE_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        return TOKENTREE_ERROR_INVALID_OPERATION;
    }
}

void tokentree_destroy(const TokenTree *tree)
{
    delete tree;
}

TokenTreeNodeId tokentree_root(const TokenTree *tree)
{
    if (tree == nullptr || tree->nodes.empty()) {
        return TOKENTREE_INVALID_NODE;
    }

    return static_cast<TokenTreeNodeId>(0);
}

size_t tokentree_node_count(const TokenTree *tree)
{
    return tree == nullptr ? 0 : tree->nodes.size();
}

TokenTreeStatus tokentree_node_info(const TokenTree *tree,
                                    const TokenTreeNodeId node,
                                    TokenTreeNodeInfo *out_info)
{
    if (out_info == nullptr) {
        return TOKENTREE_ERROR_INVALID_ARGUMENT;
    }

    if (!is_valid_node(tree, node)) {
        return TOKENTREE_ERROR_INVALID_NODE;
    }

    const Node &stored_node = tree->nodes[node];
    out_info->kind = stored_node.kind;
    out_info->token_kind = stored_node.token_kind;
    out_info->token_index = stored_node.token_index;
    out_info->text = stored_node.text;
    out_info->range = stored_node.range;
    return TOKENTREE_OK;
}

TokenTreeNodeId tokentree_node_parent(const TokenTree *tree, const TokenTreeNodeId node)
{
    return is_valid_node(tree, node) ? tree->nodes[node].parent : TOKENTREE_INVALID_NODE;
}

size_t tokentree_node_child_count(const TokenTree *tree, const TokenTreeNodeId node)
{
    return is_valid_node(tree, node) ? tree->nodes[node].children.size() : 0;
}

TokenTreeNodeId tokentree_node_child(const TokenTree *tree,
                                     const TokenTreeNodeId node,
                                     const size_t child_index)
{
    if (!is_valid_node(tree, node) || child_index >= tree->nodes[node].children.size()) {
        return TOKENTREE_INVALID_NODE;
    }

    return tree->nodes[node].children[child_index];
}

TokenTreeStatus tokentree_node_append_child(TokenTree *tree,
                                            const TokenTreeNodeId parent,
                                            const TokenTreeNodeId child)
{
    if (!is_valid_node(tree, parent) || !is_valid_node(tree, child)) {
        return TOKENTREE_ERROR_INVALID_NODE;
    }

    if (parent == child || child == 0 || would_create_cycle(*tree, parent, child)) {
        return TOKENTREE_ERROR_INVALID_OPERATION;
    }

    const TokenTreeNodeId old_parent = tree->nodes[child].parent;
    if (old_parent != TOKENTREE_INVALID_NODE) {
        auto &siblings = tree->nodes[old_parent].children;
        std::erase(siblings, child);
    }

    append_child_unchecked(*tree, parent, child);
    return TOKENTREE_OK;
}

TokenTreeStatus tokentree_node_detach(TokenTree *tree, const TokenTreeNodeId node)
{
    if (!is_valid_node(tree, node)) {
        return TOKENTREE_ERROR_INVALID_NODE;
    }

    if (node == 0) {
        return TOKENTREE_ERROR_INVALID_OPERATION;
    }

    const TokenTreeNodeId parent = tree->nodes[node].parent;
    if (parent == TOKENTREE_INVALID_NODE) {
        return TOKENTREE_OK;
    }

    auto &siblings = tree->nodes[parent].children;
    std::erase(siblings, node);
    tree->nodes[node].parent = TOKENTREE_INVALID_NODE;
    return TOKENTREE_OK;
}

const char *tokentree_status_message(const TokenTreeStatus status)
{
    switch (status) {
    case TOKENTREE_OK:
        return "ok";
    case TOKENTREE_ERROR_INVALID_ARGUMENT:
        return "invalid argument";
    case TOKENTREE_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case TOKENTREE_ERROR_INVALID_NODE:
        return "invalid node";
    case TOKENTREE_ERROR_INVALID_OPERATION:
        return "invalid operation";
    case TOKENTREE_ERROR_LIMIT_EXCEEDED:
        return "limit exceeded";
    }

    return "unknown status";
}

} // extern "C"
