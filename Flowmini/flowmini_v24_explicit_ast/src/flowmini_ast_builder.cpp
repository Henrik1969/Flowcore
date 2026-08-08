#include <cstddef>
#include "flowmini_ast_builder.h"

namespace flowmini::ast {

    SourceLocation location_from_token(const flowmini::Token& token) {
        const auto line = token.line < 0 ? 0 : token.line;
        const auto column = token.column < 0 ? 0 : token.column;

        return SourceLocation{
            static_cast<std::size_t>(line),
            static_cast<std::size_t>(column)
        };
    }

    namespace {
        bool is_end_token(const flowmini::Token& token);




        TypeRef make_named_type_ref(const flowmini::Token& token) {
            TypeRef type;
            type.kind = TypeRefKind::Named;
            type.name = token.text;
            type.raw_text = token.text;
            type.location = location_from_token(token);
            return type;
        }

        bool is_identifier_like_type_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Identifier;
        }

        std::size_t parse_function_signature(const std::vector<flowmini::Token>& tokens,
                                             std::size_t i,
                                             FunctionDecl& fn) {
            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::LeftParen) {
                return i;
            }

            ++i; // consume '('

            while (i < tokens.size() &&
                   tokens[i].kind != flowmini::TokenKind::RightParen &&
                   !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::Newline ||
                    tokens[i].kind == flowmini::TokenKind::Comma) {
                    ++i;
                    continue;
                }

                if (tokens[i].kind != flowmini::TokenKind::Identifier) {
                    ++i;
                    continue;
                }

                Parameter param;
                param.name = tokens[i].text;
                param.location = location_from_token(tokens[i]);
                ++i;

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Colon) {
                    ++i;

                    if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                        param.type = make_named_type_ref(tokens[i]);
                        ++i;
                    }
                }

                fn.parameters.push_back(std::move(param));

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Comma) {
                    ++i;
                }
            }

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::RightParen) {
                ++i; // consume ')'
            }

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Colon) {
                ++i;

                if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                    fn.return_type = make_named_type_ref(tokens[i]);
                    ++i;
                }
            }

            return i;
        }


        bool is_identifier_text(const flowmini::Token& token, const std::string& text) {
            return token.kind == flowmini::TokenKind::Identifier && token.text == text;
        }

        bool is_import_token(const flowmini::Token& token) {
            return is_identifier_text(token, "import");
        }

        bool is_type_token(const flowmini::Token& token) {
            return is_identifier_text(token, "type");
        }

        bool is_field_token(const flowmini::Token& token) {
            return is_identifier_text(token, "field");
        }

        bool is_refines_token(const flowmini::Token& token) {
            return is_identifier_text(token, "refines");
        }

        std::size_t skip_until_line_end(const std::vector<flowmini::Token>& tokens,
                                        std::size_t i) {
            while (i < tokens.size() &&
                   !is_end_token(tokens[i]) &&
                   tokens[i].kind != flowmini::TokenKind::Newline) {
                ++i;
            }
            return i;
        }

        std::size_t skip_balanced_brace_group(const std::vector<flowmini::Token>& tokens,
                                              std::size_t i) {
            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::LeftBrace) {
                return i;
            }

            std::size_t depth = 0;
            while (i < tokens.size() && !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                    ++depth;
                } else if (tokens[i].kind == flowmini::TokenKind::RightBrace) {
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

        std::size_t parse_import_declaration(const std::vector<flowmini::Token>& tokens,
                                             std::size_t i,
                                             AstModule& module) {
            ImportDecl importDecl;
            importDecl.location = location_from_token(tokens[i]);
            ++i; // consume import

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::String) {
                importDecl.module_name = tokens[i].text;
                ++i;
            }

            module.source_unit.declarations.emplace_back(std::move(importDecl));
            return skip_until_line_end(tokens, i);
        }

        std::size_t parse_record_fields(const std::vector<flowmini::Token>& tokens,
                                        std::size_t i,
                                        RecordDecl& recordDecl) {
            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::LeftBrace) {
                return i;
            }

            ++i; // consume '{'

            while (i < tokens.size() &&
                   tokens[i].kind != flowmini::TokenKind::RightBrace &&
                   !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::Newline ||
                    tokens[i].kind == flowmini::TokenKind::Comma) {
                    ++i;
                    continue;
                }

                if (!is_field_token(tokens[i])) {
                    ++i;
                    continue;
                }

                RecordField field;
                field.location = location_from_token(tokens[i]);
                ++i; // consume field

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Identifier) {
                    field.name = tokens[i].text;
                    ++i;
                }

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Colon) {
                    ++i;

                    if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                        field.type = make_named_type_ref(tokens[i]);
                        ++i;
                    }
                }

                recordDecl.fields.push_back(std::move(field));
            }

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::RightBrace) {
                ++i;
            }

            return i;
        }

        std::size_t parse_type_declaration(const std::vector<flowmini::Token>& tokens,
                                           std::size_t i,
                                           AstModule& module) {
            const SourceLocation typeLocation = location_from_token(tokens[i]);
            ++i; // consume type

            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::Identifier) {
                return skip_until_line_end(tokens, i);
            }

            const std::string typeName = tokens[i].text;
            const SourceLocation nameLocation = location_from_token(tokens[i]);
            ++i;

            if (i < tokens.size() && is_refines_token(tokens[i])) {
                TypeAliasDecl aliasDecl;
                aliasDecl.name = typeName;
                aliasDecl.location = typeLocation;
                ++i; // consume refines

                if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                    aliasDecl.target = make_named_type_ref(tokens[i]);
                    ++i;
                }

                module.source_unit.declarations.emplace_back(std::move(aliasDecl));

                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                    return skip_balanced_brace_group(tokens, i);
                }

                return skip_until_line_end(tokens, i);
            }

            RecordDecl recordDecl;
            recordDecl.name = typeName;
            recordDecl.location = nameLocation;

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                i = parse_record_fields(tokens, i, recordDecl);
            }

            module.source_unit.declarations.emplace_back(std::move(recordDecl));
            return i;
        }

        bool is_main_token(const flowmini::Token& token) {
            return token.kind == TokenKind::KeywordMain ||
                   (token.kind == flowmini::TokenKind::Identifier && token.text == "main");
        }

        bool is_newline_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Newline;
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


        std::size_t skip_nonsemantic_separators(const std::vector<flowmini::Token>& tokens,
                                                std::size_t i) {
            while (i < tokens.size() &&
                   (tokens[i].kind == flowmini::TokenKind::Newline ||
                    tokens[i].kind == flowmini::TokenKind::Comma)) {
                ++i;
            }

            return i;
        }


        bool is_return_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Identifier && token.text == "return";
        }

        bool is_break_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::KeywordBreak ||
                   is_identifier_text(token, "break");
        }

        bool is_continue_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::KeywordContinue ||
                   is_identifier_text(token, "continue");
        }

        Statement make_statement_shell(StatementKind kind, const flowmini::Token& token) {
            Statement statement;
            statement.kind = kind;
            statement.location = location_from_token(token);
            return statement;
        }




        std::size_t add_expression_placeholder(std::vector<Expression>& expressionPool,
                                               Statement& statement,
                                               const flowmini::Token& token) {
            Expression expression;
            expression.kind = ExpressionKind::Unknown;
            expression.location = location_from_token(token);

            expressionPool.push_back(std::move(expression));
            const auto expressionId = expressionPool.size() - 1;
            statement.expressions.push_back(expressionId);
            return expressionId;
        }

        bool has_expression_until_body_or_line_end(const std::vector<flowmini::Token>& tokens,
                                                   std::size_t i) {
            while (i < tokens.size() &&
                   !is_end_token(tokens[i]) &&
                   tokens[i].kind != flowmini::TokenKind::LeftBrace &&
                   tokens[i].kind != flowmini::TokenKind::RightBrace &&
                   tokens[i].kind != flowmini::TokenKind::Newline) {
                if (tokens[i].kind != flowmini::TokenKind::Comma) {
                    return true;
                }

                ++i;
            }

            return false;
        }

        bool has_expression_until_statement_boundary(const std::vector<flowmini::Token>& tokens,
                                                     std::size_t i) {
            while (i < tokens.size() &&
                   !is_end_token(tokens[i]) &&
                   tokens[i].kind != flowmini::TokenKind::Newline &&
                   tokens[i].kind != flowmini::TokenKind::RightBrace) {
                if (tokens[i].kind != flowmini::TokenKind::Comma) {
                    return true;
                }

                ++i;
            }

            return false;
        }

        bool is_typed_binding_start(const std::vector<flowmini::Token>& tokens,
                                    const std::size_t i) {
            return i + 2 < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::Identifier &&
                   tokens[i + 1].kind == flowmini::TokenKind::Colon &&
                   is_identifier_like_type_token(tokens[i + 2]);
        }

        std::size_t parse_typed_binding_statement_shell(const std::vector<flowmini::Token>& tokens,
                                                        std::size_t i,
                                                        std::vector<Statement>& body,
                                                        std::vector<Expression>& expressionPool) {
            Statement statement;
            statement.kind = StatementKind::Let;
            statement.location = location_from_token(tokens[i]);
            statement.name = tokens[i].text;

            i += 2; // consume name and ':'

            if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                statement.type = make_named_type_ref(tokens[i]);
                ++i;
            }

            statement.has_initializer =
                i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftParen;

            if (statement.has_initializer) {
                add_expression_placeholder(expressionPool, statement, tokens[i]);
            }

            body.push_back(std::move(statement));
            return i;
        }


        bool is_plain_assignment_start(const std::vector<flowmini::Token>& tokens,
                                       const std::size_t i) {
            return i + 1 < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::Identifier &&
                   tokens[i + 1].kind == flowmini::TokenKind::Equals;
        }

        std::size_t parse_plain_assignment_statement_shell(const std::vector<flowmini::Token>& tokens,
                                                           std::size_t i,
                                                           std::vector<Statement>& body,
                                                           std::vector<Expression>& expressionPool) {
            Statement statement;
            statement.kind = StatementKind::Assignment;
            statement.location = location_from_token(tokens[i]);
            statement.name = tokens[i].text;

            i += 2; // consume name and '='

            statement.has_value = has_expression_until_statement_boundary(tokens, i);

            if (statement.has_value && i < tokens.size()) {
                add_expression_placeholder(expressionPool, statement, tokens[i]);
            }

            body.push_back(std::move(statement));
            return i;
        }


        bool is_if_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::KeywordIf ||
                   is_identifier_text(token, "if");
        }

        bool is_while_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::KeywordWhile ||
                   is_identifier_text(token, "while");
        }

        std::size_t skip_until_body_block_or_line_end(const std::vector<flowmini::Token>& tokens,
                                                      std::size_t i) {
            while (i < tokens.size() &&
                   !is_end_token(tokens[i]) &&
                   tokens[i].kind != flowmini::TokenKind::LeftBrace &&
                   tokens[i].kind != flowmini::TokenKind::Newline) {
                ++i;
            }

            return i;
        }

        std::size_t parse_body_statement_shells(const std::vector<flowmini::Token>& tokens,
                                                std::size_t i,
                                                std::vector<Statement>& body,
                                                std::vector<Expression>& expressionPool);

        std::size_t parse_if_statement_shell(const std::vector<flowmini::Token>& tokens,
                                             std::size_t i,
                                             std::vector<Statement>& body,
                                             std::vector<Expression>& expressionPool) {
            Statement statement;
            statement.kind = StatementKind::If;
            statement.location = location_from_token(tokens[i]);

            ++i; // consume if

            statement.has_condition = has_expression_until_body_or_line_end(tokens, i);
            if (statement.has_condition && i < tokens.size()) {
                add_expression_placeholder(expressionPool, statement, tokens[i]);
            }
            i = skip_until_body_block_or_line_end(tokens, i);

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                statement.has_body = true;
                statement.body_location = location_from_token(tokens[i]);
                i = parse_body_statement_shells(tokens, i, statement.body, expressionPool);
            }

            body.push_back(std::move(statement));
            return i;
        }

        std::size_t parse_while_statement_shell(const std::vector<flowmini::Token>& tokens,
                                                std::size_t i,
                                                std::vector<Statement>& body,
                                                std::vector<Expression>& expressionPool) {
            Statement statement;
            statement.kind = StatementKind::While;
            statement.location = location_from_token(tokens[i]);

            ++i; // consume while

            statement.has_condition = has_expression_until_body_or_line_end(tokens, i);
            if (statement.has_condition && i < tokens.size()) {
                add_expression_placeholder(expressionPool, statement, tokens[i]);
            }
            i = skip_until_body_block_or_line_end(tokens, i);

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                statement.has_body = true;
                statement.body_location = location_from_token(tokens[i]);
                i = parse_body_statement_shells(tokens, i, statement.body, expressionPool);
            }

            body.push_back(std::move(statement));
            return i;
        }

        std::size_t parse_body_statement_shells(const std::vector<flowmini::Token>& tokens,
                                                std::size_t i,
                                                std::vector<Statement>& body,
                                                std::vector<Expression>& expressionPool) {
            i = skip_nonsemantic_separators(tokens, i);

            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::LeftBrace) {
                return i;
            }

            ++i; // consume '{'

            while (i < tokens.size() && !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::RightBrace) {
                    ++i;
                    return i;
                }

                if (tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                    i = skip_group(tokens, i, flowmini::TokenKind::LeftBrace, flowmini::TokenKind::RightBrace);
                    continue;
                }

                if (is_typed_binding_start(tokens, i)) {
                    i = parse_typed_binding_statement_shell(tokens, i, body, expressionPool);
                    continue;
                }

                if (is_if_token(tokens[i])) {
                    i = parse_if_statement_shell(tokens, i, body, expressionPool);
                    continue;
                }

                if (is_while_token(tokens[i])) {
                    i = parse_while_statement_shell(tokens, i, body, expressionPool);
                    continue;
                }

                if (is_plain_assignment_start(tokens, i)) {
                    i = parse_plain_assignment_statement_shell(tokens, i, body, expressionPool);
                    continue;
                }

                if (is_return_token(tokens[i])) {
                    Statement statement = make_statement_shell(StatementKind::Return, tokens[i]);
                    statement.has_value = has_expression_until_statement_boundary(tokens, i + 1);
                    if (statement.has_value && i + 1 < tokens.size()) {
                        add_expression_placeholder(expressionPool, statement, tokens[i + 1]);
                    }
                    body.push_back(std::move(statement));
                    ++i;
                    continue;
                }

                if (is_break_token(tokens[i])) {
                    body.push_back(make_statement_shell(StatementKind::Break, tokens[i]));
                    ++i;
                    continue;
                }

                if (is_continue_token(tokens[i])) {
                    body.push_back(make_statement_shell(StatementKind::Continue, tokens[i]));
                    ++i;
                    continue;
                }

                ++i;
            }

            return i;
        }

        std::size_t mark_body_container(const std::vector<flowmini::Token>& tokens,
                                        std::size_t i,
                                        bool& hasBody,
                                        SourceLocation& bodyLocation,
                                        std::vector<Statement>& body,
                                        std::vector<Expression>& expressionPool) {
            i = skip_nonsemantic_separators(tokens, i);

            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::LeftBrace) {
                return i;
            }

            hasBody = true;
            bodyLocation = location_from_token(tokens[i]);

            ++i; // consume '{'
            std::size_t braceDepth = 1;

            while (i < tokens.size() && !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                    ++i;
                    continue;
                }

                if (tokens[i].kind == flowmini::TokenKind::RightBrace) {
                    --braceDepth;
                    ++i;

                    if (braceDepth == 0) {
                        return i;
                    }

                    continue;
                }

                if (braceDepth == 1) {
                    if (is_typed_binding_start(tokens, i)) {
                        i = parse_typed_binding_statement_shell(tokens, i, body, expressionPool);
                        continue;
                    }

                    if (is_if_token(tokens[i])) {
                        i = parse_if_statement_shell(tokens, i, body, expressionPool);
                        continue;
                    }

                    if (is_while_token(tokens[i])) {
                        i = parse_while_statement_shell(tokens, i, body, expressionPool);
                        continue;
                    }

                    if (is_plain_assignment_start(tokens, i)) {
                        i = parse_plain_assignment_statement_shell(tokens, i, body, expressionPool);
                        continue;
                    }

                    if (is_return_token(tokens[i])) {
                        body.push_back(make_statement_shell(StatementKind::Return, tokens[i]));
                        ++i;
                        continue;
                    }

                    if (is_break_token(tokens[i])) {
                        body.push_back(make_statement_shell(StatementKind::Break, tokens[i]));
                        ++i;
                        continue;
                    }

                    if (is_continue_token(tokens[i])) {
                        body.push_back(make_statement_shell(StatementKind::Continue, tokens[i]));
                        ++i;
                        continue;
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
            module.source_unit.location = location_from_token(tokens[i]);
            return module;
        }

        module.source_unit.location = location_from_token(tokens[i]) ;

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

            if (is_import_token(tokens[i])) {
                i = parse_import_declaration(tokens, i, module);
                continue;
            }

            if (is_type_token(tokens[i])) {
                i = parse_type_declaration(tokens, i, module);
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

                i = parse_function_signature(tokens, i, fn);
                i = mark_body_container(tokens, i, fn.has_body, fn.body_location, fn.body, module.expression_pool);

                module.source_unit.declarations.emplace_back(std::move(fn));
                i = skip_until_next_top_levelish_token(tokens, i);
                continue;
            }

            if (is_main_token(tokens[i])) {
                MainBlock mainBlock;
                mainBlock.location = location_from_token(tokens[i]);

                ++i;
                i = mark_body_container(tokens, i, mainBlock.has_body, mainBlock.body_location, mainBlock.body, module.expression_pool);

                module.source_unit.declarations.emplace_back(std::move(mainBlock));

                i = skip_until_next_top_levelish_token(tokens, i);
                continue;
            }

            ++i;
        }
        return module;
    }

} // namespace flowmini::ast