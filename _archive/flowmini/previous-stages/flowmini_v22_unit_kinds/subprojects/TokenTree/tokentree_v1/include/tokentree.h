#ifndef TOKENTREE_H
#define TOKENTREE_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(TOKENTREE_BUILDING_SHARED)
#    define TOKENTREE_API __declspec(dllexport)
#  elif defined(TOKENTREE_USING_SHARED)
#    define TOKENTREE_API __declspec(dllimport)
#  else
#    define TOKENTREE_API
#  endif
#else
#  if defined(TOKENTREE_BUILDING_SHARED)
#    define TOKENTREE_API __attribute__((visibility("default")))
#  else
#    define TOKENTREE_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TokenTreeContext TokenTreeContext;
typedef struct TokenTree TokenTree;

typedef uint32_t TokenTreeNodeId;

enum {
    TOKENTREE_INVALID_NODE = UINT32_MAX
};

typedef enum TokenTreeStatus {
    TOKENTREE_OK = 0,
    TOKENTREE_ERROR_INVALID_ARGUMENT = 1,
    TOKENTREE_ERROR_OUT_OF_MEMORY = 2,
    TOKENTREE_ERROR_INVALID_NODE = 3,
    TOKENTREE_ERROR_INVALID_OPERATION = 4,
    TOKENTREE_ERROR_LIMIT_EXCEEDED = 5
} TokenTreeStatus;

typedef enum TokenTreeTokenClass {
    TOKENTREE_TOKEN_CLASS_ATOM = 0,
    TOKENTREE_TOKEN_CLASS_OPEN_GROUP = 1,
    TOKENTREE_TOKEN_CLASS_CLOSE_GROUP = 2
} TokenTreeTokenClass;

typedef enum TokenTreeNodeKind {
    TOKENTREE_NODE_ROOT = 0,
    TOKENTREE_NODE_TOKEN = 1,
    TOKENTREE_NODE_GROUP = 2
} TokenTreeNodeKind;

/*
 * Borrowed text view.
 *
 * TokenTree stores this pointer and size exactly as supplied by the host token
 * adapter. The bytes are not copied and do not need to be null-terminated.
 * The host must keep referenced memory alive for as long as a TokenTree that
 * stores the view may be inspected.
 */
typedef struct TokenTreeText {
    const char *data;
    size_t size;
} TokenTreeText;

typedef struct TokenTreeSourceRange {
    uint32_t start_line;
    uint32_t start_column;
    uint32_t end_line;
    uint32_t end_column;
} TokenTreeSourceRange;

/*
 * Normalized token view returned by the host-provided adapter.
 *
 * kind is host-defined. token_class and group_id are structural hints only.
 * TokenTree does not interpret token kinds as language syntax or semantics.
 */
typedef struct TokenTreeTokenView {
    uint32_t kind;
    TokenTreeTokenClass token_class;
    uint32_t group_id;
    TokenTreeText text;
    TokenTreeSourceRange range;
} TokenTreeTokenView;

typedef TokenTreeTokenView (*TokenTreeReadTokenFn)(const void *token, void *user_data);

/*
 * ABI adapter contract for foreign token buffers.
 *
 * token_size is the stride between consecutive tokens in the host-owned token
 * buffer. read_token adapts one foreign token to TokenTreeTokenView. user_data
 * is passed through unchanged and remains owned by the host.
 */
typedef struct TokenTreeTokenAbi {
    size_t token_size;
    TokenTreeReadTokenFn read_token;
    void *user_data;
} TokenTreeTokenAbi;

/* Information stored for one tree node. */
typedef struct TokenTreeNodeInfo {
    TokenTreeNodeKind kind;
    uint32_t token_kind;
    size_t token_index;
    TokenTreeText text;
    TokenTreeSourceRange range;
} TokenTreeNodeInfo;

TOKENTREE_API TokenTreeStatus tokentree_context_create(const TokenTreeTokenAbi *token_abi,
                                                       TokenTreeContext **out_context);
TOKENTREE_API void tokentree_context_destroy(const TokenTreeContext *context);

TOKENTREE_API TokenTreeStatus tokentree_from_tokens(const TokenTreeContext *context,
                                                    const void *tokens,
                                                    size_t token_count,
                                                    TokenTree **out_tree);
TOKENTREE_API void tokentree_destroy(const TokenTree *tree);

TOKENTREE_API TokenTreeNodeId tokentree_root(const TokenTree *tree);
TOKENTREE_API size_t tokentree_node_count(const TokenTree *tree);
TOKENTREE_API TokenTreeStatus tokentree_node_info(const TokenTree *tree,
                                                  TokenTreeNodeId node,
                                                  TokenTreeNodeInfo *out_info);

TOKENTREE_API TokenTreeNodeId tokentree_node_parent(const TokenTree *tree, TokenTreeNodeId node);
TOKENTREE_API size_t tokentree_node_child_count(const TokenTree *tree, TokenTreeNodeId node);
TOKENTREE_API TokenTreeNodeId tokentree_node_child(const TokenTree *tree,
                                                   TokenTreeNodeId node,
                                                   size_t child_index);

TOKENTREE_API TokenTreeStatus tokentree_node_append_child(TokenTree *tree,
                                                          TokenTreeNodeId parent,
                                                          TokenTreeNodeId child);
TOKENTREE_API TokenTreeStatus tokentree_node_detach(TokenTree *tree, TokenTreeNodeId node);

TOKENTREE_API const char *tokentree_status_message(TokenTreeStatus status);

#ifdef __cplusplus
}
#endif

#endif
