//
// Created by henrik on 29.06.2026.
//
#ifndef FLOWMINI_AST_H
#define FLOWMINI_AST_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <ostream>
#include <variant>
#include <vector>

namespace flowmini::ast {

struct SourceLocation {
    std::size_t line = 0;
    std::size_t column = 0;
};

enum class SourceUnitKind {
    Program,
    Unit,
    Unknown
};

enum class TopLevelKind {
    Import,
    Function,
    Record,
    TypeAlias,
    MainBlock,
    Unknown
};

enum class StatementKind {
    Block,
    Let,
    Assignment,
    If,
    While,
    Break,
    Continue,
    Return,
    Expression,
    Flow,
    Unknown
};

enum class ExpressionKind {
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BoolLiteral,
    Call,
    Unary,
    Binary,
    Index,
    FieldAccess,
    ListLiteral,
    RecordLiteral,
    Unknown
};

enum class TypeRefKind {
    Named,
    Generic,
    Array,
    Unknown
};

struct TypeRef;

struct UnknownTypeRef {
    std::string text;
};

struct NamedTypeRef {
    std::vector<std::string> name_segments;
};

struct GenericTypeRef {
    std::vector<std::string> constructor_segments;
    std::vector<TypeRef> arguments;
};

struct ArrayExtent {
    std::string text;
    SourceLocation location;
};

struct ArrayTypeRef {
    std::shared_ptr<const TypeRef> element_type;
    std::vector<ArrayExtent> extents;
};

struct TypeRef {
    SourceLocation location;

    using Payload = std::variant<
        UnknownTypeRef,
        NamedTypeRef,
        GenericTypeRef,
        ArrayTypeRef
    >;

    Payload payload = UnknownTypeRef{};
};

struct Parameter {
    std::string name;
    TypeRef type;

    bool has_body = false;
    SourceLocation body_location;
    SourceLocation location;
};

struct Expression;

struct IdentifierExpr {
    std::string name;
};

struct IntegerLiteralExpr {
    std::string text;
};

struct FloatLiteralExpr {
    std::string text;
};

struct StringLiteralExpr {
    std::string text;
};

struct BoolLiteralExpr {
    std::string text;
};

struct UnaryExpr {
    std::string op;
    std::optional<std::size_t> operand;
};

struct BinaryExpr {
    std::string op;
    std::optional<std::size_t> left;
    std::optional<std::size_t> right;
};

struct CallExpr {
    std::optional<std::size_t> base;
    std::vector<std::size_t> arguments;
};

struct IndexExpr {
    std::optional<std::size_t> base;
    std::vector<std::size_t> indexes;
};

struct FieldAccessExpr {
    std::optional<std::size_t> base;
    std::string field;
};

struct ListLiteralExpr {
    std::vector<std::size_t> elements;
};

struct RecordLiteralFieldExpr {
    std::string name;
    std::optional<std::size_t> value;
    SourceLocation location;
};

struct RecordLiteralExpr {
    std::vector<RecordLiteralFieldExpr> fields;
};

struct UnknownExpr {
    std::string text;
};

struct Expression {
    SourceLocation location;

    using Payload = std::variant<
        UnknownExpr,
        IdentifierExpr,
        IntegerLiteralExpr,
        FloatLiteralExpr,
        StringLiteralExpr,
        BoolLiteralExpr,
        UnaryExpr,
        BinaryExpr,
        CallExpr,
        IndexExpr,
        FieldAccessExpr,
        ListLiteralExpr,
        RecordLiteralExpr
    >;

    Payload payload = UnknownExpr{};
};

struct Statement {
    StatementKind kind = StatementKind::Unknown;
    SourceLocation location;

    // Statement-shell metadata.
    // For typed bindings:
    //     name : Type(...)
    std::string name;
    TypeRef type;

    bool has_initializer = false;
    bool has_value = false;
    bool has_condition = false;

    // C5 canonical statement expression roles.
    // Let owns initializer_expression when initializer syntax contains a value
    // expression. Return owns value_expression. expression_ids remains a
    // derived JSON compatibility projection while C5 migration is in progress.
    std::optional<std::size_t> initializer_expression;
    std::optional<std::size_t> value_expression;

    bool has_body = false;
    SourceLocation body_location;

    std::vector<Statement> body;

    // Legacy statement-expression storage retained while the remaining
    // statement kinds migrate to explicit semantic roles during C5.
    std::vector<std::size_t> child_statements;
    std::vector<std::size_t> expressions;
};

struct FunctionDecl {
    std::string name;
    std::vector<Parameter> parameters;
    TypeRef return_type;
    std::vector<Statement> body;
    SourceLocation location;

    bool has_body = false;
    SourceLocation body_location;

    bool is_extern = false;
    bool is_imported = false;
    std::string origin_module;
};

struct ImportDecl {
    std::string module_name;
    SourceLocation location;
};

struct RecordField {
    std::string name;
    TypeRef type;
    SourceLocation location;
};

struct RecordDecl {
    std::string name;
    std::vector<RecordField> fields;
    SourceLocation location;
};

struct TypeAliasDecl {
    std::string name;
    TypeRef target;
    SourceLocation location;
};

struct MainBlock {
    std::vector<Statement> body;
    SourceLocation location;

    bool has_body = false;
    SourceLocation body_location;
};

using TopLevelDecl = std::variant<
    ImportDecl,
    FunctionDecl,
    RecordDecl,
    TypeAliasDecl,
    MainBlock
>;

struct SourceUnit {
    SourceUnitKind kind = SourceUnitKind::Unknown;
    std::string name;
    std::vector<TopLevelDecl> declarations;
    SourceLocation location;
};

struct AstModule {
    SourceUnit source_unit;
    std::vector<Expression> expression_pool;
};

const char* to_string(SourceUnitKind kind);
const char* to_string(TopLevelKind kind);
const char* to_string(StatementKind kind);
const char* to_string(ExpressionKind kind);
const char* to_string(TypeRefKind kind);

TypeRefKind type_ref_kind(const TypeRef& type);
std::string type_ref_text(const TypeRef& type);

ExpressionKind expression_kind(const Expression& expression);
std::string expression_text(const Expression& expression,
                            const std::vector<Expression>& expression_pool);
std::vector<std::size_t> expression_children(const Expression& expression);

TopLevelKind top_level_kind(const TopLevelDecl& decl);

void dump_ast_json(std::ostream& out, const AstModule& module);

AstModule make_empty_ast_module();

} // namespace flowmini::ast

#endif // FLOWMINI_AST_H
