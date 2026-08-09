//
// Created by henrik on 29.06.2026.
//
#ifndef FLOWMINI_AST_H
#define FLOWMINI_AST_H

#include <cstddef>
#include <cstdint>
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
    Pointer,
    Reference,
    Unknown
};

struct TypeRef {
    TypeRefKind kind = TypeRefKind::Unknown;

    // For simple current types:
    //     int
    //     Bool
    //     c_string
    std::string name;

    // Preserves source spelling while parser support is incomplete:
    //     array<int>[2, 3]
    //     list<int>
    std::string raw_text;

    // Future semantic structure:
    std::vector<TypeRef> arguments;
    std::vector<std::string> dimensions;

    SourceLocation location;
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
    std::string text;
};

struct LiteralExpr {
    std::string text;
};

struct UnaryExpr {
    std::string op;
    std::size_t operand = 0;
};

struct BinaryExpr {
    std::string op;
    std::size_t left = 0;
    std::size_t right = 0;
};

struct CallExpr {
    std::string callee;
    std::vector<std::size_t> arguments;
};

struct Expression {
    std::string text;
    ExpressionKind kind = ExpressionKind::Unknown;
    SourceLocation location;
    std::vector<std::size_t> child_expressions;

    using Payload = std::variant<
        std::monostate,
        IdentifierExpr,
        LiteralExpr,
        UnaryExpr,
        BinaryExpr,
        CallExpr
    >;

    Payload payload;
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

    bool has_body = false;
    SourceLocation body_location;

    std::vector<Statement> body;

    // For v24 this remains intentionally skeletal.
    // Expressions and nested statement bodies are populated in later steps.
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

TopLevelKind top_level_kind(const TopLevelDecl& decl);

void dump_ast_json(std::ostream& out, const AstModule& module);

AstModule make_empty_ast_module();

} // namespace flowmini::ast

#endif // FLOWMINI_AST_H