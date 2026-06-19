#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace flowcore {

struct SourcePos {
    std::size_t line{1};
    std::size_t column{1};
};

struct Token {
    std::string type;
    std::string lexeme;
    SourcePos pos;
};

class LexError final : public std::runtime_error {
public:
    LexError(SourcePos pos, const std::string& message)
        : std::runtime_error(message), pos_(pos) {}

    [[nodiscard]] const SourcePos& pos() const noexcept { return pos_; }

private:
    SourcePos pos_;
};

class Lexer final {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) {}

    [[nodiscard]] Token next() {
        while (true) {
            if (in_block_comment_) {
                if (eof()) {
                    throw LexError(block_comment_start_, "unterminated block comment");
                }

                if (match("*/")) {
                    in_block_comment_ = false;
                    continue;
                }

                if (is_newline()) {
                    const SourcePos start = pos_;
                    consume_newline();
                    return Token{"NEWLINE", "\\n", start};
                }

                advance();
                continue;
            }

            skip_horizontal_whitespace();

            if (eof()) {
                return Token{"EOF", "", pos_};
            }

            if (is_newline()) {
                const SourcePos start = pos_;
                consume_newline();
                return Token{"NEWLINE", "\\n", start};
            }

            if (match("//")) {
                while (!eof() && !is_newline()) {
                    advance();
                }
                continue;
            }

            if (match("/*")) {
                in_block_comment_ = true;
                block_comment_start_ = previous_pos_;
                continue;
            }

            return lex_token();
        }
    }

private:
    [[nodiscard]] bool eof(std::size_t lookahead = 0) const noexcept {
        return index_ + lookahead >= source_.size();
    }

    [[nodiscard]] char peek(std::size_t lookahead = 0) const noexcept {
        return eof(lookahead) ? '\0' : source_[index_ + lookahead];
    }

    [[nodiscard]] bool starts_with(std::string_view text) const noexcept {
        return source_.compare(index_, text.size(), text) == 0;
    }

    [[nodiscard]] bool is_newline() const noexcept {
        return peek() == '\n' || (peek() == '\r' && peek(1) == '\n');
    }

    bool match(std::string_view text) {
        if (!starts_with(text)) {
            return false;
        }

        previous_pos_ = pos_;
        index_ += text.size();
        pos_.column += text.size();
        return true;
    }

    char advance() {
        if (eof()) {
            return '\0';
        }

        previous_pos_ = pos_;
        const char c = source_[index_++];
        ++pos_.column;
        return c;
    }

    void advance_section_symbol() {
        previous_pos_ = pos_;
        index_ += 2; // UTF-8 C2 A7
        ++pos_.column;
    }

    void consume_newline() {
        previous_pos_ = pos_;
        if (peek() == '\r' && peek(1) == '\n') {
            index_ += 2;
        } else {
            ++index_;
        }

        ++pos_.line;
        pos_.column = 1;
    }

    void skip_horizontal_whitespace() {
        while (!eof()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\f' || c == '\v') {
                advance();
                continue;
            }
            break;
        }
    }

    [[nodiscard]] static bool is_identifier_start(unsigned char c) noexcept {
        return std::isalpha(c) != 0 || c == '_';
    }

    [[nodiscard]] static bool is_identifier_continue(unsigned char c) noexcept {
        return std::isalnum(c) != 0 || c == '_';
    }

    [[nodiscard]] static bool is_digit_for_base(char c, int base) noexcept {
        if (base <= 10) {
            return c >= '0' && c < ('0' + base);
        }

        return std::isdigit(static_cast<unsigned char>(c)) != 0 ||
               (c >= 'a' && c < ('a' + base - 10)) ||
               (c >= 'A' && c < ('A' + base - 10));
    }

    Token lex_token() {
        const SourcePos start = pos_;

        if (!eof(1) &&
            static_cast<unsigned char>(peek()) == 0xC2 &&
            static_cast<unsigned char>(peek(1)) == 0xA7) {
            advance_section_symbol();
            return Token{"DECLARE", "§", start};
        }

        const unsigned char c = static_cast<unsigned char>(peek());
        if (is_identifier_start(c)) return lex_identifier_or_keyword();
        if (std::isdigit(c) != 0) return lex_number();
        if (peek() == '"') return lex_string();
        if (peek() == '\'') return lex_character();

        if (match("=>")) return Token{"PIPE", "=>", start};
        if (match("->")) return Token{"GRAPH_ARROW", "->", start};
        if (match("&&")) return Token{"LOGICAL_AND", "&&", start};
        if (match("||")) return Token{"LOGICAL_OR", "||", start};
        if (match("<<")) return Token{"SHIFT_LEFT", "<<", start};
        if (match(">>")) return Token{"SHIFT_RIGHT", ">>", start};
        if (match("<=")) return Token{"LESS_EQUAL", "<=", start};
        if (match(">=")) return Token{"GREATER_EQUAL", ">=", start};
        if (match("==")) return Token{"EQUAL_EQUAL", "==", start};
        if (match("!=")) return Token{"BANG_EQUAL", "!=", start};
        if (match("..")) return Token{"RANGE", "..", start};

        const char single = advance();
        switch (single) {
            case ':': return Token{"COLON", ":", start};
            case '=': return Token{"ASSIGN", "=", start};
            case '.': return Token{"DOT", ".", start};
            case '@': return Token{"AT", "@", start};
            case '#': return Token{"DEREFERENCE", "#", start};
            case ',': return Token{"COMMA", ",", start};
            case ';': return Token{"SEMICOLON", ";", start};
            case '(': return Token{"LEFT_PAREN", "(", start};
            case ')': return Token{"RIGHT_PAREN", ")", start};
            case '{': return Token{"LEFT_BRACE", "{", start};
            case '}': return Token{"RIGHT_BRACE", "}", start};
            case '[': return Token{"LEFT_BRACKET", "[", start};
            case ']': return Token{"RIGHT_BRACKET", "]", start};
            case '?': return Token{"QUESTION", "?", start};
            case '+': return Token{"PLUS", "+", start};
            case '-': return Token{"MINUS", "-", start};
            case '*': return Token{"STAR", "*", start};
            case '/': return Token{"SLASH", "/", start};
            case '%': return Token{"PERCENT", "%", start};
            case '!': return Token{"BANG", "!", start};
            case '~': return Token{"TILDE", "~", start};
            case '&': return Token{"BIT_AND", "&", start};
            case '|': return Token{"BIT_OR", "|", start};
            case '^': return Token{"XOR", "^", start};
            case '<': return Token{"LESS", "<", start};
            case '>': return Token{"GREATER", ">", start};
            default:
                throw LexError(start, std::string("unexpected character: '") + single + "'");
        }
    }

    Token lex_identifier_or_keyword() {
        const SourcePos start = pos_;
        const std::size_t begin = index_;

        advance();
        while (!eof() && is_identifier_continue(static_cast<unsigned char>(peek()))) {
            advance();
        }

        std::string text = source_.substr(begin, index_ - begin);
        if (keywords().find(text) != keywords().end()) {
            return Token{"KEYWORD", std::move(text), start};
        }

        return Token{"IDENTIFIER", std::move(text), start};
    }

    Token lex_number() {
        const SourcePos start = pos_;
        const std::size_t begin = index_;

        int base = 10;
        bool is_float = false;

        if (peek() == '0' && !eof(1)) {
            const char prefix = peek(1);
            if (prefix == 'x' || prefix == 'X') {
                base = 16; advance(); advance();
            } else if (prefix == 'b' || prefix == 'B') {
                base = 2; advance(); advance();
            } else if (prefix == 'o' || prefix == 'O') {
                base = 8; advance(); advance();
            }
        }

        bool saw_digit = false;
        while (!eof()) {
            const char c = peek();
            if (c == '_') { advance(); continue; }
            if (!is_digit_for_base(c, base)) break;
            saw_digit = true;
            advance();
        }

        if (!saw_digit && base != 10) {
            throw LexError(start, "base-prefixed numeric literal requires at least one digit");
        }

        if (base == 10 && peek() == '.' && peek(1) != '.' &&
            std::isdigit(static_cast<unsigned char>(peek(1))) != 0) {
            is_float = true;
            advance();
            while (!eof()) {
                const char c = peek();
                if (c == '_') { advance(); continue; }
                if (std::isdigit(static_cast<unsigned char>(c)) == 0) break;
                advance();
            }
        }

        if (base == 10 && (peek() == 'e' || peek() == 'E')) {
            is_float = true;
            advance();
            if (peek() == '+' || peek() == '-') advance();
            if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                throw LexError(start, "scientific notation requires exponent digits");
            }
            while (!eof()) {
                const char c = peek();
                if (c == '_') { advance(); continue; }
                if (std::isdigit(static_cast<unsigned char>(c)) == 0) break;
                advance();
            }
        }

        return Token{
            is_float ? "FLOAT_LITERAL" : "INTEGER_LITERAL",
            source_.substr(begin, index_ - begin),
            start
        };
    }

    Token lex_string() {
        const SourcePos start = pos_;
        const std::size_t begin = index_;
        advance();

        while (!eof()) {
            if (peek() == '"') {
                advance();
                return Token{"STRING_LITERAL", source_.substr(begin, index_ - begin), start};
            }
            if (peek() == '\\') {
                advance();
                if (eof() || is_newline()) {
                    throw LexError(start, "unterminated escape sequence in string literal");
                }
                advance();
                continue;
            }
            if (is_newline()) throw LexError(start, "unterminated string literal");
            advance();
        }

        throw LexError(start, "unterminated string literal");
    }

    Token lex_character() {
        const SourcePos start = pos_;
        const std::size_t begin = index_;
        advance();

        if (eof() || is_newline()) throw LexError(start, "unterminated character literal");
        if (peek() == '\\') {
            advance();
            if (eof()) throw LexError(start, "unterminated escape sequence in character literal");
            advance();
        } else {
            advance();
        }

        if (peek() != '\'') {
            throw LexError(start, "character literal must contain exactly one character or escape");
        }

        advance();
        return Token{"CHAR_LITERAL", source_.substr(begin, index_ - begin), start};
    }

    [[nodiscard]] static const std::unordered_set<std::string>& keywords() {
        static const std::unordered_set<std::string> values{
            "type", "alias", "newtype", "interface", "port_interface", "is_type_of",
            "module", "import", "use", "export", "as", "extern", "foreign", "with",
            "if", "elseif", "else", "guard", "switch", "default", "match",
            "loop", "while", "until", "do", "for", "forall", "in",
            "break", "continue", "return",
            "true", "false", "none", "void",
            "ref", "mutable", "move",
            "try", "catch", "transaction", "commit", "rollback",
            "node", "flow", "property", "state", "input", "output",
            "data", "event", "ctrl", "error", "stream", "on", "emit",
            "analysis", "require", "forbid", "warn"
        };
        return values;
    }

    std::string source_;
    std::size_t index_{0};
    SourcePos pos_{};
    SourcePos previous_pos_{};
    bool in_block_comment_{false};
    SourcePos block_comment_start_{};
};

[[nodiscard]] std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    static constexpr char hex[] = "0123456789abcdef";

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

void write_json_line(const Token& token) {
    std::cout
        << "{\"type\":\"" << json_escape(token.type)
        << "\",\"lexeme\":\"" << json_escape(token.lexeme)
        << "\",\"line\":" << token.pos.line
        << ",\"column\":" << token.pos.column
        << "}\n";
}

} // namespace flowcore

int main() {
    try {
        std::string source{
            std::istreambuf_iterator<char>{std::cin},
            std::istreambuf_iterator<char>{}
        };

        flowcore::Lexer lexer{std::move(source)};
        while (true) {
            const flowcore::Token token = lexer.next();
            flowcore::write_json_line(token);
            if (token.type == "EOF") break;
        }
        return EXIT_SUCCESS;
    } catch (const flowcore::LexError& error) {
        std::cerr << "lex error at " << error.pos().line << ':' << error.pos().column
                  << ": " << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "fatal lexer error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
