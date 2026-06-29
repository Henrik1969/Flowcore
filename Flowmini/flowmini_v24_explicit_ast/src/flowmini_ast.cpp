// Flowmini AST
// ------------
// This is the language-aware parsed source representation.
//
// It is intentionally separate from TokenTree.
// TokenTree preserves grouped token structure.
// AST represents Flowmini source meaning.

#include "flowmini_ast.h"

namespace flowmini::ast {

const char* to_string(SourceUnitKind kind) {
    switch (kind) {
        case SourceUnitKind::Program:
            return "program";
        case SourceUnitKind::Unit:
            return "unit";
        case SourceUnitKind::Unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(TopLevelKind kind) {
    switch (kind) {
        case TopLevelKind::Import:
            return "import";
        case TopLevelKind::Function:
            return "function";
        case TopLevelKind::Record:
            return "record";
        case TopLevelKind::TypeAlias:
            return "type_alias";
        case TopLevelKind::MainBlock:
            return "main_block";
        case TopLevelKind::Unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(StatementKind kind) {
    switch (kind) {
        case StatementKind::Block:
            return "block";
        case StatementKind::Let:
            return "let";
        case StatementKind::Assignment:
            return "assignment";
        case StatementKind::If:
            return "if";
        case StatementKind::While:
            return "while";
        case StatementKind::Break:
            return "break";
        case StatementKind::Continue:
            return "continue";
        case StatementKind::Return:
            return "return";
        case StatementKind::Expression:
            return "expression";
        case StatementKind::Flow:
            return "flow";
        case StatementKind::Unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(ExpressionKind kind) {
    switch (kind) {
        case ExpressionKind::Identifier:
            return "identifier";
        case ExpressionKind::IntegerLiteral:
            return "integer_literal";
        case ExpressionKind::FloatLiteral:
            return "float_literal";
        case ExpressionKind::StringLiteral:
            return "string_literal";
        case ExpressionKind::BoolLiteral:
            return "bool_literal";
        case ExpressionKind::Call:
            return "call";
        case ExpressionKind::Unary:
            return "unary";
        case ExpressionKind::Binary:
            return "binary";
        case ExpressionKind::Index:
            return "index";
        case ExpressionKind::FieldAccess:
            return "field_access";
        case ExpressionKind::ListLiteral:
            return "list_literal";
        case ExpressionKind::RecordLiteral:
            return "record_literal";
        case ExpressionKind::Unknown:
            return "unknown";
    }

    return "unknown";
}

TopLevelKind top_level_kind(const TopLevelDecl& decl) {
    if (std::holds_alternative<ImportDecl>(decl)) {
        return TopLevelKind::Import;
    }

    if (std::holds_alternative<FunctionDecl>(decl)) {
        return TopLevelKind::Function;
    }

    if (std::holds_alternative<RecordDecl>(decl)) {
        return TopLevelKind::Record;
    }

    if (std::holds_alternative<TypeAliasDecl>(decl)) {
        return TopLevelKind::TypeAlias;
    }

    if (std::holds_alternative<MainBlock>(decl)) {
        return TopLevelKind::MainBlock;
    }

    return TopLevelKind::Unknown;
}

    AstModule make_empty_ast_module() {
    AstModule module;
    module.source_unit.kind = SourceUnitKind::Unknown;
    module.source_unit.name = "";
    module.source_unit.location = SourceLocation{};
    return module;
}

void dump_ast_json(std::ostream& out, const AstModule& module) {
    out << "{\n";
    out << "  \"format\": \"flowmini.ast.v1\",\n";
    out << "  \"source_unit\": {\n";
    out << "    \"kind\": \"" << to_string(module.source_unit.kind) << "\",\n";
    out << "    \"name\": \"" << module.source_unit.name << "\",\n";
    out << "    \"declaration_count\": " << module.source_unit.declarations.size() << ",\n";
    out << "    \"declarations\": []\n";
    out << "  },\n";
    out << "  \"expression_pool_size\": " << module.expression_pool.size() << "\n";
    out << "}\n";
}

} // namespace flowmini::ast