#include "flowmini_lexer.h"

#include "flow_common.h"

#include <cctype>

namespace flowmini {

const char* tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::Identifier:      return "identifier";
        case TokenKind::Number:          return "number";
        case TokenKind::String:          return "string";
        case TokenKind::KeywordModule:   return "module";
        case TokenKind::KeywordProducer: return "producer";
        case TokenKind::KeywordNode:     return "node";
        case TokenKind::KeywordSink:     return "sink";
        case TokenKind::KeywordWire:     return "wire";
        case TokenKind::KeywordPolicy:   return "policy";
        case TokenKind::Colon:           return ":";
        case TokenKind::Dot:             return ".";
        case TokenKind::Equals:          return "=";
        case TokenKind::Arrow:           return "=>";
        case TokenKind::Newline:         return "newline";
        case TokenKind::End:             return "end";
    }

    return "unknown";
}

namespace {

[[nodiscard]] bool isIdentStart(unsigned char c) {
    return std::isalpha(c) || c == '_';
}

[[nodiscard]] bool isIdentBody(unsigned char c) {
    return std::isalnum(c) || c == '_' || c == '-';
}

[[nodiscard]] TokenKind keywordOrIdentifier(const std::string& text) {
    if (text == "module") { return TokenKind::KeywordModule; }
    if (text == "producer") { return TokenKind::KeywordProducer; }
    if (text == "node") { return TokenKind::KeywordNode; }
    if (text == "sink") { return TokenKind::KeywordSink; }
    if (text == "wire") { return TokenKind::KeywordWire; }
    if (text == "policy") { return TokenKind::KeywordPolicy; }
    return TokenKind::Identifier;
}

} // namespace

std::vector<Token> lexSource(const std::string& source) {
    std::vector<Token> tokens;

    std::size_t i = 0;
    int line = 1;
    int column = 1;

    auto push = [&](TokenKind kind, std::string text, int tokenLine, int tokenColumn) {
        tokens.push_back(Token{kind, std::move(text), tokenLine, tokenColumn});
    };

    while (i < source.size()) {
        const auto c = static_cast<unsigned char>(source[i]);

        if (c == ' ' || c == '\t' || c == '\r') {
            ++i;
            ++column;
            continue;
        }

        if (c == '\n') {
            push(TokenKind::Newline, "\\n", line, column);
            ++i;
            ++line;
            column = 1;
            continue;
        }

        if (c == '#') {
            while (i < source.size() && source[i] != '\n') {
                ++i;
                ++column;
            }
            continue;
        }

        if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') {
                ++i;
                ++column;
            }
            continue;
        }

        if (isIdentStart(c)) {
            const int tokenLine = line;
            const int tokenColumn = column;
            const std::size_t start = i;

            while (i < source.size() && isIdentBody(static_cast<unsigned char>(source[i]))) {
                ++i;
                ++column;
            }

            std::string text = source.substr(start, i - start);
            push(keywordOrIdentifier(text), text, tokenLine, tokenColumn);
            continue;
        }

        if (std::isdigit(c) || (c == '-' && i + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[i + 1])))) {
            const int tokenLine = line;
            const int tokenColumn = column;
            const std::size_t start = i;
            if (source[i] == '-') {
                ++i;
                ++column;
            }
            while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) {
                ++i;
                ++column;
            }
            push(TokenKind::Number, source.substr(start, i - start), tokenLine, tokenColumn);
            continue;
        }

        if (c == '"') {
            const int tokenLine = line;
            const int tokenColumn = column;
            ++i;
            ++column;
            std::string text;
            while (i < source.size()) {
                const char ch = source[i];
                if (ch == '"') {
                    ++i;
                    ++column;
                    push(TokenKind::String, text, tokenLine, tokenColumn);
                    goto next_char;
                }
                if (ch == '\\') {
                    if (i + 1 >= source.size()) {
                        throw flow::DiagnosticError{"lexer", "unterminated escape in string"};
                    }
                    const char esc = source[i + 1];
                    if (esc == 'n') { text.push_back('\n'); }
                    else if (esc == 't') { text.push_back('\t'); }
                    else if (esc == 'r') { text.push_back('\r'); }
                    else { text.push_back(esc); }
                    i += 2;
                    column += 2;
                    continue;
                }
                if (ch == '\n') {
                    throw flow::DiagnosticError{"lexer", "unterminated string literal"};
                }
                text.push_back(ch);
                ++i;
                ++column;
            }
            throw flow::DiagnosticError{"lexer", "unterminated string literal"};
        }

        if (c == ':') {
            push(TokenKind::Colon, ":", line, column);
            ++i;
            ++column;
            continue;
        }

        if (c == '.') {
            push(TokenKind::Dot, ".", line, column);
            ++i;
            ++column;
            continue;
        }

        if (c == '=' && i + 1 < source.size() && source[i + 1] == '>') {
            push(TokenKind::Arrow, "=>", line, column);
            i += 2;
            column += 2;
            continue;
        }

        if (c == '=') {
            push(TokenKind::Equals, "=", line, column);
            ++i;
            ++column;
            continue;
        }

        throw flow::DiagnosticError{
            "lexer",
            "unexpected character at line " + std::to_string(line) +
            ", column " + std::to_string(column)
        };

next_char:
        continue;
    }

    push(TokenKind::End, "", line, column);
    return tokens;
}

} // namespace flowmini
