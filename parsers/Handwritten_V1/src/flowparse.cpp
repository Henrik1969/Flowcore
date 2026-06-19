#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace langlab {

struct SourcePos {
    std::size_t line{0};
    std::size_t column{0};
};

struct Span {
    SourcePos begin{};
    SourcePos end{};
};

struct Token {
    std::string type;
    std::string lexeme;
    SourcePos pos{};
};

struct SourceInfo {
    std::size_t source_id{0};
    std::string file;
    std::string origin;
};

struct Annotation {
    std::size_t source_id{0};
    std::size_t line{0};
    std::size_t column{0};
    std::string message;
};

class ParseError final : public std::runtime_error {
public:
    ParseError(Token token, const std::string& message)
        : std::runtime_error(message), token_(std::move(token)) {}

    [[nodiscard]] const Token& token() const noexcept { return token_; }

private:
    Token token_;
};

[[nodiscard]] std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);

    for (const unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    static constexpr char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0x0F];
                    out += hex[c & 0x0F];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }

    return out;
}

[[nodiscard]] std::optional<std::string> read_json_string(
    const std::string& line,
    const std::string& key
) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t key_pos = line.find(marker);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    std::size_t pos = key_pos + marker.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
    }

    if (pos >= line.size() || line[pos] != '"') {
        return std::nullopt;
    }

    ++pos;
    std::string out;

    while (pos < line.size()) {
        const char c = line[pos++];
        if (c == '"') {
            return out;
        }

        if (c != '\\') {
            out += c;
            continue;
        }

        if (pos >= line.size()) {
            throw std::runtime_error("truncated JSON escape");
        }

        const char escaped = line[pos++];
        switch (escaped) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default:
                throw std::runtime_error("unsupported JSON escape in token stream");
        }
    }

    throw std::runtime_error("unterminated JSON string in token stream");
}

[[nodiscard]] std::optional<std::size_t> read_json_size(
    const std::string& line,
    const std::string& key
) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t key_pos = line.find(marker);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    std::size_t pos = key_pos + marker.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
    }

    std::size_t end = pos;
    while (end < line.size() && std::isdigit(static_cast<unsigned char>(line[end])) != 0) {
        ++end;
    }

    if (end == pos) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::stoull(line.substr(pos, end - pos)));
}

struct AstNode {
    std::string kind;
    std::string value;
    Span span{};
    std::map<std::string, std::string> attributes;
    std::vector<std::unique_ptr<AstNode>> children;
};

[[nodiscard]] std::unique_ptr<AstNode> node(
    std::string kind,
    const Token& token,
    std::string value = {}
) {
    auto result = std::make_unique<AstNode>();
    result->kind = std::move(kind);
    result->value = std::move(value);
    result->span.begin = token.pos;
    result->span.end = token.pos;
    return result;
}

void extend_span(AstNode& parent, const AstNode& child) {
    parent.span.end = child.span.end;
}

class TokenStream final {
public:
    explicit TokenStream(std::istream& input) {
        std::string line;
        std::size_t physical_line = 0;

        while (std::getline(input, line)) {
            ++physical_line;
            if (line.empty()) {
                continue;
            }

            const std::string type = read_json_string(line, "type").value_or("");
            if (type.empty()) {
                throw std::runtime_error(
                    "token-stream line " + std::to_string(physical_line) +
                    " has no string field 'type'"
                );
            }

            if (type == "GHOST_SOURCE_BEGIN") {
                SourceInfo source;
                source.source_id = read_json_size(line, "source_id").value_or(0);
                source.file = read_json_string(line, "file").value_or("<unknown>");
                source.origin = read_json_string(line, "origin").value_or("");
                sources_.push_back(std::move(source));
                continue;
            }

            if (type == "GHOST_ANNOTATION") {
                Annotation annotation;
                annotation.source_id = read_json_size(line, "source_id").value_or(0);
                annotation.line = read_json_size(line, "line").value_or(0);
                annotation.column = read_json_size(line, "column").value_or(0);
                annotation.message = read_json_string(line, "message").value_or("");
                annotations_.push_back(std::move(annotation));
                continue;
            }

            if (type.rfind("GHOST_", 0) == 0) {
                continue;
            }

            Token token;
            token.type = type;
            token.lexeme = read_json_string(line, "lexeme").value_or("");
            token.pos.line = read_json_size(line, "line").value_or(0);
            token.pos.column = read_json_size(line, "column").value_or(0);
            tokens_.push_back(std::move(token));
        }

        if (tokens_.empty() || tokens_.back().type != "EOF") {
            tokens_.push_back(Token{"EOF", "", SourcePos{0, 0}});
        }
    }

    [[nodiscard]] const std::vector<Token>& tokens() const noexcept { return tokens_; }
    [[nodiscard]] const std::vector<SourceInfo>& sources() const noexcept { return sources_; }
    [[nodiscard]] const std::vector<Annotation>& annotations() const noexcept { return annotations_; }

private:
    std::vector<Token> tokens_;
    std::vector<SourceInfo> sources_;
    std::vector<Annotation> annotations_;
};

class Parser final {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    [[nodiscard]] std::unique_ptr<AstNode> parse_program() {
        auto root = node("Program", current());
        skip_separators();

        while (!check("EOF")) {
            root->children.push_back(parse_statement(false));
            skip_separators();
        }

        root->span.end = current().pos;
        return root;
    }

private:
    [[nodiscard]] const Token& current() const noexcept { return tokens_[position_]; }
    [[nodiscard]] const Token& previous() const noexcept { return tokens_[position_ - 1]; }

    [[nodiscard]] bool check(const std::string& type) const noexcept {
        return current().type == type;
    }

    [[nodiscard]] bool check_keyword(const std::string& lexeme) const noexcept {
        return current().type == "KEYWORD" && current().lexeme == lexeme;
    }

    const Token& advance() {
        if (!check("EOF")) {
            ++position_;
        }
        return previous();
    }

    bool match(const std::string& type) {
        if (!check(type)) {
            return false;
        }
        advance();
        return true;
    }

    bool match_keyword(const std::string& lexeme) {
        if (!check_keyword(lexeme)) {
            return false;
        }
        advance();
        return true;
    }

    const Token& expect(const std::string& type, const std::string& message) {
        if (!check(type)) {
            throw ParseError(current(), message);
        }
        return advance();
    }

    const Token& expect_keyword(const std::string& lexeme, const std::string& message) {
        if (!check_keyword(lexeme)) {
            throw ParseError(current(), message);
        }
        return advance();
    }

    void skip_separators() {
        while (match("NEWLINE") || match("SEMICOLON")) {
        }
    }

    void skip_newlines() {
        while (match("NEWLINE")) {
        }
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_statement(bool inside_flow) {
        if (match_keyword("module")) {
            return parse_module(previous());
        }

        if (match_keyword("flow")) {
            return parse_flow(previous());
        }

        if (match_keyword("if")) {
            return parse_if(previous());
        }

        if (match_keyword("while")) {
            return parse_condition_loop("WhileStatement", previous());
        }

        if (match_keyword("until")) {
            return parse_condition_loop("UntilStatement", previous());
        }

        if (match_keyword("return")) {
            return parse_return(previous());
        }

        if (match("DECLARE")) {
            return parse_declaration(previous());
        }

        if (check("LEFT_BRACE")) {
            return parse_block(inside_flow);
        }

        auto left = parse_expression();
        skip_newlines_if_followed_by({"ASSIGN", "GRAPH_ARROW"});

        if (match("ASSIGN")) {
            const Token equals = previous();
            skip_newlines();
            auto assignment = node("AssignmentStatement", equals, "=");
            assignment->children.push_back(std::move(left));
            assignment->children.push_back(parse_expression());
            extend_span(*assignment, *assignment->children.back());
            return assignment;
        }

        if (match("GRAPH_ARROW")) {
            if (!inside_flow) {
                throw ParseError(previous(), "graph wiring '->' is only legal inside flow blocks");
            }

            auto connection = node("GraphConnection", previous(), "->");
            connection->children.push_back(std::move(left));

            do {
                skip_newlines();
                connection->children.push_back(parse_expression_without_pipe());
                extend_span(*connection, *connection->children.back());
                skip_newlines_if_followed_by({"GRAPH_ARROW"});
            } while (match("GRAPH_ARROW"));

            return connection;
        }

        auto statement = node("ExpressionStatement", token_from_node(*left));
        statement->children.push_back(std::move(left));
        extend_span(*statement, *statement->children.front());
        return statement;
    }

    [[nodiscard]] Token token_from_node(const AstNode& ast) const {
        return Token{"", ast.value, ast.span.begin};
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_module(const Token& keyword) {
        const Token& first = expect("IDENTIFIER", "expected module name after 'module'");
        std::string qualified_name = first.lexeme;
        auto declaration = node("ModuleDeclaration", keyword, qualified_name);

        while (match("DOT")) {
            const Token& part = expect("IDENTIFIER", "expected identifier after '.' in module name");
            qualified_name += "." + part.lexeme;
            declaration->value = qualified_name;
            declaration->span.end = part.pos;
        }

        if (check("LEFT_BRACE")) {
            declaration->children.push_back(parse_block(false));
            extend_span(*declaration, *declaration->children.back());
        }

        return declaration;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_flow(const Token& keyword) {
        const Token& name = expect("IDENTIFIER", "expected flow name after 'flow'");
        auto flow = node("FlowDeclaration", keyword, name.lexeme);
        skip_newlines();
        flow->children.push_back(parse_block(true));
        extend_span(*flow, *flow->children.back());
        return flow;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_if(const Token& keyword) {
        auto branch = node("IfStatement", keyword);
        branch->children.push_back(parse_parenthesized_condition());
        skip_newlines();
        branch->children.push_back(parse_block(false));

        skip_newlines();
        while (match_keyword("elseif")) {
            const Token marker = previous();
            auto alternative = node("ElseIfClause", marker);
            alternative->children.push_back(parse_parenthesized_condition());
            skip_newlines();
            alternative->children.push_back(parse_block(false));
            extend_span(*alternative, *alternative->children.back());
            branch->children.push_back(std::move(alternative));
            skip_newlines();
        }

        if (match_keyword("else")) {
            const Token marker = previous();
            auto alternative = node("ElseClause", marker);
            skip_newlines();
            alternative->children.push_back(parse_block(false));
            extend_span(*alternative, *alternative->children.back());
            branch->children.push_back(std::move(alternative));
        }

        extend_span(*branch, *branch->children.back());
        return branch;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_condition_loop(
        const std::string& kind,
        const Token& keyword
    ) {
        auto loop = node(kind, keyword);
        loop->children.push_back(parse_parenthesized_condition());
        skip_newlines();
        loop->children.push_back(parse_block(false));
        extend_span(*loop, *loop->children.back());
        return loop;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_parenthesized_condition() {
        expect("LEFT_PAREN", "expected '(' before condition");
        skip_newlines();
        auto condition = parse_expression();
        skip_newlines();
        expect("RIGHT_PAREN", "expected ')' after condition");
        return condition;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_return(const Token& keyword) {
        auto result = node("ReturnStatement", keyword);
        if (check("NEWLINE") || check("SEMICOLON") || check("RIGHT_BRACE") || check("EOF")) {
            return result;
        }

        result->children.push_back(parse_expression());
        extend_span(*result, *result->children.back());
        return result;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_declaration(const Token& marker) {
        const Token& name = expect("IDENTIFIER", "expected identifier after '§'");
        expect("COLON", "expected ':' after declared identifier");
        skip_newlines();

        auto declaration = node("DeclarationStatement", marker, name.lexeme);
        declaration->children.push_back(parse_type_expression());

        skip_newlines_if_followed_by({"ASSIGN"});
        if (match("ASSIGN")) {
            skip_newlines();
            auto initializer = node("Initializer", previous(), "=");
            const bool declares_type_descriptor =
                !declaration->children.empty() &&
                declaration->children.front()->kind == "TypeName" &&
                declaration->children.front()->value == "type";

            if (declares_type_descriptor) {
                initializer->children.push_back(parse_type_expression());
            } else {
                initializer->children.push_back(parse_expression());
            }
            extend_span(*initializer, *initializer->children.back());
            declaration->children.push_back(std::move(initializer));
        }

        extend_span(*declaration, *declaration->children.back());
        return declaration;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_type_expression() {
        skip_newlines();

        if (match("LEFT_PAREN")) {
            const Token open = previous();
            auto callable = node("CallableType", open);
            skip_newlines();

            if (!check("RIGHT_PAREN")) {
                do {
                    skip_newlines();
                    const Token& name = expect("IDENTIFIER", "expected parameter name in callable type");
                    expect("COLON", "expected ':' after parameter name");
                    auto parameter = node("Parameter", name, name.lexeme);
                    parameter->children.push_back(parse_type_expression());
                    extend_span(*parameter, *parameter->children.back());
                    callable->children.push_back(std::move(parameter));
                    skip_newlines();
                } while (match("COMMA"));
            }

            expect("RIGHT_PAREN", "expected ')' after callable parameters");
            expect("COLON", "expected ':' before callable result type");
            callable->children.push_back(parse_type_expression());
            extend_span(*callable, *callable->children.back());
            return callable;
        }

        if (!(check("IDENTIFIER") || check("KEYWORD"))) {
            throw ParseError(current(), "expected type expression");
        }

        const Token name = advance();
        auto type = node("TypeName", name, name.lexeme);

        if (match("LESS")) {
            const Token open = previous();
            auto generic = node("GenericType", open, name.lexeme);
            skip_newlines();

            if (!check("GREATER")) {
                do {
                    skip_newlines();
                    if (check("INTEGER_LITERAL")) {
                        const Token literal = advance();
                        generic->children.push_back(node("TypeValueArgument", literal, literal.lexeme));
                    } else {
                        generic->children.push_back(parse_type_expression());
                    }
                    skip_newlines();
                } while (match("COMMA"));
            }

            const Token& close = expect("GREATER", "expected '>' after generic type arguments");
            generic->span.end = close.pos;
            return generic;
        }

        return type;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_block(bool inside_flow) {
        const Token& open = expect("LEFT_BRACE", "expected '{'");
        auto block = node("Block", open);
        skip_separators();

        while (!check("RIGHT_BRACE") && !check("EOF")) {
            block->children.push_back(parse_statement(inside_flow));
            skip_separators();
        }

        const Token& close = expect("RIGHT_BRACE", "expected '}' after block");
        block->span.end = close.pos;
        return block;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_expression() {
        return parse_binary(1);
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_expression_without_pipe() {
        return parse_binary(2);
    }

    [[nodiscard]] int precedence(const std::string& token_type) const noexcept {
        if (token_type == "PIPE") return 1;
        if (token_type == "LOGICAL_OR") return 2;
        if (token_type == "LOGICAL_AND") return 3;
        if (token_type == "BIT_OR") return 4;
        if (token_type == "XOR") return 5;
        if (token_type == "BIT_AND") return 6;
        if (token_type == "EQUAL_EQUAL" || token_type == "BANG_EQUAL") return 7;
        if (token_type == "LESS" || token_type == "LESS_EQUAL" ||
            token_type == "GREATER" || token_type == "GREATER_EQUAL") return 8;
        if (token_type == "SHIFT_LEFT" || token_type == "SHIFT_RIGHT") return 9;
        if (token_type == "PLUS" || token_type == "MINUS") return 10;
        if (token_type == "STAR" || token_type == "SLASH" || token_type == "PERCENT") return 11;
        if (token_type == "AT") return 12;
        return 0;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_binary(int minimum_precedence) {
        auto left = parse_unary();

        while (true) {
            const std::size_t saved = position_;
            skip_newlines();
            const int current_precedence = precedence(current().type);

            if (current_precedence < minimum_precedence) {
                position_ = saved;
                break;
            }

            const Token operation = advance();
            skip_newlines();
            auto right = parse_binary(current_precedence + 1);

            auto binary = node(
                operation.type == "PIPE" ? "PipelineExpression" : "BinaryExpression",
                operation,
                operation.lexeme
            );
            binary->children.push_back(std::move(left));
            binary->children.push_back(std::move(right));
            binary->span.begin = binary->children.front()->span.begin;
            extend_span(*binary, *binary->children.back());
            left = std::move(binary);
        }

        return left;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_unary() {
        skip_newlines();

        if (check("BANG") || check("TILDE") || check("DEREFERENCE") ||
            check("PLUS") || check("MINUS") || check_keyword("ref")) {
            const Token operation = advance();
            auto unary = node("UnaryExpression", operation, operation.lexeme);
            unary->children.push_back(parse_unary());
            extend_span(*unary, *unary->children.back());
            return unary;
        }

        if (match("AT")) {
            const Token operation = previous();
            const Token& member = expect("IDENTIFIER", "expected member name after '@'");
            auto projection = node("ReverseProjectionStage", operation, member.lexeme);
            projection->span.end = member.pos;
            return projection;
        }

        return parse_postfix();
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_postfix() {
        auto expression = parse_primary();

        while (true) {
            const std::size_t saved = position_;
            skip_newlines();

            if (match("DOT")) {
                const Token& member = expect("IDENTIFIER", "expected member name after '.'");
                auto projection = node("MemberProjection", previous(), member.lexeme);
                projection->children.push_back(std::move(expression));
                projection->span.begin = projection->children.front()->span.begin;
                projection->span.end = member.pos;
                expression = std::move(projection);
                continue;
            }

            if (match("LEFT_BRACKET")) {
                const Token open = previous();
                auto index = node("IndexExpression", open);
                index->children.push_back(std::move(expression));
                skip_newlines();
                index->children.push_back(parse_expression());
                skip_newlines();
                const Token& close = expect("RIGHT_BRACKET", "expected ']' after index expression");
                index->span.begin = index->children.front()->span.begin;
                index->span.end = close.pos;
                expression = std::move(index);
                continue;
            }

            if (match("LEFT_PAREN")) {
                const Token open = previous();
                auto call = node("CallExpression", open);
                call->children.push_back(std::move(expression));
                skip_newlines();

                if (!check("RIGHT_PAREN")) {
                    do {
                        skip_newlines();
                        call->children.push_back(parse_expression());
                        skip_newlines();
                    } while (match("COMMA"));
                }

                const Token& close = expect("RIGHT_PAREN", "expected ')' after call arguments");
                call->span.begin = call->children.front()->span.begin;
                call->span.end = close.pos;
                expression = std::move(call);
                continue;
            }

            position_ = saved;
            break;
        }

        return expression;
    }

    [[nodiscard]] std::unique_ptr<AstNode> parse_primary() {
        skip_newlines();

        if (check("IDENTIFIER")) {
            const Token token = advance();
            return node("Identifier", token, token.lexeme);
        }

        if (check("INTEGER_LITERAL") || check("FLOAT_LITERAL") ||
            check("STRING_LITERAL") || check("CHAR_LITERAL")) {
            const Token token = advance();
            return node("Literal", token, token.lexeme);
        }

        if (check_keyword("true") || check_keyword("false") || check_keyword("none")) {
            const Token token = advance();
            return node("Literal", token, token.lexeme);
        }

        if (match("LEFT_PAREN")) {
            const Token open = previous();
            skip_newlines();
            auto grouped = node("GroupedExpression", open);
            grouped->children.push_back(parse_expression());
            skip_newlines();
            const Token& close = expect("RIGHT_PAREN", "expected ')' after grouped expression");
            grouped->span.end = close.pos;
            return grouped;
        }

        throw ParseError(current(), "expected expression");
    }

    void skip_newlines_if_followed_by(const std::vector<std::string>& types) {
        const std::size_t saved = position_;
        skip_newlines();
        const bool matched = std::find(types.begin(), types.end(), current().type) != types.end();
        if (!matched) {
            position_ = saved;
        }
    }

    const std::vector<Token>& tokens_;
    std::size_t position_{0};
};

void write_indent(std::ostream& output, int indent) {
    for (int i = 0; i < indent; ++i) {
        output << ' ';
    }
}

void write_ast_node(std::ostream& output, const AstNode& ast, int indent) {
    write_indent(output, indent);
    output << "{\n";

    write_indent(output, indent + 2);
    output << "\"kind\":\"" << json_escape(ast.kind) << "\"";

    if (!ast.value.empty()) {
        output << ",\n";
        write_indent(output, indent + 2);
        output << "\"value\":\"" << json_escape(ast.value) << "\"";
    }

    output << ",\n";
    write_indent(output, indent + 2);
    output << "\"span\":{"
           << "\"line\":" << ast.span.begin.line
           << ",\"column\":" << ast.span.begin.column
           << ",\"end_line\":" << ast.span.end.line
           << ",\"end_column\":" << ast.span.end.column
           << "}";

    if (!ast.attributes.empty()) {
        output << ",\n";
        write_indent(output, indent + 2);
        output << "\"attributes\":{";
        bool first = true;
        for (const auto& [key, value] : ast.attributes) {
            if (!first) output << ',';
            output << "\"" << json_escape(key) << "\":\"" << json_escape(value) << "\"";
            first = false;
        }
        output << '}';
    }

    output << ",\n";
    write_indent(output, indent + 2);
    output << "\"children\":[";

    if (!ast.children.empty()) {
        output << '\n';
        for (std::size_t i = 0; i < ast.children.size(); ++i) {
            write_ast_node(output, *ast.children[i], indent + 4);
            if (i + 1 < ast.children.size()) output << ',';
            output << '\n';
        }
        write_indent(output, indent + 2);
    }

    output << "]\n";
    write_indent(output, indent);
    output << '}';
}

void write_document(
    std::ostream& output,
    const std::vector<SourceInfo>& sources,
    const std::vector<Annotation>& annotations,
    const AstNode& root
) {
    output << "{\n";
    output << "  \"schema\":\"langlab.ast.json\",\n";
    output << "  \"version\":\"0.1-A\",\n";
    output << "  \"parser\":\"flowparse-handwritten-cpp\",\n";

    output << "  \"sources\":[";
    if (!sources.empty()) output << '\n';
    for (std::size_t i = 0; i < sources.size(); ++i) {
        const SourceInfo& source = sources[i];
        output << "    {\"source_id\":" << source.source_id
               << ",\"file\":\"" << json_escape(source.file)
               << "\",\"origin\":\"" << json_escape(source.origin) << "\"}";
        if (i + 1 < sources.size()) output << ',';
        output << '\n';
    }
    if (!sources.empty()) output << "  ";
    output << "],\n";

    output << "  \"annotations\":[";
    if (!annotations.empty()) output << '\n';
    for (std::size_t i = 0; i < annotations.size(); ++i) {
        const Annotation& annotation = annotations[i];
        output << "    {\"source_id\":" << annotation.source_id
               << ",\"line\":" << annotation.line
               << ",\"column\":" << annotation.column
               << ",\"message\":\"" << json_escape(annotation.message) << "\"}";
        if (i + 1 < annotations.size()) output << ',';
        output << '\n';
    }
    if (!annotations.empty()) output << "  ";
    output << "],\n";

    output << "  \"root\":";
    write_ast_node(output, root, 2);
    output << "\n}\n";
}

} // namespace langlab

int main() {
    try {
        langlab::TokenStream token_stream{std::cin};
        langlab::Parser parser{token_stream.tokens()};
        std::unique_ptr<langlab::AstNode> root = parser.parse_program();

        langlab::write_document(
            std::cout,
            token_stream.sources(),
            token_stream.annotations(),
            *root
        );

        return EXIT_SUCCESS;
    } catch (const langlab::ParseError& error) {
        std::cerr
            << "parse error at "
            << error.token().pos.line << ':' << error.token().pos.column
            << " near token " << error.token().type;

        if (!error.token().lexeme.empty()) {
            std::cerr << " ('" << error.token().lexeme << "')";
        }

        std::cerr << ": " << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "fatal parser error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
