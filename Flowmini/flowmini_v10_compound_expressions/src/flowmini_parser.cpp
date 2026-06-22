#include "flowmini_parser.h"

#include "flow_common.h"

#include <cctype>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace flowmini {
namespace {

struct Symbol {
    std::string type;
    std::string path;
    std::string qualifiedName;
};

struct Scope {
    std::string name;
    std::string qualifiedName;
    std::map<std::string, Symbol> symbols;
};

struct ParsedEndpoint {
    std::string node;
    std::string port;
};

struct ChainEnd {
    std::string node;
    std::string port;
};

struct Step {
    bool empty = true;
    bool terminates = false; // true for break/continue or branches where no fallthrough remains
    ChainEnd first;
    ChainEnd last;
};

struct LoopContext {
    ChainEnd continueTarget;
    ChainEnd breakTarget;
};

enum class ExprKind { LiteralInt, Identifier, StdinInt, ListIndex, ListLength, Binary };
struct Expr {
    ExprKind kind = ExprKind::Identifier;
    int literal = 0;
    std::string ident;
    TokenKind op = TokenKind::Plus;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct Target {
    enum class Kind { Identifier, ListIndex } kind = Kind::Identifier;
    std::string ident;
    std::unique_ptr<Expr> index;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    [[nodiscard]] ModuleSpec parse() {
        skipNewlines();
        expect(TokenKind::KeywordModule, "expected 'module'");
        module_.name = expectIdentifier("expected module name").text;
        scopes_.push_back(Scope{module_.name, module_.name, {}});
        expectLineEnd("expected newline after module declaration");

        while (!check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::End)) { break; }

            if (match(TokenKind::KeywordMain)) {
                parseMainBlock();
            } else if (match(TokenKind::KeywordProducer)) {
                module_.nodes.push_back(parseOldNodeDecl("producer"));
            } else if (match(TokenKind::KeywordNode)) {
                module_.nodes.push_back(parseOldNodeDecl("node"));
            } else if (match(TokenKind::KeywordSink)) {
                module_.nodes.push_back(parseOldNodeDecl("sink"));
            } else if (match(TokenKind::KeywordWire)) {
                module_.wires.push_back(parseWireDecl());
            } else if (match(TokenKind::KeywordPolicy)) {
                module_.policies.push_back(parsePolicyDecl());
            } else if (looksLikePlacement()) {
                [[maybe_unused]] const Step ignored = parsePlacementAsStep();
            } else if (check(TokenKind::Identifier) && lookahead(1).kind == TokenKind::Colon) {
                [[maybe_unused]] const Step ignored = parseSweetDeclarationAsStep();
            } else if (looksLikeWireChain()) {
                parseWireChain();
            } else {
                fail(peek(), "expected declaration, policy, wire, chain, placement, or main block");
            }

            expectLineEnd("expected newline after statement");
        }
        return module_;
    }

private:
    [[nodiscard]] const Token& peek() const { return tokens_.at(pos_); }
    [[nodiscard]] const Token& lookahead(std::size_t offset) const {
        const auto index = pos_ + offset;
        return index >= tokens_.size() ? tokens_.back() : tokens_.at(index);
    }
    [[nodiscard]] bool check(TokenKind kind) const { return peek().kind == kind; }
    [[nodiscard]] bool match(TokenKind kind) { if (!check(kind)) { return false; } ++pos_; return true; }
    const Token& expect(TokenKind kind, const std::string& message) { if (!check(kind)) { fail(peek(), message); } return tokens_.at(pos_++); }
    const Token& expectIdentifier(const std::string& message) { return expect(TokenKind::Identifier, message); }
    const Token& expectName(const std::string& message) {
        if (check(TokenKind::Identifier) || check(TokenKind::KeywordTrue) || check(TokenKind::KeywordFalse)) { return tokens_.at(pos_++); }
        fail(peek(), message);
    }
    void skipNewlines() { while (check(TokenKind::Newline)) { ++pos_; } }
    [[nodiscard]] bool isLineEnd(TokenKind kind) const { return kind == TokenKind::Newline || kind == TokenKind::End || kind == TokenKind::RightBrace; }
    void expectLineEnd(const std::string& message) {
        if (check(TokenKind::Newline)) { skipNewlines(); return; }
        if (check(TokenKind::End) || check(TokenKind::RightBrace)) { return; }
        fail(peek(), message);
    }

    [[nodiscard]] std::string parseQualifiedName() {
        std::string name = expectIdentifier("expected identifier").text;
        while (match(TokenKind::Dot)) { name += "."; name += expectName("expected identifier after '.'").text; }
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
        if (match(TokenKind::Dot)) { endpoint.port = expectName("expected endpoint port").text; }
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

    [[nodiscard]] int parseIntToken(const Token& token) const {
        try {
            std::size_t parsed = 0;
            const int value = std::stoi(token.text, &parsed);
            if (parsed != token.text.size()) { fail(token, "invalid integer value"); }
            return value;
        } catch (const std::invalid_argument&) { fail(token, "invalid integer value"); }
          catch (const std::out_of_range&) { fail(token, "integer value out of range"); }
    }

    [[nodiscard]] flow::PolicyValue parsePolicyValue() {
        if (check(TokenKind::String)) { return expect(TokenKind::String, "expected string").text; }
        if (check(TokenKind::Number)) { return parseIntToken(expect(TokenKind::Number, "expected number")); }
        if (match(TokenKind::KeywordTrue)) { return true; }
        if (match(TokenKind::KeywordFalse)) { return false; }
        if (check(TokenKind::Identifier)) { return expectIdentifier("expected policy value").text; }
        fail(peek(), "expected policy value");
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
        if (tokens_.at(i).kind != TokenKind::Identifier) { return false; }
        ++i;
        if (tokens_.at(i).kind == TokenKind::Dot) { i += 2; }
        return tokens_.at(i).kind == TokenKind::Arrow;
    }

    [[nodiscard]] bool looksLikePlacement() const {
        std::size_t i = pos_;
        bool sawAny = false;
        while (i < tokens_.size() && !isLineEnd(tokens_.at(i).kind)) {
            if (tokens_.at(i).kind == TokenKind::PlaceArrow) { return sawAny; }
            sawAny = true;
            ++i;
        }
        return false;
    }

    [[nodiscard]] std::string inferRoleForKind(const std::string& kind) const {
        if (kind == "stdin.text" || kind == "start.record") { return "producer"; }
        if (kind == "halt.record") { return "sink"; }
        return "node";
    }

    void addNode(std::string role, std::string id, std::string kind) { module_.nodes.push_back(NodeDecl{std::move(role), std::move(id), std::move(kind)}); }
    void addPolicy(std::string node, std::string key, flow::PolicyValue value) { module_.policies.push_back(PolicyDecl{std::move(node), std::move(key), std::move(value)}); }
    void addWire(ChainEnd from, ChainEnd to) { module_.wires.push_back(WireDecl{Endpoint{from.node, from.port}, Endpoint{to.node, to.port}}); }

    [[nodiscard]] std::string sanitize(const std::string& in) const {
        std::string out;
        for (char c : in) { out.push_back((std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_'); }
        return out;
    }
    [[nodiscard]] std::string generatedId(const std::string& base) {
        ++generatedCounter_;
        return "__" + sanitize(currentScope().qualifiedName) + "_" + base + "_" + std::to_string(generatedCounter_);
    }
    [[nodiscard]] std::string pathFor(const std::string& id) const { return "__" + sanitize(currentScope().qualifiedName) + "_" + id; }

    [[nodiscard]] Scope& currentScope() { return scopes_.back(); }
    [[nodiscard]] const Scope& currentScope() const { return scopes_.back(); }

    [[nodiscard]] const Symbol* lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            const auto found = it->symbols.find(name);
            if (found != it->symbols.end()) { return &found->second; }
        }
        return nullptr;
    }

    void declareSymbol(const Token& token, const std::string& name, const std::string& type) {
        if (const Symbol* visible = lookup(name)) {
            fail(token, "declaration of '" + name + "' conflicts with visible symbol '" + visible->qualifiedName + "'; implicit shadowing is forbidden");
        }
        currentScope().symbols[name] = Symbol{type, pathFor(name), currentScope().qualifiedName + "::" + name};
    }

    void enterScope(const std::string& name) {
        const std::string q = scopes_.empty() ? name : scopes_.back().qualifiedName + "::" + name;
        scopes_.push_back(Scope{name, q, {}});
    }
    void leaveScope() { scopes_.pop_back(); }

    void parseWireChain() {
        std::vector<ParsedEndpoint> endpoints;
        endpoints.push_back(parseSweetEndpoint());
        while (match(TokenKind::Arrow)) { endpoints.push_back(parseSweetEndpoint()); }
        if (endpoints.size() < 2) { fail(peek(), "wire chain requires at least two endpoints"); }
        for (std::size_t i = 0; i + 1 < endpoints.size(); ++i) {
            ChainEnd from{endpoints[i].node, (i == 0 && !endpoints[i].port.empty()) ? endpoints[i].port : "out"};
            ChainEnd to{endpoints[i + 1].node, endpoints[i + 1].port.empty() ? "in" : endpoints[i + 1].port};
            addWire(from, to);
        }
    }

    // -------- Expressions --------
    [[nodiscard]] Expr parseValuePrimaryExpr() {
        if (check(TokenKind::Number)) {
            Expr expr; expr.kind = ExprKind::LiteralInt; expr.literal = parseIntToken(expect(TokenKind::Number, "expected number")); return expr;
        }
        if (match(TokenKind::LeftParen)) {
            Expr expr = parseValueExpr();
            expect(TokenKind::RightParen, "expected ')' after parenthesized expression");
            return expr;
        }
        if (check(TokenKind::Identifier)) {
            std::string id = expectIdentifier("expected identifier").text;

            if (id == "stdin" && match(TokenKind::Dot)) {
                const std::string fn = expectIdentifier("expected stdin function").text;
                expect(TokenKind::LeftParen, "expected '(' after stdin function");
                expect(TokenKind::RightParen, "expected ')' after stdin function");
                if (fn != "int") { fail(peek(), "only stdin.int() is supported"); }
                Expr expr; expr.kind = ExprKind::StdinInt; return expr;
            }

            if (id == "length" && match(TokenKind::LeftParen)) {
                const std::string listName = expectIdentifier("expected list identifier in length(...)").text;
                expect(TokenKind::RightParen, "expected ')' after length argument");
                Expr expr; expr.kind = ExprKind::ListLength; expr.ident = listName; return expr;
            }

            if (match(TokenKind::LeftBracket)) {
                Expr index = parseValueExpr();
                expect(TokenKind::RightBracket, "expected ']' after list index");
                Expr expr; expr.kind = ExprKind::ListIndex; expr.ident = std::move(id); expr.left = std::make_unique<Expr>(std::move(index)); return expr;
            }

            Expr expr; expr.kind = ExprKind::Identifier; expr.ident = std::move(id); return expr;
        }
        fail(peek(), "expected value expression");
    }

    [[nodiscard]] Expr parseValueMulExpr() {
        Expr left = parseValuePrimaryExpr();
        while (check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Percent)) {
            const TokenKind op = peek().kind; ++pos_;
            Expr right = parseValuePrimaryExpr();
            Expr expr; expr.kind = ExprKind::Binary; expr.op = op; expr.left = std::make_unique<Expr>(std::move(left)); expr.right = std::make_unique<Expr>(std::move(right));
            left = std::move(expr);
        }
        return left;
    }

    [[nodiscard]] Expr parseValueExpr() {
        Expr left = parseValueMulExpr();
        while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
            const TokenKind op = peek().kind; ++pos_;
            Expr right = parseValueMulExpr();
            Expr expr; expr.kind = ExprKind::Binary; expr.op = op; expr.left = std::make_unique<Expr>(std::move(left)); expr.right = std::make_unique<Expr>(std::move(right));
            left = std::move(expr);
        }
        return left;
    }

    [[nodiscard]] Expr parsePredicateExpr() {
        Expr left = parseValueExpr();
        if (check(TokenKind::Greater) || check(TokenKind::Less) || check(TokenKind::EqualEqual)) {
            const TokenKind op = peek().kind; ++pos_;
            Expr right = parseValueExpr();
            Expr expr; expr.kind = ExprKind::Binary; expr.op = op; expr.left = std::make_unique<Expr>(std::move(left)); expr.right = std::make_unique<Expr>(std::move(right)); return expr;
        }
        return left;
    }

    [[nodiscard]] Target parseAssignableTarget() {
        Target target;
        target.ident = expectIdentifier("expected placement target").text;
        if (match(TokenKind::LeftBracket)) {
            target.kind = Target::Kind::ListIndex;
            Expr index = parseValueExpr();
            target.index = std::make_unique<Expr>(std::move(index));
            expect(TokenKind::RightBracket, "expected ']' after target index");
        }
        return target;
    }

    [[nodiscard]] std::string opToKind(TokenKind op) const {
        switch (op) {
            case TokenKind::Plus: return "int.add";
            case TokenKind::Minus: return "int.sub";
            case TokenKind::Star: return "int.mul";
            case TokenKind::Slash: return "int.div";
            case TokenKind::Percent: return "int.mod";
            case TokenKind::Greater: return "int.gt";
            case TokenKind::Less: return "int.lt";
            case TokenKind::EqualEqual: return "int.eq";
            default: break;
        }
        throw flow::DiagnosticError{"lowerer", "unsupported operator in flowmini v10 frontend"};
    }

    [[nodiscard]] std::string exprType(const Expr& expr) const {
        if (expr.kind == ExprKind::LiteralInt || expr.kind == ExprKind::StdinInt || expr.kind == ExprKind::ListLength) { return "int"; }
        if (expr.kind == ExprKind::Identifier) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"}; }
            return sym->type;
        }
        if (expr.kind == ExprKind::ListIndex) {
            const Symbol* listSym = lookup(expr.ident);
            if (listSym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared list '" + expr.ident + "'"}; }
            if (listSym->type != "list<int>") { throw flow::DiagnosticError{"lowerer", "indexed access requires list<int>, got " + listSym->type + " for '" + expr.ident + "'"}; }
            if (exprType(*expr.left) != "int") { throw flow::DiagnosticError{"lowerer", "list index for '" + expr.ident + "' must be int"}; }
            return "int";
        }
        const std::string lhs = exprType(*expr.left);
        const std::string rhs = exprType(*expr.right);
        if (lhs != "int" || rhs != "int") { throw flow::DiagnosticError{"lowerer", "binary operator requires int operands"}; }
        if (expr.op == TokenKind::Greater || expr.op == TokenKind::Less || expr.op == TokenKind::GreaterEqual || expr.op == TokenKind::LessEqual || expr.op == TokenKind::EqualEqual || expr.op == TokenKind::BangEqual) {
            return "bool";
        }
        return "int";
    }

    [[nodiscard]] std::string targetType(const Target& target) const {
        const Symbol* sym = lookup(target.ident);
        if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "cannot assign to undeclared target '" + target.ident + "'"}; }
        if (target.kind == Target::Kind::Identifier) { return sym->type; }
        if (sym->type != "list<int>") { throw flow::DiagnosticError{"lowerer", "indexed assignment requires list<int>, got " + sym->type + " for '" + target.ident + "'"}; }
        if (exprType(*target.index) != "int") { throw flow::DiagnosticError{"lowerer", "list assignment index for '" + target.ident + "' must be int"}; }
        return "int";
    }

    [[nodiscard]] std::string lowerExprToPath(const Expr& expr, const std::string& targetHint, Step* step) {
        if (expr.kind == ExprKind::Identifier) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"}; }
            return sym->path;
        }
        if (expr.kind == ExprKind::ListLength) {
            const Symbol* listSym = lookup(expr.ident);
            if (listSym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared list '" + expr.ident + "'"}; }
            if (listSym->type != "list<int>") { throw flow::DiagnosticError{"lowerer", "length(...) requires list<int>, got " + listSym->type + " for '" + expr.ident + "'"}; }
            const std::string id = generatedId("len");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "list.length");
            addPolicy(id, "list", listSym->path); addPolicy(id, "out", out);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return out;
        }
        if (expr.kind == ExprKind::ListIndex) {
            const Symbol* listSym = lookup(expr.ident);
            if (listSym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared list '" + expr.ident + "'"}; }
            if (listSym->type != "list<int>") { throw flow::DiagnosticError{"lowerer", "indexed access requires list<int>, got " + listSym->type + " for '" + expr.ident + "'"}; }
            Step indexStep;
            const std::string indexPath = lowerExprToPath(*expr.left, generatedId("idx"), &indexStep);
            const std::string id = generatedId("list_get");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "list.get");
            addPolicy(id, "list", listSym->path); addPolicy(id, "index", indexPath); addPolicy(id, "out", out);
            Step combined;
            appendStep(combined, indexStep);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            if (step != nullptr) { appendStep(*step, combined); }
            return out;
        }
        if (expr.kind == ExprKind::LiteralInt) {
            const std::string id = generatedId("lit");
            const std::string path = targetHint.empty() ? id : targetHint;
            addNode("node", id, "const.int"); addPolicy(id, "out", path); addPolicy(id, "value", expr.literal);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return path;
        }
        if (expr.kind == ExprKind::StdinInt) {
            const std::string stdinId = generatedId("stdin");
            const std::string parseId = generatedId("parse_int");
            const std::string path = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("producer", stdinId, "stdin.text");
            addNode("node", parseId, "parse.int.to_record"); addPolicy(parseId, "out", path);
            addWire({stdinId, "out"}, {parseId, "in"});
            if (step != nullptr) { appendStep(*step, Step{false, false, {stdinId, "out"}, {parseId, "out"}}); }
            return path;
        }

        Step leftStep;
        Step rightStep;
        const std::string lhs = lowerExprToPath(*expr.left, generatedId("tmp"), &leftStep);
        const std::string rhs = lowerExprToPath(*expr.right, generatedId("tmp"), &rightStep);
        const std::string id = generatedId("op");
        const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
        addNode("node", id, opToKind(expr.op));
        addPolicy(id, "lhs", lhs); addPolicy(id, "rhs", rhs); addPolicy(id, "out", out);
        Step opStep{false, false, {id, "in"}, {id, "out"}};
        Step combined;
        appendStep(combined, leftStep);
        appendStep(combined, rightStep);
        appendStep(combined, opStep);
        if (step != nullptr) { appendStep(*step, combined); }
        return out;
    }

    void appendStep(Step& seq, const Step& next) {
        if (next.empty) { return; }
        if (seq.empty) { seq = next; return; }
        if (seq.terminates) {
            throw flow::DiagnosticError{"lowerer", "unreachable statement after break/continue"};
        }
        addWire(seq.last, next.first);
        seq.last = next.last;
        seq.terminates = next.terminates;
    }

    // -------- Declarations and statements --------
    [[nodiscard]] Step parseSweetDeclarationAsStep() {
        const Token& idToken = expectIdentifier("expected declaration id");
        const std::string id = idToken.text;
        expect(TokenKind::Colon, "expected ':' after declaration id");

        if (check(TokenKind::Identifier) && peek().text == "int" && lookahead(1).kind != TokenKind::Dot) {
            return parseIntDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && peek().text == "list" && lookahead(1).kind != TokenKind::Dot) {
            return parseListDeclaration(idToken, id);
        }

        const std::string kind = parseQualifiedName();
        addNode(inferRoleForKind(kind), id, kind);
        if (match(TokenKind::LeftParen)) {
            if (!check(TokenKind::RightParen)) {
                while (true) {
                    const std::string key = expectIdentifier("expected policy key").text;
                    expect(TokenKind::Equals, "expected '=' after policy key");
                    addPolicy(id, key, parsePolicyValue());
                    if (!match(TokenKind::Comma)) { break; }
                }
            }
            expect(TokenKind::RightParen, "expected ')' after node arguments");
        }
        return Step{}; // graph declaration only; explicit chains still wire it
    }

    [[nodiscard]] Step parseIntDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'int'");
        if (!match(TokenKind::LeftParen)) { fail(peek(), "declaration of '" + id + "' requires initializer"); }
        Expr initializer = parseValueExpr();
        expect(TokenKind::RightParen, "expected ')' after int initializer");
        if (exprType(initializer) != "int") { throw flow::DiagnosticError{"lowerer", "initializer for int '" + id + "' is not int"}; }
        // At module/root level, keep v5 graph-sugar compatibility: `zero : int(0)` creates a node named `zero`.
        if (scopes_.size() == 1 && initializer.kind == ExprKind::LiteralInt) {
            declareSymbol(idToken, id, "int");
            currentScope().symbols[id].path = id;
            addNode("node", id, "const.int");
            addPolicy(id, "out", id);
            addPolicy(id, "value", initializer.literal);
            return Step{};
        }
        declareSymbol(idToken, id, "int");
        const std::string path = lookup(id)->path;
        Step step;
        static_cast<void>(lowerExprToPath(initializer, path, &step));
        return step;
    }

    [[nodiscard]] Step parseListDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'list'");
        expect(TokenKind::Less, "expected '<' in list<int> declaration");
        const std::string elementType = expectIdentifier("expected list element type").text;
        if (elementType != "int") { fail(peek(), "only list<int> is supported in flowmini v10 sugar"); }
        expect(TokenKind::Greater, "expected '>' in list<int> declaration");
        expect(TokenKind::LeftParen, "list declaration requires initializer");
        expect(TokenKind::LeftBracket, "list<int> initializer must start with '['");
        std::ostringstream values;
        bool first = true;
        if (!check(TokenKind::RightBracket)) {
            while (true) {
                const Token& token = expect(TokenKind::Number, "expected integer in list initializer");
                if (!first) { values << ','; }
                values << parseIntToken(token); first = false;
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightBracket, "expected ']' after list initializer");
        expect(TokenKind::RightParen, "expected ')' after list initializer");
        declareSymbol(idToken, id, "list<int>");
        if (scopes_.size() == 1) {
            currentScope().symbols[id].path = id;
            addNode("node", id, "list.from_ints"); addPolicy(id, "out", id); addPolicy(id, "values", values.str());
            return Step{};
        }
        const std::string nodeId = generatedId("list");
        addNode("node", nodeId, "list.from_ints"); addPolicy(nodeId, "out", lookup(id)->path); addPolicy(nodeId, "values", values.str());
        return Step{false, false, {nodeId, "in"}, {nodeId, "out"}};
    }

    [[nodiscard]] Step lowerAssignmentToTarget(const Expr& expr, const Target& target) {
        const std::string actual = exprType(expr);
        const std::string wanted = targetType(target);
        if (actual != wanted) { throw flow::DiagnosticError{"lowerer", "type mismatch assigning " + actual + " to " + wanted + " target '" + target.ident + "'"}; }

        if (target.kind == Target::Kind::Identifier) {
            const Symbol* targetSym = lookup(target.ident);
            Step step;
            if (expr.kind == ExprKind::Identifier) {
                const std::string id = generatedId("copy");
                addNode("node", id, "record.copy");
                addPolicy(id, "from", lookup(expr.ident)->path); addPolicy(id, "to", targetSym->path);
                step = Step{false, false, {id, "in"}, {id, "out"}};
            } else {
                static_cast<void>(lowerExprToPath(expr, targetSym->path, &step));
            }
            return step;
        }

        const Symbol* listSym = lookup(target.ident);
        Step valueStep;
        const std::string valuePath = lowerExprToPath(expr, generatedId("set_value"), &valueStep);
        Step indexStep;
        const std::string indexPath = lowerExprToPath(*target.index, generatedId("set_index"), &indexStep);
        const std::string id = generatedId("list_set");
        addNode("node", id, "list.set");
        addPolicy(id, "list", listSym->path); addPolicy(id, "index", indexPath); addPolicy(id, "value", valuePath);
        Step combined;
        appendStep(combined, valueStep);
        appendStep(combined, indexStep);
        appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
        return combined;
    }

    [[nodiscard]] Step parsePlacementAsStep() {
        Expr expr = parseValueExpr();
        expect(TokenKind::PlaceArrow, "expected '->' in placement");
        Target target = parseAssignableTarget();
        return lowerAssignmentToTarget(expr, target);
    }

    [[nodiscard]] Step parsePrintStatement() {
        expectIdentifier("expected 'print'");
        Expr expr = parseValueExpr();
        const std::string type = exprType(expr);
        Step step;
        if (type == "int") {
            std::string path;
            if (expr.kind == ExprKind::Identifier) { path = lookup(expr.ident)->path; }
            else { path = lowerExprToPath(expr, generatedId("print_tmp"), &step); }
            const std::string id = generatedId("print");
            addNode("node", id, "stdout.int_line"); addPolicy(id, "path", path);
            appendStep(step, Step{false, false, {id, "in"}, {id, "out"}});
            return step;
        }
        if (type == "list<int>" && expr.kind == ExprKind::Identifier) {
            const std::string id = generatedId("print_list");
            addNode("node", id, "list.print_int_lines"); addPolicy(id, "list", lookup(expr.ident)->path);
            return Step{false, false, {id, "in"}, {id, "out"}};
        }
        throw flow::DiagnosticError{"lowerer", "print currently supports int expressions and list<int> identifiers only"};
    }

    [[nodiscard]] Step parseIfStatement() {
        expect(TokenKind::KeywordIf, "expected 'if'");
        const int ifIndex = ++ifCounter_;

        Expr cond = parsePredicateExpr();
        if (exprType(cond) != "bool") { throw flow::DiagnosticError{"lowerer", "if condition must be bool"}; }
        expect(TokenKind::LeftBrace, "expected '{' after if condition");
        skipNewlines();

        Step condStep;
        const std::string condPath = lowerExprToPath(cond, generatedId("if" + std::to_string(ifIndex) + "_cond_path"), &condStep);
        const std::string routeId = generatedId("if" + std::to_string(ifIndex) + "_route");
        addNode("node", routeId, "route.bool");
        addPolicy(routeId, "path", condPath);

        Step start = condStep;
        appendStep(start, Step{false, false, {routeId, "in"}, {routeId, "false"}});

        enterScope("if" + std::to_string(ifIndex) + "_true");
        Step trueBody = parseBlockStatementsUntilRightBrace();
        leaveScope();
        if (trueBody.empty) { fail(peek(), "if true block may not be empty in flowmini v10"); }

        const std::size_t afterTrueBrace = pos_;
        skipNewlines();
        Step falseBody;
        const bool hasElse = match(TokenKind::KeywordElse);
        if (!hasElse) { pos_ = afterTrueBrace; }
        if (hasElse) {
            expect(TokenKind::LeftBrace, "expected '{' after else");
            skipNewlines();
            enterScope("if" + std::to_string(ifIndex) + "_false");
            falseBody = parseBlockStatementsUntilRightBrace();
            leaveScope();
            if (falseBody.empty) { fail(peek(), "else block may not be empty in flowmini v10"); }
        }

        const std::string joinId = generatedId("if" + std::to_string(ifIndex) + "_join");
        addNode("node", joinId, "record.nop");

        addWire({routeId, "true"}, trueBody.first);
        if (!trueBody.terminates) { addWire(trueBody.last, {joinId, "in"}); }

        bool ifTerminates = false;
        if (hasElse) {
            addWire({routeId, "false"}, falseBody.first);
            if (!falseBody.terminates) { addWire(falseBody.last, {joinId, "in"}); }
            ifTerminates = trueBody.terminates && falseBody.terminates;
        } else {
            addWire({routeId, "false"}, {joinId, "in"});
            ifTerminates = false;
        }

        return Step{false, ifTerminates, start.first, {joinId, "out"}};
    }

    [[nodiscard]] Step parseWhileStatement() {
        expect(TokenKind::KeywordWhile, "expected 'while'");
        const int whileIndex = ++whileCounter_;
        enterScope("while" + std::to_string(whileIndex));
        Expr cond = parsePredicateExpr();
        if (exprType(cond) != "bool") { throw flow::DiagnosticError{"lowerer", "while condition must be bool"}; }
        expect(TokenKind::LeftBrace, "expected '{' after while condition");
        skipNewlines();

        Step condStep;
        const std::string condPath = lowerExprToPath(cond, generatedId("cond_path"), &condStep);
        const std::string routeId = generatedId("route");
        const std::string exitJoinId = generatedId("while" + std::to_string(whileIndex) + "_exit");
        addNode("node", routeId, "route.bool"); addPolicy(routeId, "path", condPath);
        addNode("node", exitJoinId, "record.nop");
        appendStep(condStep, Step{false, false, {routeId, "in"}, {routeId, "false"}});
        addWire({routeId, "false"}, {exitJoinId, "in"});

        loopStack_.push_back(LoopContext{condStep.first, {exitJoinId, "in"}});
        Step body = parseBlockStatementsUntilRightBrace();
        loopStack_.pop_back();

        if (body.empty) { fail(peek(), "while body may not be empty in flowmini v10"); }
        addWire({routeId, "true"}, body.first);
        if (!body.terminates) { addWire(body.last, condStep.first); }
        leaveScope();
        return Step{false, false, condStep.first, {exitJoinId, "out"}};
    }

    [[nodiscard]] Step parseBreakStatement() {
        expect(TokenKind::KeywordBreak, "expected 'break'");
        if (loopStack_.empty()) { throw flow::DiagnosticError{"lowerer", "'break' is only valid inside a loop"}; }
        const std::string id = generatedId("break");
        addNode("node", id, "record.nop");
        addWire({id, "out"}, loopStack_.back().breakTarget);
        return Step{false, true, {id, "in"}, {id, "out"}};
    }

    [[nodiscard]] Step parseContinueStatement() {
        expect(TokenKind::KeywordContinue, "expected 'continue'");
        if (loopStack_.empty()) { throw flow::DiagnosticError{"lowerer", "'continue' is only valid inside a loop"}; }
        const std::string id = generatedId("continue");
        addNode("node", id, "record.nop");
        addWire({id, "out"}, loopStack_.back().continueTarget);
        return Step{false, true, {id, "in"}, {id, "out"}};
    }

    [[nodiscard]] Step parseStatementInBlock() {
        if (check(TokenKind::Identifier) && peek().text == "print") { Step s = parsePrintStatement(); expectLineEnd("expected newline after print"); return s; }
        if (check(TokenKind::KeywordWhile)) { Step s = parseWhileStatement(); expectLineEnd("expected newline after while block"); return s; }
        if (check(TokenKind::KeywordIf)) { Step s = parseIfStatement(); expectLineEnd("expected newline after if block"); return s; }
        if (check(TokenKind::KeywordBreak)) { Step s = parseBreakStatement(); expectLineEnd("expected newline after break"); return s; }
        if (check(TokenKind::KeywordContinue)) { Step s = parseContinueStatement(); expectLineEnd("expected newline after continue"); return s; }
        if (looksLikePlacement()) { Step s = parsePlacementAsStep(); expectLineEnd("expected newline after placement"); return s; }
        if (check(TokenKind::Identifier) && lookahead(1).kind == TokenKind::Colon) { Step s = parseSweetDeclarationAsStep(); expectLineEnd("expected newline after declaration"); return s; }
        fail(peek(), "expected block statement");
    }

    [[nodiscard]] Step parseBlockStatementsUntilRightBrace() {
        Step seq;
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::RightBrace) || check(TokenKind::End)) { break; }
            appendStep(seq, parseStatementInBlock());
        }
        expect(TokenKind::RightBrace, "expected '}' to close block");
        return seq;
    }

    void parseMainBlock() {
        enterScope("main");
        expect(TokenKind::LeftBrace, "expected '{' after main");
        skipNewlines();
        Step body = parseBlockStatementsUntilRightBrace();
        if (body.empty) { fail(peek(), "main block may not be empty"); }

        bool hasProducer = false;
        for (const auto& node : module_.nodes) {
            if (node.role == "producer") { hasProducer = true; break; }
        }
        if (!hasProducer) {
            const std::string startId = generatedId("start");
            addNode("producer", startId, "start.record");
            addWire({startId, "out"}, body.first);
        }

        const std::string haltId = generatedId("halt");
        addNode("sink", haltId, "halt.record");
        addWire(body.last, {haltId, "in"});
        leaveScope();
    }

    [[noreturn]] void fail(const Token& token, const std::string& message) const {
        std::ostringstream out;
        out << message << " at line " << token.line << ", column " << token.column << " near '" << token.text << "'";
        throw flow::DiagnosticError{"parser", out.str()};
    }

    const std::vector<Token>& tokens_;
    std::size_t pos_ = 0;
    ModuleSpec module_;
    std::vector<Scope> scopes_;
    int generatedCounter_ = 0;
    int whileCounter_ = 0;
    int ifCounter_ = 0;
    std::vector<LoopContext> loopStack_;
};

std::string policyValueToText(const flow::PolicyValue& value) {
    if (const auto* typed = std::get_if<bool>(&value)) { return *typed ? "true" : "false"; }
    if (const auto* typed = std::get_if<int>(&value)) { return std::to_string(*typed); }
    if (const auto* typed = std::get_if<std::string>(&value)) {
        bool simple = !typed->empty();
        for (char c : *typed) {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ',')) { simple = false; break; }
        }
        if (simple) { return *typed; }
        std::string escaped = "\"";
        for (char c : *typed) { if (c == '"' || c == '\\') { escaped.push_back('\\'); } escaped.push_back(c); }
        escaped.push_back('"'); return escaped;
    }
    return "";
}

} // namespace

ModuleSpec parseModule(const std::vector<Token>& tokens) { Parser parser{tokens}; return parser.parse(); }

void writeFlowIr(const ModuleSpec& module, std::ostream& out) {
    out << "module " << module.name << "\n\n";
    for (const auto& node : module.nodes) { out << node.role << ' ' << node.id << " : " << node.kind << "\n"; }
    if (!module.policies.empty()) { out << "\n"; }
    for (const auto& policy : module.policies) { out << "policy " << policy.node << '.' << policy.key << " = " << policyValueToText(policy.value) << "\n"; }
    if (!module.wires.empty()) { out << "\n"; }
    for (const auto& wire : module.wires) {
        out << "wire " << wire.from.node; if (!wire.from.port.empty()) { out << '.' << wire.from.port; }
        out << " => " << wire.to.node; if (!wire.to.port.empty()) { out << '.' << wire.to.port; }
        out << "\n";
    }
}

} // namespace flowmini
