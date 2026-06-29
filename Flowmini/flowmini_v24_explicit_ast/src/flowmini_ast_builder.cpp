#include "flowmini_ast_builder.h"

namespace flowmini::ast {

    namespace {

        bool is_newline_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Newline;
        }

        SourceLocation location_from_token(const flowmini::Token& token) {
            const auto line = token.line < 0 ? 0 : token.line;
            const auto column = token.column < 0 ? 0 : token.column;

            return SourceLocation{
                static_cast<std::size_t>(line),
                static_cast<std::size_t>(column)
            };
        }

    } // namespace

    AstModule build_source_header_ast(const std::vector<flowmini::Token>& tokens) {
        AstModule module = make_empty_ast_module();

        std::size_t i = 0;

        while (i < tokens.size() && is_newline_token(tokens[i])) {
            ++i;
        }

        if (i >= tokens.size()) {
            return module;
        }

        const flowmini::Token& kindToken = tokens[i];

        if (kindToken.kind == flowmini::TokenKind::KeywordProgram) {
            module.source_unit.kind = SourceUnitKind::Program;
        } else if (kindToken.kind == flowmini::TokenKind::KeywordUnit) {
            module.source_unit.kind = SourceUnitKind::Unit;
        } else {
            module.source_unit.kind = SourceUnitKind::Unknown;
            module.source_unit.location = location_from_token(kindToken);
            return module;
        }

        module.source_unit.location = location_from_token(kindToken);

        ++i;

        while (i < tokens.size() && is_newline_token(tokens[i])) {
            ++i;
        }

        if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Identifier) {
            module.source_unit.name = tokens[i].text;
        }

        return module;
    }

} // namespace flowmini::ast