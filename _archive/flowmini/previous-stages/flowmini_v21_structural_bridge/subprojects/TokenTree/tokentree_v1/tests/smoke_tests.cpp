#include "test_framework.h"
#include "tokentree.h"

#include <cstring>
#include <string_view>

struct HostToken {
    uint32_t kind;
    uint32_t group_id;
    TokenTreeTokenClass token_class;
    const char *text;
};

static TokenTreeTokenView read_host_token(const void *token, void *)
{
    const auto *host = static_cast<const HostToken *>(token);
    TokenTreeTokenView view{};
    view.kind = host->kind;
    view.group_id = host->group_id;
    view.token_class = host->token_class;
    view.text = {host->text, std::strlen(host->text)};
    view.range = {1, 1, 1, static_cast<uint32_t>(view.text.size + 1)};
    return view;
}

static TokenTreeContext *make_context()
{
    TokenTreeTokenAbi abi{};
    abi.token_size = sizeof(HostToken);
    abi.read_token = read_host_token;

    TokenTreeContext *context = nullptr;
    if (tokentree_context_create(&abi, &context) != TOKENTREE_OK) {
        return nullptr;
    }
    return context;
}

TT_TEST(invalid_context_arguments)
{
    TokenTreeContext *context = reinterpret_cast<TokenTreeContext *>(0x1);
    TT_EXPECT(tokentree_context_create(nullptr, &context) == TOKENTREE_ERROR_INVALID_ARGUMENT);
    TT_EXPECT(context == nullptr);

    TokenTreeTokenAbi abi{};
    TT_EXPECT(tokentree_context_create(&abi, &context) == TOKENTREE_ERROR_INVALID_ARGUMENT);
    TT_EXPECT(context == nullptr);
    return true;
}

TT_TEST(empty_tree_has_root)
{
    TokenTreeContext *context = make_context();
    TT_EXPECT(context != nullptr);

    TokenTree *tree = nullptr;
    TT_EXPECT(tokentree_from_tokens(context, nullptr, 0, &tree) == TOKENTREE_OK);
    TT_EXPECT(tree != nullptr);
    TT_EXPECT(tokentree_root(tree) == 0);
    TT_EXPECT(tokentree_node_count(tree) == 1);
    TT_EXPECT(tokentree_node_child_count(tree, 0) == 0);

    tokentree_destroy(tree);
    tokentree_context_destroy(context);
    return true;
}

TT_TEST(groups_are_structural_and_lossless)
{
    HostToken tokens[] = {
        {1, 0, TOKENTREE_TOKEN_CLASS_ATOM, "add"},
        {2, 1, TOKENTREE_TOKEN_CLASS_OPEN_GROUP, "("},
        {3, 0, TOKENTREE_TOKEN_CLASS_ATOM, "1"},
        {4, 0, TOKENTREE_TOKEN_CLASS_ATOM, ","},
        {3, 0, TOKENTREE_TOKEN_CLASS_ATOM, "2"},
        {5, 1, TOKENTREE_TOKEN_CLASS_CLOSE_GROUP, ")"},
    };

    TokenTreeContext *context = make_context();
    TT_EXPECT(context != nullptr);

    TokenTree *tree = nullptr;
    TT_EXPECT(tokentree_from_tokens(context, tokens, 6, &tree) == TOKENTREE_OK);
    TT_EXPECT(tokentree_node_count(tree) == 7);
    TT_EXPECT(tokentree_node_child_count(tree, 0) == 2);

    const TokenTreeNodeId group = tokentree_node_child(tree, 0, 1);
    TokenTreeNodeInfo info{};
    TT_EXPECT(tokentree_node_info(tree, group, &info) == TOKENTREE_OK);
    TT_EXPECT(info.kind == TOKENTREE_NODE_GROUP);
    TT_EXPECT(tokentree_node_child_count(tree, group) == 4);

    const TokenTreeNodeId close = tokentree_node_child(tree, group, 3);
    TT_EXPECT(tokentree_node_info(tree, close, &info) == TOKENTREE_OK);
    TT_EXPECT(info.kind == TOKENTREE_NODE_TOKEN);
    TT_EXPECT(info.text.size == 1);
    TT_EXPECT(info.text.data[0] == ')');

    tokentree_destroy(tree);
    tokentree_context_destroy(context);
    return true;
}

TT_TEST(mutation_rejects_cycles_and_root_move)
{
    HostToken tokens[] = {
        {1, 1, TOKENTREE_TOKEN_CLASS_OPEN_GROUP, "("},
        {2, 1, TOKENTREE_TOKEN_CLASS_OPEN_GROUP, "("},
        {3, 1, TOKENTREE_TOKEN_CLASS_CLOSE_GROUP, ")"},
        {3, 1, TOKENTREE_TOKEN_CLASS_CLOSE_GROUP, ")"},
    };

    TokenTreeContext *context = make_context();
    TT_EXPECT(context != nullptr);

    TokenTree *tree = nullptr;
    TT_EXPECT(tokentree_from_tokens(context, tokens, 4, &tree) == TOKENTREE_OK);

    const TokenTreeNodeId outer = tokentree_node_child(tree, 0, 0);
    const TokenTreeNodeId inner = tokentree_node_child(tree, outer, 0);

    TT_EXPECT(tokentree_node_append_child(tree, inner, 0) == TOKENTREE_ERROR_INVALID_OPERATION);
    TT_EXPECT(tokentree_node_append_child(tree, inner, inner) == TOKENTREE_ERROR_INVALID_OPERATION);
    TT_EXPECT(tokentree_node_append_child(tree, inner, outer) == TOKENTREE_ERROR_INVALID_OPERATION);

    TT_EXPECT(tokentree_node_detach(tree, inner) == TOKENTREE_OK);
    TT_EXPECT(tokentree_node_parent(tree, inner) == TOKENTREE_INVALID_NODE);
    TT_EXPECT(tokentree_node_append_child(tree, 0, inner) == TOKENTREE_OK);
    TT_EXPECT(tokentree_node_parent(tree, inner) == 0);

    tokentree_destroy(tree);
    tokentree_context_destroy(context);
    return true;
}

TT_TEST(status_messages_are_stable)
{
    TT_EXPECT(std::string_view(tokentree_status_message(TOKENTREE_OK)) == "ok");
    TT_EXPECT(std::string_view(tokentree_status_message(TOKENTREE_ERROR_INVALID_ARGUMENT)) == "invalid argument");
    TT_EXPECT(std::string_view(tokentree_status_message(static_cast<TokenTreeStatus>(999))) == "unknown status");
    return true;
}
