#include "flowmini_parser.h"

#include "flow_common.h"

#include <cctype>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace flowmini {

namespace {

struct Symbol {
    std::string type;
    bool initialized = false;
};

struct ParsedEndpoint {
    std::string node;
    std::string port; // empty means infer for that side during validation
};

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
                module.nodes.push_back(parseOldNodeDecl("producer"));
            } else if (match(TokenKind::KeywordNode)) {
                module.nodes.push_back(parseOldNodeDecl("node"));
            } else if (match(TokenKind::KeywordSink)) {
                module.nodes.push_back(parseOldNodeDecl("sink"));
            } else if (match(TokenKind::KeywordWire)) {
                module.wires.push_back(parseWireDecl());
            } else if (match(TokenKind::KeywordPolicy)) {
                module.policies.push_back(parsePolicyDecl());
            } else if (looksLikePlacement()) {
                parsePlacement(module);
            } else if (check(TokenKind::Identifier) && lookahead(1).kind == TokenKind::Colon) {
                parseSweetDeclaration(module);
            } else if (looksLikeWireChain()) {
                parseWireChain(module);
            } else {
                fail(peek(), "expected declaration, policy, wire, chain, or placement statement");
            }

            expectLineEnd("expected newline after statement");
        }

        return module;
    }

private:
    [[nodiscard]] const Token& peek() const {
        return tokens_.at(pos_);
    }

    [[nodiscard]] const Token& lookahead(std::size_t offset) const {
        const auto index = pos_ + offset;
        if (index >= tokens_.size()) {
            return tokens_.back();
        }
        return tokens_.at(index);
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

    const Token& expectName(const std::string& message) {
        if (check(TokenKind::Identifier) || check(TokenKind::KeywordTrue) || check(TokenKind::KeywordFalse)) {
            return tokens_.at(pos_++);
        }
        fail(peek(), message);
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

    [[nodiscard]] bool isLineEnd(TokenKind kind) const {
        return kind == TokenKind::Newline || kind == TokenKind::End;
    }

    [[nodiscard]] std::string parseQualifiedName() {
        std::string name = expectIdentifier("expected identifier").text;

        while (match(TokenKind::Dot)) {
            name += ".";
            name += expectName("expected identifier after '.'").text;
        }

        return name;
    }

    [[nodiscard]] Endpoint parseEndpoint() {
        Endpoint endpoint;
        endpoint.node = expectIdentifier("expected endpoint node id").text;
        expect(TokenKind::Dot, "expected '.' in endpoint");
        endpoint.port = expectName("expected endpoint port").text;
        return endpoint;
    }

    [[nodiscard]] ParsedEndpoint parseSweetEndpoint() {
        ParsedEndpoint endpoint;
        endpoint.node = expectIdentifier("expected endpoint node id").text;
        if (match(TokenKind::Dot)) {
            endpoint.port = expectName("expected endpoint port").text;
        }
        return endpoint;
    }

    [[nodiscard]] NodeDecl parseOldNodeDecl(std::string role) {
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

    [[nodiscard]] flow::PolicyValue parsePolicyValue() {
        if (check(TokenKind::String)) {
            return expect(TokenKind::String, "expected string").text;
        }
        if (check(TokenKind::Number)) {
            const auto token = expect(TokenKind::Number, "expected number");
            return parseIntToken(token);
        }
        if (match(TokenKind::KeywordTrue)) {
            return true;
        }
        if (match(TokenKind::KeywordFalse)) {
            return false;
        }
        if (check(TokenKind::Identifier)) {
            return expectIdentifier("expected policy value").text;
        }

        fail(peek(), "expected policy value");
    }

    [[nodiscard]] int parseIntToken(const Token& token) const {
        try {
            std::size_t pos = 0;
            const int value = std::stoi(token.text, &pos);
            if (pos != token.text.size()) {
                fail(token, "invalid integer value");
            }
            return value;
        } catch (const std::invalid_argument&) {
            fail(token, "invalid integer value");
        } catch (const std::out_of_range&) {
            fail(token, "integer value out of range");
        }
    }

    [[nodiscard]] PolicyDecl parsePolicyDecl() {
        PolicyDecl decl;
        decl.node = expectIdentifier("expected policy node id").text;
        expect(TokenKind::Dot, "expected '.' after policy node id");
        decl.key = parseQualifiedName();
        expect(TokenKind::Equals, "expected '=' in policy declaration");
        decl.value = parsePolicyValue();
        return decl;
    }

    [[nodiscard]] bool looksLikeWireChain() const {
        std::size_t i = pos_;
        if (tokens_.at(i).kind != TokenKind::Identifier) {
            return false;
        }
        ++i;
        if (tokens_.at(i).kind == TokenKind::Dot) {
            i += 2;
        }
        return tokens_.at(i).kind == TokenKind::Arrow;
    }

    [[nodiscard]] bool looksLikePlacement() const {
        std::size_t i = pos_;
        bool sawAny = false;
        while (i < tokens_.size() && !isLineEnd(tokens_.at(i).kind)) {
            if (tokens_.at(i).kind == TokenKind::PlaceArrow) {
                return sawAny;
            }
            sawAny = true;
            ++i;
        }
        return false;
    }

    [[nodiscard]] std::string inferRoleForKind(const std::string& kind) const {
        if (kind == "stdin.text") {
            return "producer";
        }
        if (kind == "halt.record") {
            return "sink";
        }
        return "node";
    }

    void addNode(ModuleSpec& module, std::string role, std::string id, std::string kind) {
        module.nodes.push_back(NodeDecl{std::move(role), std::move(id), std::move(kind)});
    }

    void addPolicy(ModuleSpec& module, std::string node, std::string key, flow::PolicyValue value) {
        module.policies.push_back(PolicyDecl{std::move(node), std::move(key), std::move(value)});
    }

    void parseSweetDeclaration(ModuleSpec& module) {
        const std::string id = expectIdentifier("expected declaration id").text;
        expect(TokenKind::Colon, "expected ':' after declaration id");

        if (check(TokenKind::Identifier) && lookahead(0).text == "int" && lookahead(1).kind != TokenKind::Dot) {
            parseIntDeclaration(module, id);
            return;
        }

        if (check(TokenKind::Identifier) && lookahead(0).text == "list" && lookahead(1).kind != TokenKind::Dot) {
            parseListDeclaration(module, id);
            return;
        }

        const std::string kind = parseQualifiedName();
        addNode(module, inferRoleForKind(kind), id, kind);

        if (match(TokenKind::LeftParen)) {
            if (!check(TokenKind::RightParen)) {
                while (true) {
                    const std::string key = expectIdentifier("expected policy key").text;
                    expect(TokenKind::Equals, "expected '=' after policy key");
                    addPolicy(module, id, key, parsePolicyValue());
                    if (!match(TokenKind::Comma)) {
                        break;
                    }
                }
            }
            expect(TokenKind::RightParen, "expected ')' after node arguments");
        }
    }

    void parseIntDeclaration(ModuleSpec& module, const std::string& id) {
        expectIdentifier("expected 'int'");
        if (!match(TokenKind::LeftParen)) {
            fail(peek(), "declaration of '" + id + "' requires initializer");
        }
        const Token& valueToken = expect(TokenKind::Number, "int declaration currently requires integer literal initializer");
        const int value = parseIntToken(valueToken);
        expect(TokenKind::RightParen, "expected ')' after int initializer");

        symbols_[id] = Symbol{"int", true};
        addNode(module, "node", id, "const.int");
        addPolicy(module, id, "out", id);
        addPolicy(module, id, "value", value);
    }

    void parseListDeclaration(ModuleSpec& module, const std::string& id) {
        expectIdentifier("expected 'list'");
        expect(TokenKind::Less, "expected '<' in list<int> declaration");
        const std::string elementType = expectIdentifier("expected list element type").text;
        if (elementType != "int") {
            fail(peek(), "only list<int> is supported in flowmini v5 sugar");
        }
        expect(TokenKind::Greater, "expected '>' in list<int> declaration");
        expect(TokenKind::LeftParen, "list declaration requires initializer");
        expect(TokenKind::LeftBracket, "list<int> initializer must start with '['");

        std::ostringstream values;
        bool first = true;
        if (!check(TokenKind::RightBracket)) {
            while (true) {
                const Token& token = expect(TokenKind::Number, "expected integer in list initializer");
                if (!first) {
                    values << ',';
                }
                values << parseIntToken(token);
                first = false;
                if (!match(TokenKind::Comma)) {
                    break;
                }
            }
        }

        expect(TokenKind::RightBracket, "expected ']' after list initializer");
        expect(TokenKind::RightParen, "expected ')' after list initializer");

        symbols_[id] = Symbol{"list<int>", true};
        addNode(module, "node", id, "list.from_ints");
        addPolicy(module, id, "out", id);
        addPolicy(module, id, "values", values.str());
    }

    void parseWireChain(ModuleSpec& module) {
        std::vector<ParsedEndpoint> endpoints;
        endpoints.push_back(parseSweetEndpoint());
        while (match(TokenKind::Arrow)) {
            endpoints.push_back(parseSweetEndpoint());
        }

        if (endpoints.size() < 2) {
            fail(peek(), "wire chain requires at least two endpoints");
        }

        for (std::size_t i = 0; i + 1 < endpoints.size(); ++i) {
            Endpoint from;
            Endpoint to;

            if (i == 0) {
                from = Endpoint{endpoints[i].node, endpoints[i].port.empty() ? "out" : endpoints[i].port};
            } else {
                // In a chain, a middle endpoint is first a target, then the next edge
                // continues from the same node's default output port.
                from = Endpoint{endpoints[i].node, "out"};
            }

            to = Endpoint{endpoints[i + 1].node, endpoints[i + 1].port.empty() ? "in" : endpoints[i + 1].port};
            module.wires.push_back(WireDecl{std::move(from), std::move(to)});
        }
    }

    enum class ExprKind { LiteralInt, Identifier, Binary };
    struct Expr {
        ExprKind kind = ExprKind::Identifier;
        int literal = 0;
        std::string ident;
        TokenKind op = TokenKind::Plus;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;
    };

    [[nodiscard]] Expr parsePrimaryExpr() {
        if (check(TokenKind::Number)) {
            const auto token = expect(TokenKind::Number, "expected number");
            Expr expr;
            expr.kind = ExprKind::LiteralInt;
            expr.literal = parseIntToken(token);
            return expr;
        }
        if (check(TokenKind::Identifier)) {
            Expr expr;
            expr.kind = ExprKind::Identifier;
            expr.ident = expectIdentifier("expected identifier").text;
            return expr;
        }
        fail(peek(), "expected expression");
    }

    [[nodiscard]] Expr parseBinaryExpr() {
        Expr left = parsePrimaryExpr();
        if (check(TokenKind::Plus) || check(TokenKind::Minus) || check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Percent)) {
            const TokenKind op = peek().kind;
            ++pos_;
            Expr right = parsePrimaryExpr();
            Expr expr;
            expr.kind = ExprKind::Binary;
            expr.op = op;
            expr.left = std::make_unique<Expr>(std::move(left));
            expr.right = std::make_unique<Expr>(std::move(right));
            return expr;
        }
        return left;
    }

    [[nodiscard]] std::string opToKind(TokenKind op) const {
        switch (op) {
            case TokenKind::Plus:    return "int.add";
            case TokenKind::Minus:   return "int.sub";
            case TokenKind::Star:    return "int.mul";
            case TokenKind::Slash:   return "int.div";
            case TokenKind::Percent: return "int.mod";
            default: break;
        }
        throw flow::DiagnosticError{"lowerer", "unsupported binary operator"};
    }

    [[nodiscard]] std::string generatedId(const std::string& base) {
        ++generatedCounter_;
        return "__" + base + "_" + std::to_string(generatedCounter_);
    }

    [[nodiscard]] std::string lowerExprToPath(ModuleSpec& module, const Expr& expr, const std::string& targetHint) {
        if (expr.kind == ExprKind::Identifier) {
            const auto it = symbols_.find(expr.ident);
            if (it == symbols_.end()) {
                throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"};
            }
            return expr.ident;
        }

        if (expr.kind == ExprKind::LiteralInt) {
            const std::string id = generatedId("lit");
            const std::string path = targetHint.empty() ? id : targetHint;
            addNode(module, "node", id, "const.int");
            addPolicy(module, id, "out", path);
            addPolicy(module, id, "value", expr.literal);
            return path;
        }

        const std::string lhs = lowerExprToPath(module, *expr.left, generatedId("tmp"));
        const std::string rhs = lowerExprToPath(module, *expr.right, generatedId("tmp"));
        const std::string id = generatedId("op");
        const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
        addNode(module, "node", id, opToKind(expr.op));
        addPolicy(module, id, "lhs", lhs);
        addPolicy(module, id, "rhs", rhs);
        addPolicy(module, id, "out", out);
        return out;
    }

    [[nodiscard]] std::string exprType(const Expr& expr) const {
        if (expr.kind == ExprKind::LiteralInt) {
            return "int";
        }
        if (expr.kind == ExprKind::Identifier) {
            const auto it = symbols_.find(expr.ident);
            if (it == symbols_.end()) {
                throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"};
            }
            return it->second.type;
        }
        const std::string lhs = exprType(*expr.left);
        const std::string rhs = exprType(*expr.right);
        if (lhs != "int" || rhs != "int") {
            throw flow::DiagnosticError{"lowerer", "binary arithmetic requires int operands"};
        }
        return "int";
    }

    void parsePlacement(ModuleSpec& module) {
        Expr expr = parseBinaryExpr();
        expect(TokenKind::PlaceArrow, "expected '->' in placement");
        const std::string target = expectIdentifier("expected placement target").text;

        const auto targetIt = symbols_.find(target);
        if (targetIt == symbols_.end()) {
            throw flow::DiagnosticError{"lowerer", "cannot assign to undeclared target '" + target + "'"};
        }
        const std::string actual = exprType(expr);
        if (actual != targetIt->second.type) {
            throw flow::DiagnosticError{"lowerer", "type mismatch assigning " + actual + " to " + targetIt->second.type + " target '" + target + "'"};
        }

        if (expr.kind == ExprKind::Identifier) {
            addNode(module, "node", generatedId("copy"), "record.copy");
            const std::string copyId = module.nodes.back().id;
            addPolicy(module, copyId, "from", expr.ident);
            addPolicy(module, copyId, "to", target);
        } else {
            lowerExprToPath(module, expr, target);
        }
        symbols_[target].initialized = true;
    }

    [[noreturn]] void fail(const Token& token, const std::string& message) const {
        std::ostringstream out;
        out << message << " at line " << token.line << ", column " << token.column
            << " near '" << token.text << "'";
        throw flow::DiagnosticError{"parser", out.str()};
    }

    const std::vector<Token>& tokens_;
    std::size_t pos_ = 0;
    std::map<std::string, Symbol> symbols_;
    int generatedCounter_ = 0;
};

std::string policyValueToText(const flow::PolicyValue& value) {
    if (const auto* typed = std::get_if<bool>(&value)) {
        return *typed ? "true" : "false";
    }
    if (const auto* typed = std::get_if<int>(&value)) {
        return std::to_string(*typed);
    }
    if (const auto* typed = std::get_if<std::string>(&value)) {
        bool simple = !typed->empty();
        for (char c : *typed) {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ',')) {
                simple = false;
                break;
            }
        }
        if (simple) {
            return *typed;
        }
        std::string escaped = "\"";
        for (char c : *typed) {
            if (c == '"' || c == '\\') { escaped.push_back('\\'); }
            escaped.push_back(c);
        }
        escaped.push_back('"');
        return escaped;
    }
    return "";
}

} // namespace

ModuleSpec parseModule(const std::vector<Token>& tokens) {
    Parser parser{tokens};
    return parser.parse();
}

void writeFlowIr(const ModuleSpec& module, std::ostream& out) {
    out << "module " << module.name << "\n\n";
    for (const auto& node : module.nodes) {
        out << node.role << ' ' << node.id << " : " << node.kind << "\n";
    }
    if (!module.policies.empty()) {
        out << "\n";
    }
    for (const auto& policy : module.policies) {
        out << "policy " << policy.node << '.' << policy.key << " = " << policyValueToText(policy.value) << "\n";
    }
    if (!module.wires.empty()) {
        out << "\n";
    }
    for (const auto& wire : module.wires) {
        out << "wire " << wire.from.node;
        if (!wire.from.port.empty()) {
            out << '.' << wire.from.port;
        }
        out << " => " << wire.to.node;
        if (!wire.to.port.empty()) {
            out << '.' << wire.to.port;
        }
        out << "\n";
    }
}

} // namespace flowmini
