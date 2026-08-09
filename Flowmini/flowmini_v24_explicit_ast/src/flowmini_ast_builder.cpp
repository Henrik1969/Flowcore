#include <cstddef>
#include <memory>
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
        constexpr std::size_t max_expression_population_depth = 64;

        bool is_end_token(const flowmini::Token& token);




        bool is_identifier_like_type_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Identifier;
        }

        std::vector<std::string> parse_qualified_type_name(
            const std::vector<flowmini::Token>& tokens,
            std::size_t& i) {
            std::vector<std::string> segments;
            if (i >= tokens.size() || !is_identifier_like_type_token(tokens[i])) {
                return segments;
            }

            segments.push_back(tokens[i].text);
            ++i;
            while (i + 1 < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::Dot &&
                   is_identifier_like_type_token(tokens[i + 1])) {
                segments.push_back(tokens[i + 1].text);
                i += 2;
            }
            return segments;
        }

        std::string render_token_slice(const std::vector<flowmini::Token>& tokens,
                                       const std::size_t begin,
                                       const std::size_t end) {
            std::string text;
            for (std::size_t i = begin; i < end; ++i) {
                text += tokens[i].text;
            }
            return text;
        }

        std::string consume_type_ref_spelling(const std::vector<flowmini::Token>& tokens,
                                              std::size_t& i) {
            const auto begin = i;
            parse_qualified_type_name(tokens, i);

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Less) {
                std::size_t angleDepth = 0;
                do {
                    if (tokens[i].kind == flowmini::TokenKind::Less) {
                        ++angleDepth;
                    } else if (tokens[i].kind == flowmini::TokenKind::Greater) {
                        --angleDepth;
                    }
                    ++i;
                } while (i < tokens.size() && angleDepth > 0 && !is_end_token(tokens[i]));
            }

            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBracket) {
                std::size_t bracketDepth = 0;
                do {
                    if (tokens[i].kind == flowmini::TokenKind::LeftBracket) {
                        ++bracketDepth;
                    } else if (tokens[i].kind == flowmini::TokenKind::RightBracket) {
                        --bracketDepth;
                    }
                    ++i;
                } while (i < tokens.size() && bracketDepth > 0 && !is_end_token(tokens[i]));
            }

            return render_token_slice(tokens, begin, i);
        }

        TypeRef parse_type_ref(const std::vector<flowmini::Token>& tokens,
                               std::size_t& i,
                               const std::size_t depth = 0) {
            TypeRef type;
            if (i >= tokens.size() || !is_identifier_like_type_token(tokens[i])) {
                return type;
            }

            type.location = location_from_token(tokens[i]);
            if (depth >= 64) {
                type.payload = UnknownTypeRef{consume_type_ref_spelling(tokens, i)};
                return type;
            }
            const auto nameSegments = parse_qualified_type_name(tokens, i);
            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::Less) {
                type.payload = NamedTypeRef{nameSegments};
                return type;
            }

            ++i; // consume '<'
            std::vector<TypeRef> arguments;
            while (i < tokens.size() &&
                   tokens[i].kind != flowmini::TokenKind::Greater &&
                   !is_end_token(tokens[i])) {
                if (tokens[i].kind == flowmini::TokenKind::Comma ||
                    tokens[i].kind == flowmini::TokenKind::Newline) {
                    ++i;
                    continue;
                }

                if (!is_identifier_like_type_token(tokens[i])) {
                    ++i;
                    continue;
                }

                arguments.push_back(parse_type_ref(tokens, i, depth + 1));
            }
            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::Greater) {
                ++i;
            }

            const bool isArray = nameSegments.size() == 1 &&
                                 nameSegments.front() == "array" &&
                                 arguments.size() == 1;
            if (!isArray) {
                type.payload = GenericTypeRef{nameSegments, std::move(arguments)};
                return type;
            }

            ArrayTypeRef array;
            array.element_type = std::make_shared<const TypeRef>(std::move(arguments.front()));
            if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftBracket) {
                ++i; // consume '['
                while (i < tokens.size() &&
                       tokens[i].kind != flowmini::TokenKind::RightBracket &&
                       !is_end_token(tokens[i])) {
                    if (tokens[i].kind == flowmini::TokenKind::Comma ||
                        tokens[i].kind == flowmini::TokenKind::Newline) {
                        ++i;
                        continue;
                    }

                    const auto extentStart = i;
                    const auto extentLocation = location_from_token(tokens[i]);
                    std::size_t parenDepth = 0;
                    while (i < tokens.size() && !is_end_token(tokens[i])) {
                        if (tokens[i].kind == flowmini::TokenKind::LeftParen) {
                            ++parenDepth;
                        } else if (tokens[i].kind == flowmini::TokenKind::RightParen && parenDepth > 0) {
                            --parenDepth;
                        } else if (parenDepth == 0 &&
                                   (tokens[i].kind == flowmini::TokenKind::Comma ||
                                    tokens[i].kind == flowmini::TokenKind::RightBracket)) {
                            break;
                        }
                        ++i;
                    }

                    array.extents.push_back(ArrayExtent{
                        render_token_slice(tokens, extentStart, i),
                        extentLocation
                    });
                }
                if (i < tokens.size() && tokens[i].kind == flowmini::TokenKind::RightBracket) {
                    ++i;
                }
            }
            type.payload = std::move(array);
            return type;
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
                        param.type = parse_type_ref(tokens, i);
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

            if (i < tokens.size() &&
                (tokens[i].kind == flowmini::TokenKind::Colon ||
                 tokens[i].kind == flowmini::TokenKind::PlaceArrow)) {
                ++i;

                if (i < tokens.size() && is_identifier_like_type_token(tokens[i])) {
                    fn.return_type = parse_type_ref(tokens, i);
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
                        field.type = parse_type_ref(tokens, i);
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
                    aliasDecl.target = parse_type_ref(tokens, i);
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





        bool token_text_contains(const flowmini::Token& token, const char needle) {
            return token.text.find(needle) != std::string::npos;
        }

        ExpressionKind classify_expression_token(const flowmini::Token& token) {
            switch (token.kind) {
                case flowmini::TokenKind::Identifier:
                    return ExpressionKind::Identifier;

                case flowmini::TokenKind::Number:
                    return token_text_contains(token, '.')
                        ? ExpressionKind::FloatLiteral
                        : ExpressionKind::IntegerLiteral;

                case flowmini::TokenKind::String:
                    return ExpressionKind::StringLiteral;

                case flowmini::TokenKind::KeywordTrue:
                case flowmini::TokenKind::KeywordFalse:
                    return ExpressionKind::BoolLiteral;

                default:
                    break;
            }

            if (token.text == "true" || token.text == "false") {
                return ExpressionKind::BoolLiteral;
            }

            return ExpressionKind::Unknown;
        }

        Expression::Payload make_leaf_payload(const flowmini::Token& token) {
            switch (classify_expression_token(token)) {
                case ExpressionKind::Identifier:     return IdentifierExpr{token.text};
                case ExpressionKind::IntegerLiteral: return IntegerLiteralExpr{token.text};
                case ExpressionKind::FloatLiteral:   return FloatLiteralExpr{token.text};
                case ExpressionKind::StringLiteral:  return StringLiteralExpr{token.text};
                case ExpressionKind::BoolLiteral:    return BoolLiteralExpr{token.text};
                default:                             return UnknownExpr{token.text};
            }
        }

        std::size_t add_expression_placeholder(std::vector<Expression>& expressionPool,
                                               Statement& statement,
                                               const flowmini::Token& token,
                                               const bool attachToLegacyProjection = true) {
            Expression expression;
            expression.location = location_from_token(token);
            expression.payload = make_leaf_payload(token);

            expressionPool.push_back(std::move(expression));
            const auto expressionId = expressionPool.size() - 1;
            if (attachToLegacyProjection) {
                statement.expressions.push_back(expressionId);
            }
            return expressionId;
        }



        enum class BinaryOperatorAssociativity {
            Left,
            NonAssociative
        };

        struct BinaryOperatorBinding {
            int precedence = 0;
            BinaryOperatorAssociativity associativity = BinaryOperatorAssociativity::Left;
        };

        bool is_binary_operator_token(const flowmini::Token& token) {
            switch (token.kind) {
                case flowmini::TokenKind::Plus:
                case flowmini::TokenKind::Minus:
                case flowmini::TokenKind::Star:
                case flowmini::TokenKind::Slash:
                case flowmini::TokenKind::Percent:
                case flowmini::TokenKind::Less:
                case flowmini::TokenKind::Greater:
                case flowmini::TokenKind::LessEqual:
                case flowmini::TokenKind::GreaterEqual:
                case flowmini::TokenKind::EqualEqual:
                case flowmini::TokenKind::BangEqual:
                    return true;

                default:
                    break;
            }

            return false;
        }

        BinaryOperatorBinding binary_operator_binding(const flowmini::Token& token) {
            switch (token.kind) {
                case flowmini::TokenKind::Less:
                case flowmini::TokenKind::Greater:
                case flowmini::TokenKind::LessEqual:
                case flowmini::TokenKind::GreaterEqual:
                case flowmini::TokenKind::EqualEqual:
                case flowmini::TokenKind::BangEqual:
                    return {10, BinaryOperatorAssociativity::NonAssociative};

                case flowmini::TokenKind::Plus:
                case flowmini::TokenKind::Minus:
                    return {20, BinaryOperatorAssociativity::Left};

                case flowmini::TokenKind::Star:
                case flowmini::TokenKind::Slash:
                case flowmini::TokenKind::Percent:
                    return {30, BinaryOperatorAssociativity::Left};

                default:
                    break;
            }

            return {};
        }

        bool is_expression_boundary_token(const flowmini::Token& token) {
            return token.kind == flowmini::TokenKind::Newline ||
                   token.kind == flowmini::TokenKind::RightBrace ||
                   token.kind == flowmini::TokenKind::End;
        }

        bool is_prefix_minus_at(const std::vector<flowmini::Token>& tokens,
                                const std::size_t expressionStart,
                                const std::size_t i) {
            if (i >= tokens.size() || tokens[i].kind != flowmini::TokenKind::Minus) {
                return false;
            }

            if (i == expressionStart) {
                return true;
            }

            const auto& previous = tokens[i - 1];
            return is_binary_operator_token(previous) ||
                   previous.kind == flowmini::TokenKind::LeftParen ||
                   previous.kind == flowmini::TokenKind::LeftBracket ||
                   previous.kind == flowmini::TokenKind::Comma ||
                   previous.kind == flowmini::TokenKind::Colon;
        }

        std::size_t find_binary_operator_split(const std::vector<flowmini::Token>& tokens,
                                               std::size_t i) {
            const auto expressionStart = i;

            std::size_t parenDepth = 0;
            std::size_t bracketDepth = 0;
            std::size_t selectedOperator = tokens.size();
            int selectedPrecedence = 0;

            while (i < tokens.size() && !is_expression_boundary_token(tokens[i])) {
                const auto& token = tokens[i];

                if (token.kind == flowmini::TokenKind::LeftBrace && parenDepth == 0 && bracketDepth == 0) {
                    return selectedOperator;
                }

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                    ++i;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightParen) {
                    if (parenDepth == 0) {
                        return tokens.size();
                    }

                    --parenDepth;
                    ++i;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                    ++i;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBracket) {
                    if (bracketDepth == 0) {
                        return tokens.size();
                    }

                    --bracketDepth;
                    ++i;
                    continue;
                }

                if (is_prefix_minus_at(tokens, expressionStart, i)) {
                    ++i;
                    continue;
                }

                if (parenDepth == 0 && bracketDepth == 0 && is_binary_operator_token(token)) {
                    const auto binding = binary_operator_binding(token);

                    const bool isWeaker = selectedOperator == tokens.size() ||
                                          binding.precedence < selectedPrecedence;
                    const bool replacesEqualLeftAssociative =
                        binding.precedence == selectedPrecedence &&
                        binding.associativity == BinaryOperatorAssociativity::Left;

                    if (isWeaker || replacesEqualLeftAssociative) {
                        selectedOperator = i;
                        selectedPrecedence = binding.precedence;
                    }
                }

                ++i;
            }

            return selectedOperator;
        }

        bool expression_starts_unary(const std::vector<flowmini::Token>& tokens,
                                     const std::size_t i) {
            return i < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::Minus;
        }

        bool expression_starts_float_literal(const std::vector<flowmini::Token>& tokens,
                                             const std::size_t i) {
            return i + 2 < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::Number &&
                   tokens[i + 1].kind == flowmini::TokenKind::Dot &&
                   tokens[i + 2].kind == flowmini::TokenKind::Number;
        }



        bool expression_starts_list_literal(const std::vector<flowmini::Token>& tokens,
                                            const std::size_t i) {
            return i < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::LeftBracket;
        }

        bool expression_starts_record_literal(const std::vector<flowmini::Token>& tokens,
                                              const std::size_t i) {
            return i < tokens.size() &&
                   tokens[i].kind == flowmini::TokenKind::LeftBrace;
        }

        struct PostfixDecomposition {
            ExpressionKind kind = ExpressionKind::Unknown;
            std::size_t operator_index = 0;
            std::size_t end_index = 0;
        };

        PostfixDecomposition find_outermost_postfix(const std::vector<flowmini::Token>& tokens,
                                                    const std::size_t expressionStart) {
            PostfixDecomposition selected;
            std::size_t parenDepth = 0;
            std::size_t bracketDepth = 0;
            std::size_t braceDepth = 0;

            for (std::size_t i = expressionStart; i < tokens.size(); ++i) {
                const auto& token = tokens[i];
                const bool atTopLevel = parenDepth == 0 && bracketDepth == 0 && braceDepth == 0;

                if (atTopLevel && i > expressionStart && token.kind == flowmini::TokenKind::LeftParen) {
                    selected = {ExpressionKind::Call, i, tokens.size()};
                    ++parenDepth;
                    continue;
                }

                if (atTopLevel && i > expressionStart && token.kind == flowmini::TokenKind::LeftBracket) {
                    selected = {ExpressionKind::Index, i, tokens.size()};
                    ++bracketDepth;
                    continue;
                }

                if (atTopLevel && token.kind == flowmini::TokenKind::Dot &&
                    i + 1 < tokens.size() &&
                    tokens[i + 1].kind == flowmini::TokenKind::Identifier) {
                    selected = {ExpressionKind::FieldAccess, i, i + 1};
                    ++i;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                } else if (token.kind == flowmini::TokenKind::RightParen && parenDepth > 0) {
                    --parenDepth;
                    if (parenDepth == 0 && selected.kind == ExpressionKind::Call) {
                        selected.end_index = i;
                    }
                } else if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                } else if (token.kind == flowmini::TokenKind::RightBracket && bracketDepth > 0) {
                    --bracketDepth;
                    if (bracketDepth == 0 && selected.kind == ExpressionKind::Index) {
                        selected.end_index = i;
                    }
                } else if (token.kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                } else if (token.kind == flowmini::TokenKind::RightBrace && braceDepth > 0) {
                    --braceDepth;
                }
            }

            if (selected.kind == ExpressionKind::Unknown ||
                selected.end_index != tokens.size() - 1) {
                return {};
            }

            return selected;
        }

    std::size_t find_matching_right_paren(const std::vector<flowmini::Token>& tokens,
                                          const std::size_t leftParenIndex) {
        if (leftParenIndex >= tokens.size() ||
            tokens[leftParenIndex].kind != flowmini::TokenKind::LeftParen) {
            return tokens.size();
        }

        std::size_t depth = 0;

        for (std::size_t i = leftParenIndex; i < tokens.size(); ++i) {
            if (tokens[i].kind == flowmini::TokenKind::LeftParen) {
                ++depth;
                continue;
            }

            if (tokens[i].kind == flowmini::TokenKind::RightParen) {
                if (depth == 0) {
                    return tokens.size();
                }

                --depth;

                if (depth == 0) {
                    return i;
                }
            }

            if (is_end_token(tokens[i]) || tokens[i].kind == flowmini::TokenKind::Newline) {
                return tokens.size();
            }
        }

        return tokens.size();
    }

    bool is_expression_separator_token(const flowmini::Token& token) {
        return token.kind == flowmini::TokenKind::Comma ||
               token.kind == flowmini::TokenKind::Newline;
    }

    std::size_t first_argument_token(const std::vector<flowmini::Token>& tokens,
                                     std::size_t begin,
                                     const std::size_t end) {
        while (begin < end && is_expression_separator_token(tokens[begin])) {
            ++begin;
        }

        return begin;
    }

    std::vector<flowmini::Token> expression_token_slice(const std::vector<flowmini::Token>& tokens,
                                                        const std::size_t begin) {
        std::vector<flowmini::Token> result;
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = begin; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            const bool atTopLevel = parenDepth == 0 && bracketDepth == 0 && braceDepth == 0;

            if (atTopLevel && is_expression_boundary_token(token)) {
                break;
            }

            if (atTopLevel &&
                (token.kind == flowmini::TokenKind::RightParen ||
                 token.kind == flowmini::TokenKind::RightBracket ||
                 token.kind == flowmini::TokenKind::Comma)) {
                break;
            }

            if (atTopLevel && token.kind == flowmini::TokenKind::LeftBrace && i != begin) {
                break;
            }

            result.push_back(token);

            if (token.kind == flowmini::TokenKind::LeftParen) {
                ++parenDepth;
            } else if (token.kind == flowmini::TokenKind::RightParen && parenDepth > 0) {
                --parenDepth;
            } else if (token.kind == flowmini::TokenKind::LeftBracket) {
                ++bracketDepth;
            } else if (token.kind == flowmini::TokenKind::RightBracket && bracketDepth > 0) {
                --bracketDepth;
            } else if (token.kind == flowmini::TokenKind::LeftBrace) {
                ++braceDepth;
            } else if (token.kind == flowmini::TokenKind::RightBrace && braceDepth > 0) {
                --braceDepth;
            }
        }

        return result;
    }

    std::vector<flowmini::Token> strip_enclosing_parentheses(std::vector<flowmini::Token> tokens) {
        while (tokens.size() >= 2 &&
               tokens.front().kind == flowmini::TokenKind::LeftParen &&
               find_matching_right_paren(tokens, 0) == tokens.size() - 1) {
            tokens = std::vector<flowmini::Token>{tokens.begin() + 1, tokens.end() - 1};
        }

        return tokens;
    }

    Expression make_shallow_expression_from_tokens(const std::vector<flowmini::Token>& tokens,
                                                   const std::size_t i) {
        Expression expression;

        if (tokens.empty() || i >= tokens.size()) {
            return expression;
        }

        const auto& token = tokens[i];
        expression.location = location_from_token(token);

        const auto binaryOperatorIndex = find_binary_operator_split(tokens, i);
        if (binaryOperatorIndex < tokens.size()) {
            expression.payload = BinaryExpr{tokens[binaryOperatorIndex].text, std::nullopt, std::nullopt};
        } else if (expression_starts_float_literal(tokens, i)) {
            expression.payload = FloatLiteralExpr{
                tokens[i].text + tokens[i + 1].text + tokens[i + 2].text
            };
        } else if (expression_starts_unary(tokens, i)) {
            expression.payload = UnaryExpr{token.text, std::nullopt};
        } else if (const auto postfix = find_outermost_postfix(tokens, i);
                   postfix.kind != ExpressionKind::Unknown) {
            if (postfix.kind == ExpressionKind::Call) {
                expression.payload = CallExpr{};
            } else if (postfix.kind == ExpressionKind::Index) {
                expression.payload = IndexExpr{};
            } else {
                expression.payload = FieldAccessExpr{
                    std::nullopt,
                    tokens[postfix.end_index].text
                };
            }
        } else if (expression_starts_list_literal(tokens, i)) {
            expression.payload = ListLiteralExpr{};
        } else if (expression_starts_record_literal(tokens, i)) {
            expression.payload = RecordLiteralExpr{};
        } else {
            expression.payload = make_leaf_payload(token);
        }

        return expression;
    }

    void populate_expression_children(std::vector<Expression>& expressionPool,
                                      std::size_t expressionId,
                                      const std::vector<flowmini::Token>& tokens,
                                      std::size_t expressionStart,
                                      std::size_t depth);

    std::optional<std::size_t> append_populated_expression_child(
                                           std::vector<Expression>& expressionPool,
                                           const std::size_t parentExpressionId,
                                           const std::vector<flowmini::Token>& childTokens,
                                           const std::size_t depth) {
        auto normalizedTokens = strip_enclosing_parentheses(childTokens);
        if (normalizedTokens.empty()) {
            return std::nullopt;
        }

        Expression child = make_shallow_expression_from_tokens(normalizedTokens, 0);

        expressionPool.push_back(std::move(child));
        const auto childId = expressionPool.size() - 1;

        auto& parent = expressionPool[parentExpressionId];
        if (auto* value = std::get_if<UnaryExpr>(&parent.payload)) {
            value->operand = childId;
        } else if (auto* value = std::get_if<BinaryExpr>(&parent.payload)) {
            if (!value->left) { value->left = childId; }
            else { value->right = childId; }
        } else if (auto* value = std::get_if<CallExpr>(&parent.payload)) {
            if (!value->base) { value->base = childId; }
            else { value->arguments.push_back(childId); }
        } else if (auto* value = std::get_if<IndexExpr>(&parent.payload)) {
            if (!value->base) { value->base = childId; }
            else { value->indexes.push_back(childId); }
        } else if (auto* value = std::get_if<FieldAccessExpr>(&parent.payload)) {
            value->base = childId;
        } else if (auto* value = std::get_if<ListLiteralExpr>(&parent.payload)) {
            value->elements.push_back(childId);
        }

        populate_expression_children(expressionPool, childId, normalizedTokens, 0, depth + 1);
        return childId;
    }

    void append_call_argument_child(std::vector<Expression>& expressionPool,
                                    const std::size_t callExpressionId,
                                    const std::vector<flowmini::Token>& tokens,
                                    const std::size_t begin,
                                    const std::size_t end,
                                    const std::size_t depth) {
        const auto argStart = first_argument_token(tokens, begin, end);
        if (argStart >= end) {
            return;
        }

        std::vector<flowmini::Token> argumentTokens;
        argumentTokens.reserve(end - argStart);

        for (std::size_t i = argStart; i < end; ++i) {
            argumentTokens.push_back(tokens[i]);
        }

        append_populated_expression_child(expressionPool,
                                          callExpressionId,
                                          argumentTokens,
                                          depth);
    }

    void populate_call_argument_children(std::vector<Expression>& expressionPool,
                                         const std::size_t callExpressionId,
                                         const std::vector<flowmini::Token>& tokens,
                                         const std::size_t expressionStart,
                                         const std::size_t depth) {
        const auto postfix = find_outermost_postfix(tokens, expressionStart);
        if (postfix.kind != ExpressionKind::Call ||
            postfix.operator_index <= expressionStart) {
            return;
        }

        append_populated_expression_child(
            expressionPool,
            callExpressionId,
            std::vector<flowmini::Token>{tokens.begin() + expressionStart,
                                         tokens.begin() + postfix.operator_index},
            depth);

        const auto closeParenIndex = postfix.end_index;
        if (closeParenIndex <= postfix.operator_index + 1) {
            return;
        }

        std::size_t argBegin = postfix.operator_index + 1;
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = argBegin; i <= closeParenIndex; ++i) {
            const bool atEnd = i == closeParenIndex;

            if (!atEnd) {
                const auto& token = tokens[i];

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightParen) {
                    if (parenDepth > 0) {
                        --parenDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBracket) {
                    if (bracketDepth > 0) {
                        --bracketDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBrace) {
                    if (braceDepth > 0) {
                        --braceDepth;
                    }
                    continue;
                }

                const bool atTopLevelComma =
                    token.kind == flowmini::TokenKind::Comma &&
                    parenDepth == 0 &&
                    bracketDepth == 0 &&
                    braceDepth == 0;

                if (!atTopLevelComma) {
                    continue;
                }
            }

            append_call_argument_child(expressionPool,
                                       callExpressionId,
                                       tokens,
                                       argBegin,
                                       i,
                                       depth);
            argBegin = i + 1;
        }
    }


    
    void append_binary_operand_child(std::vector<Expression>& expressionPool,
                                     const std::size_t binaryExpressionId,
                                     const std::vector<flowmini::Token>& tokens,
                                     const std::size_t begin,
                                     const std::size_t end,
                                     const std::size_t depth) {
        std::size_t operandStart = begin;

        while (operandStart < end &&
               (tokens[operandStart].kind == flowmini::TokenKind::Comma ||
                tokens[operandStart].kind == flowmini::TokenKind::Newline)) {
            ++operandStart;
        }

        if (operandStart >= end) {
            return;
        }

        std::vector<flowmini::Token> operandTokens;
        operandTokens.reserve(end - operandStart);

        for (std::size_t i = operandStart; i < end; ++i) {
            operandTokens.push_back(tokens[i]);
        }

        append_populated_expression_child(expressionPool,
                                          binaryExpressionId,
                                          operandTokens,
                                          depth);
    }

    void populate_binary_operand_children(std::vector<Expression>& expressionPool,
                                          const std::size_t binaryExpressionId,
                                          const std::vector<flowmini::Token>& tokens,
                                          const std::size_t expressionStart,
                                          const std::size_t depth) {
        const auto binaryOperatorIndex = find_binary_operator_split(tokens, expressionStart);
        if (binaryOperatorIndex == tokens.size()) {
            return;
        }

        if (binaryOperatorIndex <= expressionStart) {
            return;
        }

        append_binary_operand_child(expressionPool,
                                    binaryExpressionId,
                                    tokens,
                                    expressionStart,
                                    binaryOperatorIndex,
                                    depth);

        append_binary_operand_child(expressionPool,
                                    binaryExpressionId,
                                    tokens,
                                    binaryOperatorIndex + 1,
                                    tokens.size(),
                                    depth);
    }


    void populate_unary_operand_child(std::vector<Expression>& expressionPool,
                                      const std::size_t unaryExpressionId,
                                      const std::vector<flowmini::Token>& tokens,
                                      const std::size_t unaryIndex,
                                      const std::size_t depth) {
        if (unaryIndex + 1 >= tokens.size()) {
            return;
        }

        const auto operandIndex = unaryIndex + 1;

        if (is_expression_boundary_token(tokens[operandIndex]) ||
            tokens[operandIndex].kind == flowmini::TokenKind::Comma ||
            tokens[operandIndex].kind == flowmini::TokenKind::RightParen ||
            tokens[operandIndex].kind == flowmini::TokenKind::RightBracket ||
            tokens[operandIndex].kind == flowmini::TokenKind::RightBrace) {
            return;
        }

        std::vector<flowmini::Token> operandTokens{tokens.begin() + operandIndex, tokens.end()};

        if (operandTokens.empty()) {
            return;
        }

        append_populated_expression_child(expressionPool,
                                          unaryExpressionId,
                                          operandTokens,
                                          depth);
    }


    std::size_t find_matching_right_bracket(const std::vector<flowmini::Token>& tokens,
                                            const std::size_t leftBracketIndex) {
        if (leftBracketIndex >= tokens.size() ||
            tokens[leftBracketIndex].kind != flowmini::TokenKind::LeftBracket) {
            return tokens.size();
        }

        std::size_t depth = 0;

        for (std::size_t i = leftBracketIndex; i < tokens.size(); ++i) {
            if (tokens[i].kind == flowmini::TokenKind::LeftBracket) {
                ++depth;
                continue;
            }

            if (tokens[i].kind == flowmini::TokenKind::RightBracket) {
                if (depth == 0) {
                    return tokens.size();
                }

                --depth;

                if (depth == 0) {
                    return i;
                }
            }

            if (is_end_token(tokens[i]) || tokens[i].kind == flowmini::TokenKind::Newline) {
                return tokens.size();
            }
        }

        return tokens.size();
    }

    void append_index_child(std::vector<Expression>& expressionPool,
                            const std::size_t indexExpressionId,
                            const std::vector<flowmini::Token>& tokens,
                            const std::size_t begin,
                            const std::size_t end,
                            const std::size_t depth) {
        std::size_t childStart = begin;

        while (childStart < end &&
               (tokens[childStart].kind == flowmini::TokenKind::Comma ||
                tokens[childStart].kind == flowmini::TokenKind::Newline)) {
            ++childStart;
        }

        if (childStart >= end) {
            return;
        }

        std::vector<flowmini::Token> childTokens;
        childTokens.reserve(end - childStart);

        for (std::size_t i = childStart; i < end; ++i) {
            childTokens.push_back(tokens[i]);
        }

        append_populated_expression_child(expressionPool,
                                          indexExpressionId,
                                          childTokens,
                                          depth);
    }

    void populate_index_expression_children(std::vector<Expression>& expressionPool,
                                            const std::size_t indexExpressionId,
                                            const std::vector<flowmini::Token>& tokens,
                                            const std::size_t expressionStart,
                                            const std::size_t depth) {
        const auto postfix = find_outermost_postfix(tokens, expressionStart);
        if (postfix.kind != ExpressionKind::Index ||
            postfix.operator_index <= expressionStart) {
            return;
        }

        const auto leftBracketIndex = postfix.operator_index;
        const auto rightBracketIndex = postfix.end_index;

        append_index_child(expressionPool,
                           indexExpressionId,
                           tokens,
                           expressionStart,
                           leftBracketIndex,
                           depth);

        if (rightBracketIndex <= leftBracketIndex + 1) {
            return;
        }

        std::size_t indexBegin = leftBracketIndex + 1;
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = indexBegin; i <= rightBracketIndex; ++i) {
            const bool atEnd = i == rightBracketIndex;

            if (!atEnd) {
                const auto& token = tokens[i];

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightParen) {
                    if (parenDepth > 0) {
                        --parenDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBracket) {
                    if (bracketDepth > 0) {
                        --bracketDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBrace) {
                    if (braceDepth > 0) {
                        --braceDepth;
                    }
                    continue;
                }

                const bool atTopLevelComma =
                    token.kind == flowmini::TokenKind::Comma &&
                    parenDepth == 0 &&
                    bracketDepth == 0 &&
                    braceDepth == 0;

                if (!atTopLevelComma) {
                    continue;
                }
            }

            append_index_child(expressionPool,
                               indexExpressionId,
                               tokens,
                               indexBegin,
                               i,
                               depth);
            indexBegin = i + 1;
        }
    }


    void populate_field_access_children(std::vector<Expression>& expressionPool,
                                        const std::size_t fieldExpressionId,
                                        const std::vector<flowmini::Token>& tokens,
                                        const std::size_t expressionStart,
                                        const std::size_t depth) {
        const auto postfix = find_outermost_postfix(tokens, expressionStart);
        if (postfix.kind != ExpressionKind::FieldAccess ||
            postfix.operator_index <= expressionStart) {
            return;
        }

        append_populated_expression_child(
            expressionPool,
            fieldExpressionId,
            std::vector<flowmini::Token>{tokens.begin() + expressionStart,
                                         tokens.begin() + postfix.operator_index},
            depth);
    }


    void append_list_literal_element_child(std::vector<Expression>& expressionPool,
                                           const std::size_t listExpressionId,
                                           const std::vector<flowmini::Token>& tokens,
                                           const std::size_t begin,
                                           const std::size_t end,
                                           const std::size_t depth) {
        std::size_t elementStart = begin;

        while (elementStart < end &&
               (tokens[elementStart].kind == flowmini::TokenKind::Comma ||
                tokens[elementStart].kind == flowmini::TokenKind::Newline)) {
            ++elementStart;
        }

        if (elementStart >= end) {
            return;
        }

        std::vector<flowmini::Token> elementTokens;
        elementTokens.reserve(end - elementStart);

        for (std::size_t i = elementStart; i < end; ++i) {
            elementTokens.push_back(tokens[i]);
        }

        append_populated_expression_child(expressionPool,
                                          listExpressionId,
                                          elementTokens,
                                          depth);
    }

    void populate_list_literal_children(std::vector<Expression>& expressionPool,
                                        const std::size_t listExpressionId,
                                        const std::vector<flowmini::Token>& tokens,
                                        const std::size_t expressionStart,
                                        const std::size_t depth) {
        if (!expression_starts_list_literal(tokens, expressionStart)) {
            return;
        }

        const auto leftBracketIndex = expressionStart;
        const auto rightBracketIndex = find_matching_right_bracket(tokens, leftBracketIndex);

        if (rightBracketIndex == tokens.size() || rightBracketIndex <= leftBracketIndex + 1) {
            return;
        }

        std::size_t elementBegin = leftBracketIndex + 1;
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = elementBegin; i <= rightBracketIndex; ++i) {
            const bool atEnd = i == rightBracketIndex;

            if (!atEnd) {
                const auto& token = tokens[i];

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightParen) {
                    if (parenDepth > 0) {
                        --parenDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBracket) {
                    if (bracketDepth > 0) {
                        --bracketDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBrace) {
                    if (braceDepth > 0) {
                        --braceDepth;
                    }
                    continue;
                }

                const bool atTopLevelComma =
                    token.kind == flowmini::TokenKind::Comma &&
                    parenDepth == 0 &&
                    bracketDepth == 0 &&
                    braceDepth == 0;

                if (!atTopLevelComma) {
                    continue;
                }
            }

            append_list_literal_element_child(expressionPool,
                                              listExpressionId,
                                              tokens,
                                              elementBegin,
                                              i,
                                              depth);
            elementBegin = i + 1;
        }
    }


    std::size_t find_matching_right_brace(const std::vector<flowmini::Token>& tokens,
                                          const std::size_t leftBraceIndex) {
        if (leftBraceIndex >= tokens.size() ||
            tokens[leftBraceIndex].kind != flowmini::TokenKind::LeftBrace) {
            return tokens.size();
        }

        std::size_t depth = 0;

        for (std::size_t i = leftBraceIndex; i < tokens.size(); ++i) {
            if (tokens[i].kind == flowmini::TokenKind::LeftBrace) {
                ++depth;
                continue;
            }

            if (tokens[i].kind == flowmini::TokenKind::RightBrace) {
                if (depth == 0) {
                    return tokens.size();
                }

                --depth;

                if (depth == 0) {
                    return i;
                }
            }

            if (is_end_token(tokens[i]) || tokens[i].kind == flowmini::TokenKind::Newline) {
                return tokens.size();
            }
        }

        return tokens.size();
    }

    std::size_t find_top_level_colon(const std::vector<flowmini::Token>& tokens,
                                     const std::size_t begin,
                                     const std::size_t end) {
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = begin; i < end; ++i) {
            const auto& token = tokens[i];

            if (token.kind == flowmini::TokenKind::LeftParen) {
                ++parenDepth;
                continue;
            }

            if (token.kind == flowmini::TokenKind::RightParen) {
                if (parenDepth > 0) {
                    --parenDepth;
                }
                continue;
            }

            if (token.kind == flowmini::TokenKind::LeftBracket) {
                ++bracketDepth;
                continue;
            }

            if (token.kind == flowmini::TokenKind::RightBracket) {
                if (bracketDepth > 0) {
                    --bracketDepth;
                }
                continue;
            }

            if (token.kind == flowmini::TokenKind::LeftBrace) {
                ++braceDepth;
                continue;
            }

            if (token.kind == flowmini::TokenKind::RightBrace) {
                if (braceDepth > 0) {
                    --braceDepth;
                }
                continue;
            }

            if (token.kind == flowmini::TokenKind::Colon &&
                parenDepth == 0 &&
                bracketDepth == 0 &&
                braceDepth == 0) {
                return i;
            }
        }

        return tokens.size();
    }

    void append_record_literal_value_child(std::vector<Expression>& expressionPool,
                                           const std::size_t recordExpressionId,
                                           const std::vector<flowmini::Token>& tokens,
                                           const std::size_t begin,
                                           const std::size_t end,
                                           const std::size_t depth) {
        std::size_t fieldStart = begin;

        while (fieldStart < end &&
               (tokens[fieldStart].kind == flowmini::TokenKind::Comma ||
                tokens[fieldStart].kind == flowmini::TokenKind::Newline)) {
            ++fieldStart;
        }

        if (fieldStart >= end) {
            return;
        }

        const auto colonIndex = find_top_level_colon(tokens, fieldStart, end);
        if (colonIndex == tokens.size() || colonIndex + 1 >= end) {
            return;
        }

        std::size_t valueStart = colonIndex + 1;
        while (valueStart < end &&
               (tokens[valueStart].kind == flowmini::TokenKind::Comma ||
                tokens[valueStart].kind == flowmini::TokenKind::Newline)) {
            ++valueStart;
        }

        if (valueStart >= end) {
            return;
        }

        std::vector<flowmini::Token> valueTokens;
        valueTokens.reserve(end - valueStart);

        for (std::size_t i = valueStart; i < end; ++i) {
            valueTokens.push_back(tokens[i]);
        }

        const auto valueId = append_populated_expression_child(expressionPool,
                                                                recordExpressionId,
                                                                valueTokens,
                                                                depth);
        auto* record = std::get_if<RecordLiteralExpr>(&expressionPool[recordExpressionId].payload);
        if (record != nullptr) {
            record->fields.push_back(RecordLiteralFieldExpr{
                tokens[fieldStart].text,
                valueId,
                location_from_token(tokens[fieldStart])
            });
        }
    }

    void populate_record_literal_children(std::vector<Expression>& expressionPool,
                                          const std::size_t recordExpressionId,
                                          const std::vector<flowmini::Token>& tokens,
                                          const std::size_t expressionStart,
                                          const std::size_t depth) {
        if (!expression_starts_record_literal(tokens, expressionStart)) {
            return;
        }

        const auto leftBraceIndex = expressionStart;
        const auto rightBraceIndex = find_matching_right_brace(tokens, leftBraceIndex);

        if (rightBraceIndex == tokens.size() || rightBraceIndex <= leftBraceIndex + 1) {
            return;
        }

        std::size_t fieldBegin = leftBraceIndex + 1;
        std::size_t parenDepth = 0;
        std::size_t bracketDepth = 0;
        std::size_t braceDepth = 0;

        for (std::size_t i = fieldBegin; i <= rightBraceIndex; ++i) {
            const bool atEnd = i == rightBraceIndex;

            if (!atEnd) {
                const auto& token = tokens[i];

                if (token.kind == flowmini::TokenKind::LeftParen) {
                    ++parenDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightParen) {
                    if (parenDepth > 0) {
                        --parenDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBracket) {
                    ++bracketDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBracket) {
                    if (bracketDepth > 0) {
                        --bracketDepth;
                    }
                    continue;
                }

                if (token.kind == flowmini::TokenKind::LeftBrace) {
                    ++braceDepth;
                    continue;
                }

                if (token.kind == flowmini::TokenKind::RightBrace) {
                    if (braceDepth > 0) {
                        --braceDepth;
                    }
                    continue;
                }

                const bool atTopLevelComma =
                    token.kind == flowmini::TokenKind::Comma &&
                    parenDepth == 0 &&
                    bracketDepth == 0 &&
                    braceDepth == 0;

                if (!atTopLevelComma) {
                    continue;
                }
            }

            append_record_literal_value_child(expressionPool,
                                              recordExpressionId,
                                              tokens,
                                              fieldBegin,
                                              i,
                                              depth);
            fieldBegin = i + 1;
        }
    }

    void populate_expression_children(std::vector<Expression>& expressionPool,
                                      const std::size_t expressionId,
                                      const std::vector<flowmini::Token>& tokens,
                                      const std::size_t expressionStart,
                                      const std::size_t depth) {
        if (depth >= max_expression_population_depth) {
            return;
        }

        const auto kind = expression_kind(expressionPool[expressionId]);

        if (kind == ExpressionKind::RecordLiteral) {
            populate_record_literal_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::ListLiteral) {
            populate_list_literal_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::FieldAccess) {
            populate_field_access_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::Index) {
            populate_index_expression_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::Binary) {
            populate_binary_operand_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::Unary) {
            populate_unary_operand_child(expressionPool, expressionId, tokens, expressionStart, depth);
        }

        if (kind == ExpressionKind::Call) {
            populate_call_argument_children(expressionPool, expressionId, tokens, expressionStart, depth);
        }
    }

    std::size_t add_expression_placeholder_at(std::vector<Expression>& expressionPool,
                                              Statement& statement,
                                              const std::vector<flowmini::Token>& tokens,
                                              const std::size_t i,
                                              const bool attachToLegacyProjection = true) {
            if (tokens.empty()) {
                return 0;
            }

            if (i >= tokens.size()) {
                return add_expression_placeholder(
                    expressionPool, statement, tokens.back(), attachToLegacyProjection);
            }

            auto expressionTokens = strip_enclosing_parentheses(expression_token_slice(tokens, i));
            if (expressionTokens.empty()) {
                return add_expression_placeholder(
                    expressionPool, statement, tokens[i], attachToLegacyProjection);
            }

            Expression expression = make_shallow_expression_from_tokens(expressionTokens, 0);

            expressionPool.push_back(std::move(expression));
            const auto expressionId = expressionPool.size() - 1;
            if (attachToLegacyProjection) {
                statement.expressions.push_back(expressionId);
            }

            populate_expression_children(expressionPool, expressionId, expressionTokens, 0, 0);

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
                statement.type = parse_type_ref(tokens, i);
            }

            statement.has_initializer =
                i < tokens.size() && tokens[i].kind == flowmini::TokenKind::LeftParen;

            if (statement.has_initializer) {
                const auto expressionStart = i + 1;
                if (expressionStart < tokens.size() &&
                    tokens[expressionStart].kind != flowmini::TokenKind::RightParen) {
                    statement.initializer_expression = add_expression_placeholder_at(
                        expressionPool,
                        statement,
                        tokens,
                        expressionStart,
                        false);
                }
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

            const bool hasValue = has_expression_until_statement_boundary(tokens, i);

            if (hasValue && i < tokens.size()) {
                statement.value_expression = add_expression_placeholder_at(
                    expressionPool, statement, tokens, i, false);
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

        std::size_t parse_return_statement_shell(const std::vector<flowmini::Token>& tokens,
                                                 std::size_t i,
                                                 std::vector<Statement>& body,
                                                 std::vector<Expression>& expressionPool) {
            Statement statement = make_statement_shell(StatementKind::Return, tokens[i]);
            const auto valueStart = i + 1;

            if (valueStart < tokens.size() &&
                has_expression_until_statement_boundary(tokens, valueStart)) {
                statement.value_expression = add_expression_placeholder_at(
                    expressionPool,
                    statement,
                    tokens,
                    valueStart,
                    false);
            }

            body.push_back(std::move(statement));

            i = valueStart;
            while (i < tokens.size() &&
                   !is_end_token(tokens[i]) &&
                   tokens[i].kind != flowmini::TokenKind::Newline &&
                   tokens[i].kind != flowmini::TokenKind::RightBrace) {
                ++i;
            }
            return i;
        }

        std::size_t parse_if_statement_shell(const std::vector<flowmini::Token>& tokens,
                                             std::size_t i,
                                             std::vector<Statement>& body,
                                             std::vector<Expression>& expressionPool) {
            Statement statement;
            statement.kind = StatementKind::If;
            statement.location = location_from_token(tokens[i]);

            ++i; // consume if

            const bool hasCondition = has_expression_until_body_or_line_end(tokens, i);
            if (hasCondition && i < tokens.size()) {
                statement.condition_expression = add_expression_placeholder_at(
                    expressionPool, statement, tokens, i, false);
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

            const bool hasCondition = has_expression_until_body_or_line_end(tokens, i);
            if (hasCondition && i < tokens.size()) {
                statement.condition_expression = add_expression_placeholder_at(
                    expressionPool, statement, tokens, i, false);
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
                    i = parse_return_statement_shell(tokens, i, body, expressionPool);
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

            // Function/main bodies and nested control-flow bodies share one
            // canonical statement parser.
            return parse_body_statement_shells(tokens, i, body, expressionPool);
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
