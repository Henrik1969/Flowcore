#include "flowmini_parser.h"

#include "flow_common.h"

#include <sstream>

namespace flowmini {

namespace {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens)
        : tokens_(tokens) {}

    [[nodiscard]] ModuleSpec parse() {
        skipNewlines();

        expect(TokenKind::KeywordModule, "expected 'module'");
        ModuleSpec module;
        module.name = expectIdentifier("expected module name").text;
        expectLineEnd("expected newline after module declaration");

        while (!check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::End)) {
                break;
            }

            if (match(TokenKind::KeywordProducer)) {
                module.nodes.push_back(parseNodeDecl("producer"));
            } else if (match(TokenKind::KeywordNode)) {
                module.nodes.push_back(parseNodeDecl("node"));
            } else if (match(TokenKind::KeywordSink)) {
                module.nodes.push_back(parseNodeDecl("sink"));
            } else if (match(TokenKind::KeywordWire)) {
                module.wires.push_back(parseWireDecl());
            } else {
                fail(peek(), "expected producer, node, sink, or wire declaration");
            }

            expectLineEnd("expected newline after declaration");
        }

        return module;
    }

private:
    [[nodiscard]] const Token& peek() const {
        return tokens_.at(pos_);
    }

    [[nodiscard]] bool check(TokenKind kind) const {
        return peek().kind == kind;
    }

    [[nodiscard]] bool match(TokenKind kind) {
        if (!check(kind)) {
            return false;
        }

        ++pos_;
        return true;
    }

    const Token& expect(TokenKind kind, const std::string& message) {
        if (!check(kind)) {
            fail(peek(), message);
        }

        return tokens_.at(pos_++);
    }

    const Token& expectIdentifier(const std::string& message) {
        return expect(TokenKind::Identifier, message);
    }

    void skipNewlines() {
        while (check(TokenKind::Newline)) {
            ++pos_;
        }
    }

    void expectLineEnd(const std::string& message) {
        if (check(TokenKind::Newline)) {
            while (check(TokenKind::Newline)) {
                ++pos_;
            }
            return;
        }

        if (check(TokenKind::End)) {
            return;
        }

        fail(peek(), message);
    }

    [[nodiscard]] std::string parseQualifiedName() {
        std::string name = expectIdentifier("expected identifier").text;

        while (match(TokenKind::Dot)) {
            name += ".";
            name += expectIdentifier("expected identifier after '.'").text;
        }

        return name;
    }

    [[nodiscard]] Endpoint parseEndpoint() {
        Endpoint endpoint;
        endpoint.node = expectIdentifier("expected endpoint node id").text;
        expect(TokenKind::Dot, "expected '.' in endpoint");
        endpoint.port = expectIdentifier("expected endpoint port").text;
        return endpoint;
    }

    [[nodiscard]] NodeDecl parseNodeDecl(std::string role) {
        NodeDecl decl;
        decl.role = std::move(role);
        decl.id = expectIdentifier("expected node id").text;
        expect(TokenKind::Colon, "expected ':' after node id");
        decl.kind = parseQualifiedName();
        return decl;
    }

    [[nodiscard]] WireDecl parseWireDecl() {
        WireDecl decl;
        decl.from = parseEndpoint();
        expect(TokenKind::Arrow, "expected '=>' in wire declaration");
        decl.to = parseEndpoint();
        return decl;
    }

    [[noreturn]] void fail(const Token& token, const std::string& message) const {
        std::ostringstream out;
        out << message << " at line " << token.line << ", column " << token.column
            << " near '" << token.text << "'";
        throw flow::DiagnosticError{"parser", out.str()};
    }

    const std::vector<Token>& tokens_;
    std::size_t pos_ = 0;
};

} // namespace

ModuleSpec parseModule(const std::vector<Token>& tokens) {
    Parser parser{tokens};
    return parser.parse();
}

} // namespace flowmini
