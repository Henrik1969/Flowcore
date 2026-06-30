// Flowmini AST
// ------------
// This is the language-aware parsed source representation.
//
// It is intentionally separate from TokenTree.
// TokenTree preserves grouped token structure.
// AST represents Flowmini source meaning.

#include "flowmini_ast.h"
#include <variant>

namespace flowmini::ast {
    // Helpers
    namespace {

        void dump_indent(std::ostream& out, std::size_t spaces) {

            for (std::size_t i = 0; i < spaces; ++i) {
                out << ' ';
            }
        }

        void dump_json_string(std::ostream& out, const std::string& value) {
            out << '"';

            for (const char ch : value) {
                switch (ch) {
                    case '\\': out << "\\\\"; break;
                    case '"':  out << "\\\""; break;
                    case '\n': out << "\\n";  break;
                    case '\r': out << "\\r";  break;
                    case '\t': out << "\\t";  break;
                    default:   out << ch;     break;
                }
            }

            out << '"';
        }

        void dump_top_level_decl_json(std::ostream& out, const TopLevelDecl& decl, std::size_t indent) {
            dump_indent(out, indent);
            out << "{\n";

            const TopLevelKind kind = top_level_kind(decl);

            dump_indent(out, indent + 2);
            out << "\"kind\": \"" << to_string(kind) << "\"";

            if (const auto* importDecl = std::get_if<ImportDecl>(&decl)) {
                out << ",\n";
                dump_indent(out, indent + 2);
                out << "\"module_name\": ";
                dump_json_string(out, importDecl->module_name);
                out << "\n";
            } else if (const auto* functionDecl = std::get_if<FunctionDecl>(&decl)) {
                out << ",\n";
                dump_indent(out, indent + 2);
                out << "\"name\": ";
                dump_json_string(out, functionDecl->name);
                out << ",\n";

                dump_indent(out, indent + 2);
                out << "\"parameter_count\": " << functionDecl->parameters.size() << ",\n";

                dump_indent(out, indent + 2);
                out << "\"return_type\": ";
                dump_json_string(out, functionDecl->return_type.name);
                out << ",\n";

                dump_indent(out, indent + 2);
                out << "\"body_statement_count\": " << functionDecl->body.size() << "\n";
            } else if (const auto* mainBlock = std::get_if<MainBlock>(&decl)) {
                out << ",\n";
                dump_indent(out, indent + 2);
                out << "\"body_statement_count\": " << mainBlock->body.size() << "\n";
            } else if (const auto* recordDecl = std::get_if<RecordDecl>(&decl)) {
                out << ",\n";
                dump_indent(out, indent + 2);
                out << "\"name\": ";
                dump_json_string(out, recordDecl->name);
                out << ",\n";

                dump_indent(out, indent + 2);
                out << "\"field_count\": " << recordDecl->fields.size() << "\n";
            } else if (const auto* typeAliasDecl = std::get_if<TypeAliasDecl>(&decl)) {
                out << ",\n";
                dump_indent(out, indent + 2);
                out << "\"name\": ";
                dump_json_string(out, typeAliasDecl->name);
                out << ",\n";

                dump_indent(out, indent + 2);
                out << "\"target\": ";
                dump_json_string(out, typeAliasDecl->target.name);
                out << "\n";
            } else {
                out << "\n";
            }

            dump_indent(out, indent);
            out << "}";
        }
    }// end namespace for helpers

    const char* to_string(SourceUnitKind kind) {
        switch (kind) {
            case SourceUnitKind::Program:   return "program";
            case SourceUnitKind::Unit:      return "unit";
            case SourceUnitKind::Unknown:   return "unknown";
        }
        return "unknown";
    }

    const char* to_string(TopLevelKind kind) {
        switch (kind) {
            case TopLevelKind::Import:      return "import";
            case TopLevelKind::Function:    return "function";
            case TopLevelKind::Record:      return "record";
            case TopLevelKind::TypeAlias:   return "type_alias";
            case TopLevelKind::MainBlock:   return "main_block";
            case TopLevelKind::Unknown:     return "unknown";
        }
        return "unknown";
    }

    const char* to_string(StatementKind kind) {
        switch (kind) {
            case StatementKind::Block:      return "block";
            case StatementKind::Let:        return "let";
            case StatementKind::Assignment: return "assignment";
            case StatementKind::If:         return "if";
            case StatementKind::While:      return "while";
            case StatementKind::Break:      return "break";
            case StatementKind::Continue:   return "continue";
            case StatementKind::Return:     return "return";
            case StatementKind::Expression: return "expression";
            case StatementKind::Flow:       return "flow";
            case StatementKind::Unknown:    return "unknown";
        }
        return "unknown";
    }

    const char* to_string(ExpressionKind kind) {
        switch (kind) {
            case ExpressionKind::Identifier:        return "identifier";
            case ExpressionKind::IntegerLiteral:    return "integer_literal";
            case ExpressionKind::FloatLiteral:      return "float_literal";
            case ExpressionKind::StringLiteral:     return "string_literal";
            case ExpressionKind::BoolLiteral:       return "bool_literal";
            case ExpressionKind::Call:              return "call";
            case ExpressionKind::Unary:             return "unary";
            case ExpressionKind::Binary:            return "binary";
            case ExpressionKind::Index:             return "index";
            case ExpressionKind::FieldAccess:       return "field_access";
            case ExpressionKind::ListLiteral:       return "list_literal";
            case ExpressionKind::RecordLiteral:     return "record_literal";
            case ExpressionKind::Unknown:           return "unknown";
        }
        return "unknown";
    }

    TopLevelKind top_level_kind(const TopLevelDecl& decl) {
        if (std::holds_alternative<ImportDecl>(decl))       { return TopLevelKind::Import;}
        if (std::holds_alternative<FunctionDecl>(decl))     { return TopLevelKind::Function;}
        if (std::holds_alternative<RecordDecl>(decl))       { return TopLevelKind::Record;}
        if (std::holds_alternative<TypeAliasDecl>(decl))    { return TopLevelKind::TypeAlias;}
        if (std::holds_alternative<MainBlock>(decl))        { return TopLevelKind::MainBlock;}
        return TopLevelKind::Unknown;
    }

    AstModule make_empty_ast_module() {
        AstModule module;
        module.source_unit.kind     = SourceUnitKind::Unknown;
        module.source_unit.name     = "";
        module.source_unit.location = SourceLocation{};
        return module;
    }

    void dump_ast_json(std::ostream& out, const AstModule& module) {
        out << "{\n";
        out << "  \"format\": \"flowmini.ast.v1\",\n";
        out << "  \"source_unit\": {\n";
        out << "    \"kind\": \"" << to_string(module.source_unit.kind) << "\",\n";
        out << "    \"name\": "; dump_json_string(out, module.source_unit.name);out << ",\n";

        out << "    \"declaration_count\": " << module.source_unit.declarations.size() << ",\n";
        out << "    \"declarations\": [\n";

        for (std::size_t i = 0; i < module.source_unit.declarations.size(); ++i) {
            dump_top_level_decl_json(out, module.source_unit.declarations[i], 6);

            if (i + 1 < module.source_unit.declarations.size()) {
                out << ",";
            }

            out << "\n";
        }
        out << "    ]\n";

        out << "  },\n";
        out << "  \"expression_pool_size\": " << module.expression_pool.size() << "\n";
        out << "}\n";
    }

} // namespace flowmini::ast