#include <cstddef>
#include "flowmini_ast_builder.h"

namespace flowmini::ast {

    namespace {

        bool is_main_token(const flowmini::Token& token) {
            return token.kind == TokenKind::KeywordMain ||
                   (token.kind == flowmini::TokenKind::Identifier && token.text == "main");
        }

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

        bool is_end_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::End;
        }

        bool is_top_level_boundry(const flowmini::Token& token) {
            return  token.kind == flowmini::TokenKind::KeywordFn ||
                    is_main_token(token) ||
                    token.kind == flowmini::TokenKind::KeywordMain ||
                    token.kind == flowmini::TokenKind::KeywordUnit ||
                    token.kind == flowmini::TokenKind::KeywordProgram ||
                    token.kind == flowmini::TokenKind::KeywordModule;
        }

        std::size_t skip_group(const std::vector<flowmini::Token>& tokens,
                        std::size_t i,
                        flowmini::TokenKind openKind,
                        flowmini::TokenKind closeKind
                        ) {
            if (i >= tokens.size() || tokens[i].kind != openKind) {
                return i;
            }

            std::size_t depth = 0;

            while (i < tokens.size() && !is_end_token(tokens[i])) {
                if (tokens[i].kind == openKind) { ++depth; }
                else if (tokens[i].kind == closeKind) {
                    if (depth == 0) {
                        return i;
                    }

                    --depth;

                    if (depth == 0) {
                        return i + 1;
                    }
                }

                ++i;
            }

            return i;
        }

        std::size_t skip_until_next_top_levelish_token(const std::vector<flowmini::Token>& tokens,std::size_t i) {
            while (i < tokens.size() && !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::LeftParen) {
                    i = skip_group(tokens, i, flowmini::TokenKind::LeftParen, flowmini::TokenKind::RightParen);
                    continue;
                }

                if (tokens[i].kind == flowmini::TokenKind::LeftBracket) {
                    i = skip_group(tokens, i, flowmini::TokenKind::LeftBracket, flowmini::TokenKind::RightBracket);
                    continue;
                }

                if (tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                    i = skip_group(tokens, i, flowmini::TokenKind::LeftBrace, flowmini::TokenKind::RightBrace);
                    continue;
                }

                if (is_top_level_boundry(tokens[i])) {
                    return i;
                }

                ++i;
            }

            return i;
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
            ++i;
        }

        // continue walking
        while (i<tokens.size() && !is_end_token(tokens[i])) {
            if (is_newline_token(tokens[i])) {
                ++i;
                continue;
            }

            if (tokens[i].kind == flowmini::TokenKind::KeywordFn) {
                FunctionDecl fn;
                fn.location = location_from_token(tokens[i]);

                ++i;

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Identifier) {
                    fn.name = tokens[i].text;
                    ++i;
                }

                module.source_unit.declarations.emplace_back(std::move(fn));
                i = skip_until_next_top_levelish_token(tokens,i);
                continue;
            }

            if (is_main_token(tokens[i])) {
                MainBlock mainBlock;
                mainBlock.location = location_from_token(tokens[i]);

                module.source_unit.declarations.emplace_back(std::move(mainBlock));

                ++i;
                i = skip_until_next_top_levelish_token(tokens, i);
                continue;
            }

            ++i;
        }
        return module;
    }

} // namespace flowmini::ast