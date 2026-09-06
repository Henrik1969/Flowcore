#include "flowmini_parser.h"

#include "flow_common.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <set>
#include <utility>
#include <vector>

namespace flowmini {
namespace {

struct Symbol {
    std::string type;
    std::string path;
    std::string qualifiedName;
    std::vector<int> shape;
    bool immutable = false;
    std::optional<int> constantInt;
    std::optional<bool> constantBool;
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

struct WhenCase {
    std::optional<int> value;
    int high = 0;
    Step body;
};

struct LoopContext {
    ChainEnd continueTarget;
    ChainEnd breakTarget;
};

struct FunctionArg {
    std::string name;
    std::string type;
};

struct FunctionDef {
    std::string name;
    std::vector<FunctionArg> args;
    std::string returnType;
    std::size_t bodyStart = 0;
    std::size_t bodyEnd = 0;
    bool isExtern = false;
    std::string library;
    std::string convention;
    std::string symbol;
    std::string effect;
};

struct TypeInvariant {
    TokenKind op = TokenKind::EqualEqual;
    int value = 0;
};

struct TypeField {
    std::string name;
    std::string type;
};

struct TypeDef {
    std::string name;
    std::string base;
    std::vector<TypeInvariant> invariants;
    std::vector<TypeField> fields;
    bool isEnum = false;
    std::map<std::string, int> enumMembers;
    bool isVariant = false;
    std::map<std::string, std::vector<TypeField>> variantMembers;
};

struct AbiTypeDef {
    std::string name;
    std::string repr;
    std::string ownership;
    std::string access;
    std::string lifetime;
    std::string nullable;
    std::string terminator;
    std::string cleanup;
    bool opaque = false;
};

struct AbiStructFieldDef {
    std::string name;
    std::string type;
};

struct AbiStructDef {
    std::string name;
    std::vector<AbiStructFieldDef> fields;
};

enum class ExprKind { LiteralInt, LiteralBool, LiteralString, Identifier, StdinInt, StdinBytes, ListIndex, ArrayIndex, FieldAccess, ListLength, FunctionCall, VariantConstruct, UnaryNot, Binary };
struct Expr {
    ExprKind kind = ExprKind::Identifier;
    int literal = 0;
    bool boolLiteral = false;
    std::string stringLiteral;
    std::string ident;
    TokenKind op = TokenKind::Plus;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::vector<Expr> indices;
    std::vector<std::string> fields;
    std::vector<Expr> args;
};

struct Target {
    enum class Kind { Identifier, ListIndex, ArrayIndex, FieldAccess } kind = Kind::Identifier;
    std::string ident;
    std::unique_ptr<Expr> index;
    std::vector<Expr> indices;
    std::vector<std::string> fields;
    std::vector<Expr> args;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {
        types_.emplace("int", TypeDef{"int", "", {}, {}, false, {}, false, {}});
        types_.emplace("Bool", TypeDef{"Bool", "", {}, {}, false, {}, false, {}});
        types_.emplace("c_int", TypeDef{"c_int", "int", {}, {}, false, {}, false, {}});
        types_.emplace("c_long", TypeDef{"c_long", "int", {}, {}, false, {}, false, {}});
        types_.emplace("c_ulong", TypeDef{"c_ulong", "int", {}, {}, false, {}, false, {}});
        types_.emplace("c_size_t", TypeDef{"c_size_t", "int", {{TokenKind::GreaterEqual, 0}}, {}, false, {}, false, {}});
        types_.emplace("c_string", TypeDef{"c_string", "", {}, {}, false, {}, false, {}});
        abiTypes_.emplace("c_string", AbiTypeDef{"c_string", "const char*", "borrowed", "read", "call", "false", "nul", {}, false});
    }

    [[nodiscard]] ModuleSpec parse() {
        skipNewlines();
        if (!(match(TokenKind::KeywordProgram) || match(TokenKind::KeywordUnit) || match(TokenKind::KeywordModule))) {
            fail(peek(), "expected 'program', 'unit', or legacy 'module'");
        }
        module_.name = expectIdentifier("expected unit/program name").text;
        scopes_.push_back(Scope{module_.name, module_.name, {}});
        expectLineEnd("expected newline after unit/program declaration");

        while (!check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::End)) { break; }

            if (match(TokenKind::KeywordMain)) {
                if (mainSeen_) { fail(peek(), "multiple main definitions found; exactly one main is allowed"); }
                mainSeen_ = true;
                parseMainBlock();
            } else if (check(TokenKind::Identifier) && peek().text == "type") {
                parseTypeDecl();
            } else if (check(TokenKind::Identifier) && peek().text == "enum") {
                parseEnumDecl();
            } else if (check(TokenKind::Identifier) && peek().text == "variant") {
                parseVariantDecl();
            } else if (check(TokenKind::Identifier) && peek().text == "abi") {
                parseAbiBlock();
            } else if (match(TokenKind::KeywordFn)) {
                parseFunctionDecl();
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

    [[nodiscard]] const FunctionDef* lookupFunction(const std::string& name) const {
        const auto found = functions_.find(name);
        if (found == functions_.end()) { return nullptr; }
        return &found->second;
    }

    [[nodiscard]] const TypeDef* lookupType(const std::string& name) const {
        const auto found = types_.find(name);
        if (found == types_.end()) { return nullptr; }
        return &found->second;
    }

    [[nodiscard]] bool typeExists(const std::string& name) const { return lookupType(name) != nullptr; }

    [[nodiscard]] bool isIntLikeType(const std::string& type) const {
        std::set<std::string> seen;
        std::string cur = type;
        while (!cur.empty()) {
            if (cur == "int") { return true; }
            if (!seen.insert(cur).second) { return false; }
            const TypeDef* def = lookupType(cur);
            if (def == nullptr) { return false; }
            cur = def->base;
        }
        return false;
    }

    [[nodiscard]] bool isAbiPointerType(const std::string& type) const {
        return abiTypes_.find(type) != abiTypes_.end();
    }

    [[nodiscard]] bool isAbiStructType(const std::string& type) const {
        return abiStructs_.find(type) != abiStructs_.end();
    }

    [[nodiscard]] bool isRecordType(const std::string& type) const {
        const TypeDef* def = lookupType(type);
        return def != nullptr && !def->fields.empty();
    }

    [[nodiscard]] bool isEnumType(const std::string& type) const {
        const TypeDef* def = lookupType(type);
        return def != nullptr && def->isEnum;
    }

    [[nodiscard]] bool isVariantType(const std::string& type) const {
        const TypeDef* def = lookupType(type);
        return def != nullptr && def->isVariant;
    }

    [[nodiscard]] const TypeField* findTypeField(const std::string& type, const std::string& field) const {
        const TypeDef* def = lookupType(type);
        if (def == nullptr) { return nullptr; }
        for (const auto& f : def->fields) {
            if (f.name == field) { return &f; }
        }
        return nullptr;
    }

    [[nodiscard]] std::string fieldAccessType(const std::string& rootName, const std::vector<std::string>& fields) const {
        const Symbol* sym = lookup(rootName);
        if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared field value '" + rootName + "'"}; }
        std::string curType = sym->type;
        for (const auto& field : fields) {
            if (isVariantType(curType) && field == "__tag") { curType = "int"; continue; }
            const TypeField* f = findTypeField(curType, field);
            TypeField variantField;
            if (f == nullptr && isVariantType(curType)) {
                const TypeDef* variant = lookupType(curType);
                if (curType == activeVariantArmType_ && !activeVariantArmMember_.empty()) {
                    const auto active = variant->variantMembers.find(activeVariantArmMember_);
                    if (active != variant->variantMembers.end()) {
                        bool activeHasField = false;
                        for (const auto& candidate : active->second) if (candidate.name == field) activeHasField = true;
                        if (!activeHasField) {
                            throw flow::DiagnosticError{"lowerer", "variant payload field '" + field + "' is not available in active member '" + activeVariantArmMember_ + "'"};
                        }
                    }
                }
                for (const auto& [member, payload] : variant->variantMembers) {
                    for (const auto& candidate : payload) {
                        if (candidate.name == field) { variantField = candidate; break; }
                    }
                    if (!variantField.name.empty()) { break; }
                }
                if (!variantField.name.empty()) { f = &variantField; }
            }
            if (f == nullptr) {
                throw flow::DiagnosticError{"lowerer", "type '" + curType + "' has no field '" + field + "'"};
            }
            curType = f->type;
        }
        return curType;
    }

    [[nodiscard]] std::string joinFields(const std::vector<std::string>& fields) const {
        std::ostringstream out;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) { out << ','; }
            out << fields[i];
        }
        return out.str();
    }

    [[nodiscard]] bool isCAbiType(const std::string& type) const {
        return type == "c_int" || type == "c_long" || type == "c_ulong" || type == "c_size_t" || isAbiPointerType(type) || isAbiStructType(type);
    }

    [[nodiscard]] bool isCallableType(const std::string& type) const {
        return isIntLikeType(type) || type == "Bool" || type == "c_string" || isAbiStructType(type) || isRecordType(type) || isEnumType(type);
    }

    [[nodiscard]] bool isSubtypeOf(const std::string& actual, const std::string& wanted) const {
        if (actual == wanted) { return true; }
        std::set<std::string> seen;
        std::string cur = actual;
        while (!cur.empty()) {
            if (!seen.insert(cur).second) { return false; }
            const TypeDef* def = lookupType(cur);
            if (def == nullptr) { return false; }
            cur = def->base;
            if (cur == wanted) { return true; }
        }
        return false;
    }

    [[nodiscard]] bool typeHasOwnInvariants(const std::string& type) const {
        const TypeDef* def = lookupType(type);
        return def != nullptr && !def->invariants.empty();
    }

    [[nodiscard]] bool valueSatisfiesInvariant(int value, const TypeInvariant& inv) const {
        switch (inv.op) {
            case TokenKind::Greater: return value > inv.value;
            case TokenKind::Less: return value < inv.value;
            case TokenKind::GreaterEqual: return value >= inv.value;
            case TokenKind::LessEqual: return value <= inv.value;
            case TokenKind::EqualEqual: return value == inv.value;
            case TokenKind::BangEqual: return value != inv.value;
            default: return false;
        }
    }

    [[nodiscard]] bool literalSatisfiesType(int value, const std::string& type) const {
        const TypeDef* def = lookupType(type);
        if (def == nullptr) { return false; }
        if (!def->base.empty() && !literalSatisfiesType(value, def->base)) { return false; }
        for (const auto& inv : def->invariants) { if (!valueSatisfiesInvariant(value, inv)) { return false; } }
        return true;
    }

    [[nodiscard]] bool typeHasAnyInvariants(const std::string& type) const {
        const TypeDef* def = lookupType(type);
        if (def == nullptr) { return false; }
        if (!def->invariants.empty()) { return true; }
        return !def->base.empty() && typeHasAnyInvariants(def->base);
    }

    [[nodiscard]] bool canAssignType(const std::string& actual, const std::string& wanted) const {
        if (actual == wanted) { return true; }
        if (isSubtypeOf(actual, wanted)) { return true; }
        // int may flow into invariant-free semantic aliases such as Integer refines int.
        if (actual == "int" && isIntLikeType(wanted) && !typeHasAnyInvariants(wanted)) { return true; }
        return false;
    }

    void declareSymbol(const Token& token, const std::string& name, const std::string& type) {
        if (const Symbol* visible = lookup(name)) {
            fail(token, "declaration of '" + name + "' conflicts with visible symbol '" + visible->qualifiedName + "'; implicit shadowing is forbidden");
        }
        currentScope().symbols[name] = Symbol{type, pathFor(name), currentScope().qualifiedName + "::" + name, {}, declaringConstant_, {}, {}};
    }


    void declareArraySymbol(const Token& token, const std::string& name, std::vector<int> shape) {
        if (const Symbol* visible = lookup(name)) {
            fail(token, "declaration of '" + name + "' conflicts with visible symbol '" + visible->qualifiedName + "'; implicit shadowing is forbidden");
        }
        currentScope().symbols[name] = Symbol{"array<int>", pathFor(name), currentScope().qualifiedName + "::" + name, std::move(shape), declaringConstant_, {}, {}};
    }

    [[nodiscard]] bool isArrayIntType(const std::string& type) const { return type == "array<int>"; }
    [[nodiscard]] std::string joinInts(const std::vector<int>& values) const {
        std::ostringstream out;
        for (std::size_t i = 0; i < values.size(); ++i) { if (i > 0) { out << ','; } out << values[i]; }
        return out.str();
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
        if (match(TokenKind::KeywordTrue)) {
            Expr expr; expr.kind = ExprKind::LiteralBool; expr.boolLiteral = true; return expr;
        }
        if (match(TokenKind::KeywordFalse)) {
            Expr expr; expr.kind = ExprKind::LiteralBool; expr.boolLiteral = false; return expr;
        }
        if (check(TokenKind::String)) {
            Expr expr; expr.kind = ExprKind::LiteralString; expr.stringLiteral = expect(TokenKind::String, "expected string").text; return expr;
        }
        if (check(TokenKind::Number)) {
            Expr expr; expr.kind = ExprKind::LiteralInt; expr.literal = parseIntToken(expect(TokenKind::Number, "expected number")); return expr;
        }
        if (match(TokenKind::Minus)) {
            const Token& token = expect(TokenKind::Number, "expected number after unary '-'");
            Expr expr; expr.kind = ExprKind::LiteralInt; expr.literal = -parseIntToken(token); return expr;
        }
        if (match(TokenKind::LeftParen)) {
            Expr expr = parsePredicateExpr();
            expect(TokenKind::RightParen, "expected ')' after parenthesized expression");
            return expr;
        }
        if (check(TokenKind::Identifier)) {
            std::string id = expectIdentifier("expected identifier").text;

            if (id == "stdin" && match(TokenKind::Dot)) {
                const std::string fn = expectIdentifier("expected stdin function").text;
                expect(TokenKind::LeftParen, "expected '(' after stdin function");
                expect(TokenKind::RightParen, "expected ')' after stdin function");
                if (fn == "int") { Expr expr; expr.kind = ExprKind::StdinInt; return expr; }
                if (fn == "bytes") { Expr expr; expr.kind = ExprKind::StdinBytes; return expr; }
                fail(peek(), "only stdin.int() and stdin.bytes() are supported");
            }

            if (id == "length" && match(TokenKind::LeftParen)) {
                const std::string listName = expectIdentifier("expected list identifier in length(...)").text;
                expect(TokenKind::RightParen, "expected ')' after length argument");
                Expr expr; expr.kind = ExprKind::ListLength; expr.ident = listName; return expr;
            }

            if (isEnumType(id) && match(TokenKind::Dot)) {
                const Token& member = expectIdentifier("expected enum member after '.'");
                const TypeDef* def = lookupType(id);
                if (def->enumMembers.find(member.text) == def->enumMembers.end()) {
                    fail(member, "unknown enum member '" + member.text + "'");
                }
                Expr expr;
                expr.kind = ExprKind::Identifier;
                expr.ident = member.text;
                return expr;
            }

            if (isVariantType(id) && match(TokenKind::Dot)) {
                const Token& member = expectIdentifier("expected variant member after '.'");
                const TypeDef* def = lookupType(id);
                if (def->variantMembers.find(member.text) == def->variantMembers.end()) {
                    fail(member, "unknown variant member '" + member.text + "'");
                }
                expect(TokenKind::LeftParen, "variant construction requires payload parentheses");
                Expr expr;
                expr.kind = ExprKind::VariantConstruct;
                expr.ident = id;
                expr.fields.push_back(member.text);
                if (!check(TokenKind::RightParen)) {
                    while (true) {
                        expr.args.push_back(parsePredicateExpr());
                        if (!match(TokenKind::Comma)) { break; }
                    }
                }
                expect(TokenKind::RightParen, "expected ')' after variant payload");
                return expr;
            }

            if (match(TokenKind::Dot)) {
                Expr expr;
                expr.ident = std::move(id);
                expr.fields.push_back(expectIdentifier("expected field name after '.'").text);
                while (match(TokenKind::Dot)) { expr.fields.push_back(expectIdentifier("expected field name after '.'").text); }
                if (match(TokenKind::LeftParen)) {
                    expr.kind = ExprKind::FunctionCall;
                    std::ostringstream qualified;
                    qualified << expr.ident;
                    for (const auto& field : expr.fields) { qualified << '.' << field; }
                    expr.ident = qualified.str();
                    expr.fields.clear();
                    if (!check(TokenKind::RightParen)) {
                        while (true) {
                            expr.args.push_back(parsePredicateExpr());
                            if (!match(TokenKind::Comma)) { break; }
                        }
                    }
                    expect(TokenKind::RightParen, "expected ')' after qualified function call arguments");
                    return expr;
                }
                expr.kind = ExprKind::FieldAccess;
                return expr;
            }

            if (match(TokenKind::LeftParen)) {
                Expr expr;
                expr.kind = ExprKind::FunctionCall;
                expr.ident = std::move(id);
                if (!check(TokenKind::RightParen)) {
                    while (true) {
                        expr.args.push_back(parsePredicateExpr());
                        if (!match(TokenKind::Comma)) { break; }
                    }
                }
                expect(TokenKind::RightParen, "expected ')' after function call arguments");
                return expr;
            }

            if (match(TokenKind::LeftBracket)) {
                std::vector<Expr> indices;
                indices.push_back(parseValueExpr());
                while (match(TokenKind::Comma)) { indices.push_back(parseValueExpr()); }
                expect(TokenKind::RightBracket, "expected ']' after index expression");
                Expr expr;
                expr.ident = std::move(id);
                if (indices.size() == 1) {
                    expr.kind = ExprKind::ListIndex;
                    expr.left = std::make_unique<Expr>(std::move(indices.front()));
                } else {
                    expr.kind = ExprKind::ArrayIndex;
                    expr.indices = std::move(indices);
                }
                return expr;
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
        if (check(TokenKind::Identifier) && peek().text == "not") {
            static_cast<void>(expectIdentifier("expected 'not'"));
            Expr inner = parsePredicateExpr();
            Expr expr;
            expr.kind = ExprKind::UnaryNot;
            expr.left = std::make_unique<Expr>(std::move(inner));
            return expr;
        }
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
        if (match(TokenKind::Dot)) {
            target.kind = Target::Kind::FieldAccess;
            target.fields.push_back(expectIdentifier("expected field name after '.'").text);
            while (match(TokenKind::Dot)) { target.fields.push_back(expectIdentifier("expected field name after '.'").text); }
            return target;
        }
        if (match(TokenKind::LeftBracket)) {
            std::vector<Expr> indices;
            indices.push_back(parseValueExpr());
            while (match(TokenKind::Comma)) { indices.push_back(parseValueExpr()); }
            expect(TokenKind::RightBracket, "expected ']' after target index");
            if (indices.size() == 1) {
                target.kind = Target::Kind::ListIndex;
                target.index = std::make_unique<Expr>(std::move(indices.front()));
            } else {
                target.kind = Target::Kind::ArrayIndex;
                target.indices = std::move(indices);
            }
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
        throw flow::DiagnosticError{"lowerer", "unsupported operator in flowmini v11 frontend"};
    }

    [[nodiscard]] std::string exprType(const Expr& expr) const {
        if (expr.kind == ExprKind::LiteralString) { return "c_string"; }
        if (expr.kind == ExprKind::LiteralBool) { return "Bool"; }
        if (expr.kind == ExprKind::LiteralInt || expr.kind == ExprKind::StdinInt || expr.kind == ExprKind::ListLength) { return "int"; }
        if (expr.kind == ExprKind::StdinBytes) { return "list<int>"; }
        if (expr.kind == ExprKind::Identifier) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"}; }
            return sym->type;
        }
        if (expr.kind == ExprKind::ListIndex) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared indexed value '" + expr.ident + "'"}; }
            if (!isIntLikeType(exprType(*expr.left))) { throw flow::DiagnosticError{"lowerer", "index for '" + expr.ident + "' must be int-like"}; }
            if (sym->type == "list<int>") { return "int"; }
            if (isArrayIntType(sym->type)) {
                if (sym->shape.size() != 1) { throw flow::DiagnosticError{"lowerer", "array '" + expr.ident + "' has rank " + std::to_string(sym->shape.size()) + " but got 1 index"}; }
                return "int";
            }
            throw flow::DiagnosticError{"lowerer", "indexed access requires list<int> or array<int>, got " + sym->type + " for '" + expr.ident + "'"};
        }
        if (expr.kind == ExprKind::FieldAccess) {
            return fieldAccessType(expr.ident, expr.fields);
        }
        if (expr.kind == ExprKind::ArrayIndex) {
            const Symbol* arraySym = lookup(expr.ident);
            if (arraySym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared array '" + expr.ident + "'"}; }
            if (!isArrayIntType(arraySym->type)) { throw flow::DiagnosticError{"lowerer", "multidimensional access requires array<int>, got " + arraySym->type + " for '" + expr.ident + "'"}; }
            if (expr.indices.size() != arraySym->shape.size()) { throw flow::DiagnosticError{"lowerer", "array '" + expr.ident + "' has rank " + std::to_string(arraySym->shape.size()) + " but got " + std::to_string(expr.indices.size()) + " indexes"}; }
            for (const auto& index : expr.indices) { if (!isIntLikeType(exprType(index))) { throw flow::DiagnosticError{"lowerer", "array indexes for '" + expr.ident + "' must be int-like"}; } }
            return "int";
        }
        if (expr.kind == ExprKind::FunctionCall) {
            const FunctionDef* fn = lookupFunction(expr.ident);
            if (fn == nullptr) { throw flow::DiagnosticError{"lowerer", "call to undeclared function '" + expr.ident + "'"}; }
            if (expr.args.size() != fn->args.size()) { throw flow::DiagnosticError{"lowerer", "function '" + expr.ident + "' expects " + std::to_string(fn->args.size()) + " arguments but got " + std::to_string(expr.args.size())}; }
            for (std::size_t i = 0; i < expr.args.size(); ++i) {
                const std::string actual = exprType(expr.args[i]);
                if (!canAssignType(actual, fn->args[i].type)) { throw flow::DiagnosticError{"lowerer", "argument " + std::to_string(i + 1) + " for function '" + expr.ident + "' expects " + fn->args[i].type + " but got " + actual}; }
            }
            return fn->returnType;
        }
        if (expr.kind == ExprKind::VariantConstruct) {
            const TypeDef* def = lookupType(expr.ident);
            const auto member = def->variantMembers.find(expr.fields.front());
            if (member->second.size() != expr.args.size()) { throw flow::DiagnosticError{"lowerer", "variant member '" + expr.fields.front() + "' payload count mismatch"}; }
            for (std::size_t i = 0; i < expr.args.size(); ++i) {
                if (!canAssignType(exprType(expr.args[i]), member->second[i].type)) { throw flow::DiagnosticError{"lowerer", "variant payload type mismatch for '" + member->second[i].name + "'"}; }
            }
            return expr.ident;
        }
        if (expr.kind == ExprKind::UnaryNot) {
            const std::string inner = exprType(*expr.left);
            if (inner != "Bool") { throw flow::DiagnosticError{"lowerer", "not requires Bool operand, got " + inner}; }
            return "Bool";
        }
        const std::string lhs = exprType(*expr.left);
        const std::string rhs = exprType(*expr.right);
        if (expr.op == TokenKind::EqualEqual && lhs == "Bool" && rhs == "Bool") {
            return "Bool";
        }
        if (expr.op == TokenKind::EqualEqual && lhs == rhs && isEnumType(lhs)) {
            return "Bool";
        }
        if (!isIntLikeType(lhs) || !isIntLikeType(rhs)) { throw flow::DiagnosticError{"lowerer", "binary operator requires int-like operands"}; }
        if (expr.op == TokenKind::Greater || expr.op == TokenKind::Less || expr.op == TokenKind::GreaterEqual || expr.op == TokenKind::LessEqual || expr.op == TokenKind::EqualEqual || expr.op == TokenKind::BangEqual) {
            return "Bool";
        }
        return "int";
    }

    [[nodiscard]] std::string targetType(const Target& target) const {
        const Symbol* sym = lookup(target.ident);
        if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "cannot assign to undeclared target '" + target.ident + "'"}; }
        if (target.kind == Target::Kind::Identifier) { return sym->type; }
        if (target.kind == Target::Kind::FieldAccess) { return fieldAccessType(target.ident, target.fields); }
        if (target.kind == Target::Kind::ListIndex) {
            if (!isIntLikeType(exprType(*target.index))) { throw flow::DiagnosticError{"lowerer", "assignment index for '" + target.ident + "' must be int-like"}; }
            if (sym->type == "list<int>") { return "int"; }
            if (isArrayIntType(sym->type)) {
                if (sym->shape.size() != 1) { throw flow::DiagnosticError{"lowerer", "array '" + target.ident + "' has rank " + std::to_string(sym->shape.size()) + " but got 1 index"}; }
                return "int";
            }
            throw flow::DiagnosticError{"lowerer", "indexed assignment requires list<int> or array<int>, got " + sym->type + " for '" + target.ident + "'"};
        }
        if (!isArrayIntType(sym->type)) { throw flow::DiagnosticError{"lowerer", "array coordinate assignment requires array<int>, got " + sym->type + " for '" + target.ident + "'"}; }
        if (target.indices.size() != sym->shape.size()) { throw flow::DiagnosticError{"lowerer", "array '" + target.ident + "' has rank " + std::to_string(sym->shape.size()) + " but got " + std::to_string(target.indices.size()) + " indexes"}; }
        for (const auto& index : target.indices) { if (!isIntLikeType(exprType(index))) { throw flow::DiagnosticError{"lowerer", "array assignment indexes for '" + target.ident + "' must be int-like"}; } }
        return "int";
    }

    [[nodiscard]] std::string lowerExprToPath(const Expr& expr, const std::string& targetHint, Step* step) {
        if (expr.kind == ExprKind::LiteralBool) {
            const std::string id = generatedId("bool");
            const std::string path = targetHint.empty() ? id : targetHint;
            addNode("node", id, "const.bool"); addPolicy(id, "out", path); addPolicy(id, "value", expr.boolLiteral);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return path;
        }
        if (expr.kind == ExprKind::LiteralString) {
            const std::string id = generatedId("text");
            const std::string path = targetHint.empty() ? id : targetHint;
            addNode("node", id, "const.text"); addPolicy(id, "out", path); addPolicy(id, "value", expr.stringLiteral);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return path;
        }
        if (expr.kind == ExprKind::Identifier) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared identifier '" + expr.ident + "'"}; }
            if (sym->constantInt.has_value()) {
                const std::string id = generatedId("const_ref");
                const std::string path = targetHint.empty() ? id : targetHint;
                addNode("node", id, "const.int");
                addPolicy(id, "out", path);
                addPolicy(id, "value", *sym->constantInt);
                if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
                return path;
            }
            if (sym->constantBool.has_value()) {
                const std::string id = generatedId("const_ref");
                const std::string path = targetHint.empty() ? id : targetHint;
                addNode("node", id, "const.bool");
                addPolicy(id, "out", path);
                addPolicy(id, "value", *sym->constantBool);
                if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
                return path;
            }
            return sym->path;
        }
        if (expr.kind == ExprKind::FieldAccess) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared field value '" + expr.ident + "'"}; }
            const std::string id = generatedId("field_get");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "record.field.get");
            addPolicy(id, "record", sym->path);
            addPolicy(id, "fields", joinFields(expr.fields));
            addPolicy(id, "out", out);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return out;
        }
        if (expr.kind == ExprKind::ListLength) {
            const Symbol* listSym = lookup(expr.ident);
            if (listSym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared list '" + expr.ident + "'"}; }
            if (listSym->type.rfind("list<", 0) != 0) { throw flow::DiagnosticError{"lowerer", "length(...) requires a list, got " + listSym->type + " for '" + expr.ident + "'"}; }
            const std::string id = generatedId("len");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "list.length");
            addPolicy(id, "list", listSym->path); addPolicy(id, "out", out);
            if (step != nullptr) { appendStep(*step, Step{false, false, {id, "in"}, {id, "out"}}); }
            return out;
        }
        if (expr.kind == ExprKind::ListIndex) {
            const Symbol* sym = lookup(expr.ident);
            if (sym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared indexed value '" + expr.ident + "'"}; }
            const std::string containerType = sym->type;
            const std::string containerPath = sym->path;
            Step indexStep;
            const std::string indexPath = lowerExprToPath(*expr.left, generatedId("idx"), &indexStep);
            const std::string id = generatedId(containerType == "list<int>" ? "list_get" : "array_get");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, containerType == "list<int>" ? "list.get" : "array.get");
            if (containerType == "list<int>") { addPolicy(id, "list", containerPath); addPolicy(id, "index", indexPath); }
            else { addPolicy(id, "array", containerPath); addPolicy(id, "indices", indexPath); }
            addPolicy(id, "out", out);
            Step combined;
            appendStep(combined, indexStep);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            if (step != nullptr) { appendStep(*step, combined); }
            return out;
        }
        if (expr.kind == ExprKind::FieldAccess) {
            return fieldAccessType(expr.ident, expr.fields);
        }
        if (expr.kind == ExprKind::ArrayIndex) {
            const Symbol* arraySym = lookup(expr.ident);
            if (arraySym == nullptr) { throw flow::DiagnosticError{"lowerer", "use of undeclared array '" + expr.ident + "'"}; }
            const std::string arrayType = arraySym->type;
            const std::string arrayPath = arraySym->path;
            if (!isArrayIntType(arrayType)) { throw flow::DiagnosticError{"lowerer", "multidimensional access requires array<int>, got " + arrayType + " for '" + expr.ident + "'"}; }
            Step combined;
            std::ostringstream paths;
            for (std::size_t i = 0; i < expr.indices.size(); ++i) {
                Step indexStep;
                const std::string indexPath = lowerExprToPath(expr.indices[i], generatedId("arr_idx"), &indexStep);
                appendStep(combined, indexStep);
                if (i > 0) { paths << ','; }
                paths << indexPath;
            }
            const std::string id = generatedId("array_get");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "array.get");
            addPolicy(id, "array", arrayPath); addPolicy(id, "indices", paths.str()); addPolicy(id, "out", out);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            if (step != nullptr) { appendStep(*step, combined); }
            return out;
        }
        if (expr.kind == ExprKind::FunctionCall) {
            Step callStep = lowerFunctionCall(expr, targetHint.empty() ? generatedId("fn_return") : targetHint);
            if (step != nullptr) { appendStep(*step, callStep); }
            return targetHint.empty() ? callStepResultPath_ : targetHint;
        }
        if (expr.kind == ExprKind::VariantConstruct) {
            const TypeDef* def = lookupType(expr.ident);
            const auto member = def->variantMembers.find(expr.fields.front());
            int tag = 0;
            for (const auto& [name, fields] : def->variantMembers) {
                if (name == expr.fields.front()) { break; }
                ++tag;
            }
            const std::string out = targetHint.empty() ? generatedId("variant") : targetHint;
            Step combined;
            std::ostringstream fields;
            std::ostringstream paths;
            for (std::size_t i = 0; i < expr.args.size(); ++i) {
                Step payloadStep;
                const std::string payloadPath = lowerExprToPath(expr.args[i], generatedId("variant_payload"), &payloadStep);
                appendStep(combined, payloadStep);
                if (i > 0) { fields << ','; paths << ','; }
                fields << member->second[i].name;
                paths << payloadPath;
            }
            const std::string nodeId = generatedId("variant_construct");
            addNode("node", nodeId, "variant.construct");
            addPolicy(nodeId, "out", out);
            addPolicy(nodeId, "tag", tag);
            addPolicy(nodeId, "fields", fields.str());
            addPolicy(nodeId, "field_paths", paths.str());
            appendStep(combined, Step{false, false, {nodeId, "in"}, {nodeId, "out"}});
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
        if (expr.kind == ExprKind::StdinBytes) {
            const std::string stdinId = generatedId("stdin_bytes");
            const std::string path = targetHint.empty() ? generatedId("tmp_bytes") : targetHint;
            addNode("producer", stdinId, "stdin.bytes");
            addPolicy(stdinId, "out", path);
            if (step != nullptr) { appendStep(*step, Step{false, false, {stdinId, "out"}, {stdinId, "out"}}); }
            return path;
        }

        if (expr.kind == ExprKind::UnaryNot) {
            Step innerStep;
            const std::string in = lowerExprToPath(*expr.left, generatedId("not_in"), &innerStep);
            const std::string id = generatedId("not");
            const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
            addNode("node", id, "bool.not");
            addPolicy(id, "in_path", in); addPolicy(id, "out", out);
            Step combined;
            appendStep(combined, innerStep);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            if (step != nullptr) { appendStep(*step, combined); }
            return out;
        }

        Step leftStep;
        Step rightStep;
        const std::string lhsType = exprType(*expr.left);
        const std::string rhsType = exprType(*expr.right);
        const std::string lhs = lowerExprToPath(*expr.left, generatedId("tmp"), &leftStep);
        const std::string rhs = lowerExprToPath(*expr.right, generatedId("tmp"), &rightStep);
        const std::string id = generatedId("op");
        const std::string out = targetHint.empty() ? generatedId("tmp") : targetHint;
        std::string kind = opToKind(expr.op);
        if (expr.op == TokenKind::EqualEqual && lhsType == "Bool" && rhsType == "Bool") { kind = "bool.eq"; }
        addNode("node", id, kind);
        addPolicy(id, "lhs", lhs); addPolicy(id, "rhs", rhs); addPolicy(id, "out", out);
        Step opStep{false, false, {id, "in"}, {id, "out"}};
        Step combined;
        appendStep(combined, leftStep);
        appendStep(combined, rightStep);
        appendStep(combined, opStep);
        if (step != nullptr) { appendStep(*step, combined); }
        return out;
    }


    [[nodiscard]] Step lowerExternFunctionCall(const Expr& expr, const std::string& requestedOutPath, const FunctionDef& fn) {
        if (expr.args.size() != fn.args.size()) {
            throw flow::DiagnosticError{"lowerer", "extern function '" + expr.ident + "' expects " + std::to_string(fn.args.size()) + " arguments but got " + std::to_string(expr.args.size())};
        }
        Step combined;
        std::ostringstream argPaths;
        std::ostringstream argTypes;
        for (std::size_t i = 0; i < expr.args.size(); ++i) {
            const std::string actual = exprType(expr.args[i]);
            if (!canAssignType(actual, fn.args[i].type)) {
                throw flow::DiagnosticError{"lowerer", "argument " + std::to_string(i + 1) + " for extern function '" + expr.ident + "' expects " + fn.args[i].type + " but got " + actual};
            }
            Step argStep;
            const std::string argPath = lowerExprToPath(expr.args[i], generatedId("abi_arg"), &argStep);
            appendStep(combined, argStep);
            if (i > 0) { argPaths << ','; argTypes << ','; }
            argPaths << argPath;
            argTypes << fn.args[i].type;
        }
        const std::string id = generatedId("abi_call_" + fn.name);
        addNode("node", id, "abi.call");
        addPolicy(id, "library", fn.library);
        addPolicy(id, "symbol", fn.symbol);
        addPolicy(id, "convention", fn.convention);
        addPolicy(id, "effect", fn.effect);
        addPolicy(id, "arg_paths", argPaths.str());
        addPolicy(id, "arg_types", argTypes.str());
        addPolicy(id, "return_type", fn.returnType);
        addPolicy(id, "out", requestedOutPath);
        appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
        callStepResultPath_ = requestedOutPath;
        return combined;
    }

    [[nodiscard]] Step lowerFunctionCall(const Expr& expr, const std::string& requestedOutPath) {
        const FunctionDef* fn = lookupFunction(expr.ident);
        if (fn == nullptr) { throw flow::DiagnosticError{"lowerer", "call to undeclared function '" + expr.ident + "'"}; }
        if (fn->isExtern) { return lowerExternFunctionCall(expr, requestedOutPath, *fn); }
        if (std::find(functionCallStack_.begin(), functionCallStack_.end(), expr.ident) != functionCallStack_.end()) {
            std::ostringstream cycle;
            for (const auto& name : functionCallStack_) { cycle << name << " -> "; }
            cycle << expr.ident;
            throw flow::DiagnosticError{"lowerer", "recursive function cycle detected: " + cycle.str() + "; recursion is not supported in flowmini v12"};
        }
        if (expr.args.size() != fn->args.size()) { throw flow::DiagnosticError{"lowerer", "function '" + expr.ident + "' expects " + std::to_string(fn->args.size()) + " arguments but got " + std::to_string(expr.args.size())}; }

        Step combined;
        std::vector<std::string> callerArgPaths;
        callerArgPaths.reserve(expr.args.size());
        for (std::size_t i = 0; i < expr.args.size(); ++i) {
            if (!canAssignType(exprType(expr.args[i]), fn->args[i].type)) { throw flow::DiagnosticError{"lowerer", "argument type mismatch for function '" + expr.ident + "'"}; }
            Step argStep;
            const std::string argPath = lowerExprToPath(expr.args[i], generatedId("call_arg"), &argStep);
            appendStep(combined, argStep);
            callerArgPaths.push_back(argPath);
        }

        const auto savedScopes = scopes_;
        const std::size_t savedPos = pos_;
        const bool savedReturnSeen = currentFunctionSawReturn_;
        const std::string savedReturnType = activeReturnType_;
        currentFunctionSawReturn_ = false;
        activeReturnType_ = fn->returnType;

        scopes_.clear();
        scopes_.push_back(Scope{module_.name, module_.name, {}});
        for (const auto& [typeName, typeDef] : types_) {
            if (!typeDef.isEnum) { continue; }
            for (const auto& [memberName, memberValue] : typeDef.enumMembers) {
                scopes_.front().symbols[memberName] = Symbol{typeName,
                    "__" + sanitize(module_.name) + "_" + memberName,
                    module_.name + "::" + memberName,
                    {}, true, memberValue, {}};
            }
        }
        enterScope("call_" + fn->name + "_" + std::to_string(++functionInstanceCounter_));

        for (std::size_t i = 0; i < fn->args.size(); ++i) {
            const auto& arg = fn->args[i];
            currentScope().symbols[arg.name] = Symbol{arg.type, pathFor(arg.name), currentScope().qualifiedName + "::" + arg.name, {}, false, {}, {}};
        }
        currentScope().symbols["return"] = Symbol{fn->returnType, pathFor("return"), currentScope().qualifiedName + "::return", {}, false, {}, {}};

        Step bindStep;
        for (std::size_t i = 0; i < fn->args.size(); ++i) {
            const std::string id = generatedId("bind_" + fn->args[i].name);
            addNode("node", id, "record.copy");
            addPolicy(id, "from", callerArgPaths[i]);
            addPolicy(id, "to", lookup(fn->args[i].name)->path);
            appendStep(bindStep, Step{false, false, {id, "in"}, {id, "out"}});
        }
        appendStep(combined, bindStep);

        functionCallStack_.push_back(fn->name);
        pos_ = fn->bodyStart;
        Step body = parseBlockStatementsUntilRightBrace();
        functionCallStack_.pop_back();
        if (pos_ != fn->bodyEnd + 1) { throw flow::DiagnosticError{"lowerer", "internal parser error while lowering function '" + fn->name + "'"}; }
        if (body.empty) { throw flow::DiagnosticError{"lowerer", "function '" + fn->name + "' body may not be empty"}; }
        if (!currentFunctionSawReturn_) { throw flow::DiagnosticError{"lowerer", "function '" + fn->name + "' does not assign to return"}; }
        appendStep(combined, body);

        const std::string copyReturnId = generatedId("copy_return");
        const std::string returnPath = lookup("return")->path;
        addNode("node", copyReturnId, "record.copy");
        addPolicy(copyReturnId, "from", returnPath);
        addPolicy(copyReturnId, "to", requestedOutPath);
        appendStep(combined, Step{false, false, {copyReturnId, "in"}, {copyReturnId, "out"}});

        leaveScope();
        scopes_ = savedScopes;
        pos_ = savedPos;
        currentFunctionSawReturn_ = savedReturnSeen;
        activeReturnType_ = savedReturnType;
        callStepResultPath_ = requestedOutPath;
        return combined;
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

    // -------- Type declarations --------
    void parseTypeDecl() {
        const Token& typeToken = expectIdentifier("expected 'type'");
        if (typeToken.text != "type") { fail(typeToken, "expected 'type'"); }
        const Token& nameToken = expectIdentifier("expected type name");
        TypeDef def;
        def.name = nameToken.text;
        if (types_.count(def.name) != 0) { fail(nameToken, "duplicate type declaration '" + def.name + "'"); }

        if (check(TokenKind::Identifier) && peek().text == "refines") {
            static_cast<void>(expectIdentifier("expected 'refines' after type name"));
            def.base = expectIdentifier("expected base type after refines").text;
            if (!typeExists(def.base)) { fail(nameToken, "type '" + def.name + "' refines unknown base type '" + def.base + "'"); }
        }

        expect(TokenKind::LeftBrace, "expected '{' before type body");
        skipNewlines();
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& head = expectIdentifier("expected 'invariant' or 'field' in type body");
            if (head.text == "invariant") {
                if (!def.fields.empty()) { fail(head, "flowmini v18 does not mix scalar invariants and record fields in one type block yet"); }
                if (def.base.empty()) { fail(head, "invariant types must refine an existing base type"); }
                const Token& valueToken = expectIdentifier("expected 'value' after invariant");
                if (valueToken.text != "value") { fail(valueToken, "type invariant currently supports only 'value <op> literal'"); }
                if (!(check(TokenKind::Greater) || check(TokenKind::Less) || check(TokenKind::GreaterEqual) || check(TokenKind::LessEqual) || check(TokenKind::EqualEqual) || check(TokenKind::BangEqual))) {
                    fail(peek(), "expected comparison operator in type invariant");
                }
                const TokenKind op = peek().kind;
                ++pos_;
                int literal = 0;
                if (match(TokenKind::Minus)) { literal = -parseIntToken(expect(TokenKind::Number, "expected integer literal in type invariant")); }
                else { literal = parseIntToken(expect(TokenKind::Number, "expected integer literal in type invariant")); }
                def.invariants.push_back(TypeInvariant{op, literal});
                expectLineEnd("expected newline after type invariant");
                skipNewlines();
                continue;
            }
            if (head.text == "field") {
                if (!def.base.empty() || !def.invariants.empty()) { fail(head, "flowmini v18 record types may not refine scalar contracts yet"); }
                const Token& fieldToken = expectIdentifier("expected field name");
                expect(TokenKind::Colon, "expected ':' after field name");
                const std::string fieldType = expectIdentifier("expected field type").text;
                if (!typeExists(fieldType) || isAbiPointerType(fieldType)) {
                    fail(fieldToken, "record field '" + fieldToken.text + "' uses unknown or sealed field type '" + fieldType + "'");
                }
                for (const auto& existing : def.fields) {
                    if (existing.name == fieldToken.text) { fail(fieldToken, "duplicate field '" + fieldToken.text + "' in type '" + def.name + "'"); }
                }
                def.fields.push_back(TypeField{fieldToken.text, fieldType});
                expectLineEnd("expected newline after field declaration");
                skipNewlines();
                continue;
            }
            fail(head, "flowmini v18 type blocks support invariant or field declarations");
        }
        expect(TokenKind::RightBrace, "expected '}' after type body");
        if (def.base.empty() && def.fields.empty()) { fail(nameToken, "type '" + def.name + "' must refine a base type or declare fields"); }
        types_[def.name] = std::move(def);
    }

    void parseEnumDecl() {
        static_cast<void>(expectIdentifier("expected 'enum'"));
        const Token& nameToken = expectIdentifier("expected enum name");
        if (types_.contains(nameToken.text)) { fail(nameToken, "duplicate type declaration '" + nameToken.text + "'"); }
        TypeDef def;
        def.name = nameToken.text;
        def.isEnum = true;
        expect(TokenKind::LeftBrace, "expected '{' before enum members");
        skipNewlines();
        int ordinal = 0;
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& member = expectIdentifier("expected enum member");
            if (def.enumMembers.contains(member.text)) { fail(member, "duplicate enum member '" + member.text + "'"); }
            def.enumMembers.emplace(member.text, ordinal++);
            currentScope().symbols[member.text] = Symbol{nameToken.text, pathFor(member.text), currentScope().qualifiedName + "::" + member.text, {}, true, ordinal - 1, {}};
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' after enum members");
        if (def.enumMembers.empty()) { fail(nameToken, "enum must declare at least one member"); }
        types_[def.name] = std::move(def);
    }

    void parseVariantDecl() {
        static_cast<void>(expectIdentifier("expected 'variant'"));
        const Token& nameToken = expectIdentifier("expected variant name");
        if (types_.contains(nameToken.text)) { fail(nameToken, "duplicate type declaration '" + nameToken.text + "'"); }
        TypeDef def;
        def.name = nameToken.text;
        def.isVariant = true;
        expect(TokenKind::LeftBrace, "expected '{' before variant members");
        skipNewlines();
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& memberToken = expectIdentifier("expected variant member");
            if (def.variantMembers.contains(memberToken.text)) { fail(memberToken, "duplicate variant member '" + memberToken.text + "'"); }
            std::vector<TypeField> fields;
            if (match(TokenKind::LeftParen)) {
                if (!check(TokenKind::RightParen)) {
                    while (true) {
                        const Token& fieldToken = expectIdentifier("expected variant payload field");
                        expect(TokenKind::Colon, "expected ':' after variant payload field");
                        const std::string fieldType = expectIdentifier("expected variant payload field type").text;
                        if (!typeExists(fieldType) || isAbiPointerType(fieldType)) { fail(fieldToken, "variant payload field uses unknown or sealed type"); }
                        fields.push_back(TypeField{fieldToken.text, fieldType});
                        if (!match(TokenKind::Comma)) { break; }
                    }
                }
                expect(TokenKind::RightParen, "expected ')' after variant payload");
            }
            def.variantMembers.emplace(memberToken.text, std::move(fields));
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' after variant members");
        if (def.variantMembers.empty()) { fail(nameToken, "variant must declare at least one member"); }
        types_[def.name] = std::move(def);
    }


    // -------- ABI declarations --------
    void parseAbiBlock() {
        const Token& abiToken = expectIdentifier("expected 'abi'");
        if (abiToken.text != "abi") { fail(abiToken, "expected 'abi'"); }
        const std::string abiName = expectIdentifier("expected ABI block name").text;
        expect(TokenKind::LeftBrace, "expected '{' before abi block");
        skipNewlines();

        std::string library;
        std::string convention = "c";

        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::RightBrace)) { break; }
            const Token& head = expectIdentifier("expected abi property or extern declaration");
            if (head.text == "library") {
                library = expect(TokenKind::String, "expected quoted library path/name").text;
                expectLineEnd("expected newline after library declaration");
                continue;
            }
            if (head.text == "convention") {
                convention = expectIdentifier("expected calling convention").text;
                if (convention != "c") { fail(head, "flowmini v16 supports only C ABI convention"); }
                expectLineEnd("expected newline after convention declaration");
                continue;
            }
            if (head.text == "type") {
                parseAbiTypeDecl(abiName);
                expectLineEnd("expected newline after abi type declaration");
                continue;
            }
            if (head.text == "struct") {
                parseAbiStructDecl(abiName);
                expectLineEnd("expected newline after abi struct declaration");
                continue;
            }
            if (head.text == "extern") {
                parseExternFunctionDecl(abiName, library, convention);
                expectLineEnd("expected newline after extern function declaration");
                continue;
            }
            fail(head, "expected library, convention, type, struct, or extern in abi block");
        }
        expect(TokenKind::RightBrace, "expected '}' after abi block");
    }

    void parseAbiTypeDecl(const std::string&) {
        const Token& nameToken = expectIdentifier("expected ABI type name");
        AbiTypeDef def;
        def.name = nameToken.text;
        if (def.name == "c_int" || def.name == "c_long" || def.name == "c_ulong" || def.name == "c_size_t") {
            fail(nameToken, "ABI scalar type '" + def.name + "' is built in and may not be redeclared");
        }
        expect(TokenKind::LeftBrace, "expected '{' before abi type body");
        skipNewlines();
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& prop = expectIdentifier("expected abi type property");
            if (prop.text == "repr") {
                def.repr = expect(TokenKind::String, "expected quoted ABI representation").text;
            } else if (prop.text == "ownership") {
                def.ownership = expectIdentifier("expected ownership policy").text;
                if (!(def.ownership == "borrowed" || def.ownership == "owned" || def.ownership == "external" || def.ownership == "consumed")) {
                    fail(prop, "ABI pointer ownership must be borrowed, owned, external, or consumed");
                }
            } else if (prop.text == "access") {
                def.access = expectIdentifier("expected access policy").text;
                if (!(def.access == "read" || def.access == "write" || def.access == "readwrite" || def.access == "opaque")) {
                    fail(prop, "ABI pointer access must be read, write, readwrite, or opaque");
                }
            } else if (prop.text == "lifetime") {
                def.lifetime = expectIdentifier("expected lifetime policy").text;
                if (!(def.lifetime == "call" || def.lifetime == "owned" || def.lifetime == "external")) {
                    fail(prop, "ABI pointer lifetime must be call, owned, or external");
                }
            } else if (prop.text == "nullable") {
                if (!(check(TokenKind::KeywordTrue) || check(TokenKind::KeywordFalse) || check(TokenKind::Identifier))) { fail(peek(), "expected true or false for nullable"); }
                const Token val = peek();
                ++pos_;
                if (!(val.text == "true" || val.text == "false")) { fail(val, "nullable must be true or false"); }
                def.nullable = val.text;
            } else if (prop.text == "terminator") {
                def.terminator = expectIdentifier("expected terminator policy").text;
            } else if (prop.text == "opaque") {
                if (!(check(TokenKind::KeywordTrue) || check(TokenKind::KeywordFalse) || check(TokenKind::Identifier))) { fail(peek(), "expected true or false for opaque"); }
                const Token val = peek();
                ++pos_;
                if (!(val.text == "true" || val.text == "false")) { fail(val, "opaque must be true or false"); }
                def.opaque = (val.text == "true");
            } else if (prop.text == "cleanup") {
                def.cleanup = expectIdentifier("expected cleanup capability name").text;
            } else {
                fail(prop, "ABI type blocks support repr, ownership, access, lifetime, nullable, terminator, opaque, and cleanup");
            }
            expectLineEnd("expected newline after abi type property");
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' after abi type body");
        if (def.repr.empty()) { fail(nameToken, "ABI type '" + def.name + "' requires repr"); }
        if (def.ownership.empty()) { fail(nameToken, "ABI type '" + def.name + "' requires ownership"); }
        if (def.access.empty()) { fail(nameToken, "ABI type '" + def.name + "' requires access"); }
        if (def.lifetime.empty()) { fail(nameToken, "ABI type '" + def.name + "' requires lifetime"); }
        if (def.nullable.empty()) { def.nullable = "false"; }
        abiTypes_[def.name] = std::move(def);
        if (!typeExists(nameToken.text)) { types_.emplace(nameToken.text, TypeDef{nameToken.text, "", {}, {}, false, {}, false, {}}); }
    }

    void parseAbiStructDecl(const std::string&) {
        const Token& nameToken = expectIdentifier("expected ABI struct name");
        AbiStructDef def;
        def.name = nameToken.text;
        if (typeExists(def.name)) { fail(nameToken, "ABI struct type '" + def.name + "' conflicts with existing type"); }
        expect(TokenKind::LeftBrace, "expected '{' before abi struct body");
        skipNewlines();
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& fieldToken = expectIdentifier("expected ABI struct field name");
            expect(TokenKind::Colon, "expected ':' after ABI struct field name");
            const std::string fieldType = expectIdentifier("expected ABI struct field type").text;
            if (!(fieldType == "c_int" || fieldType == "c_long" || fieldType == "c_size_t")) {
                fail(fieldToken, "flowmini v17 ABI struct fields currently support c_int, c_long, and c_size_t only");
            }
            for (const auto& existing : def.fields) {
                if (existing.name == fieldToken.text) { fail(fieldToken, "duplicate ABI struct field '" + fieldToken.text + "'"); }
            }
            def.fields.push_back(AbiStructFieldDef{fieldToken.text, fieldType});
            expectLineEnd("expected newline after ABI struct field");
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' after abi struct body");
        if (def.fields.empty()) { fail(nameToken, "ABI struct '" + def.name + "' must have at least one field"); }
        abiStructs_[def.name] = def;
        types_.emplace(def.name, TypeDef{def.name, "", {}, {}, false, {}, false, {}});
    }

    void parseExternFunctionDecl(const std::string& abiName, const std::string& library, const std::string& convention) {
        if (library.empty()) { throw flow::DiagnosticError{"parser", "abi block '" + abiName + "' must declare library before extern functions"}; }
        expect(TokenKind::KeywordFn, "expected 'fn' after extern");
        const Token& nameToken = expectIdentifier("expected extern function name");
        FunctionDef def;
        const std::string shortName = nameToken.text;
        def.name = abiName + "." + shortName;
        if (functions_.count(def.name) != 0) { fail(nameToken, "duplicate function declaration '" + def.name + "'"); }
        def.isExtern = true;
        def.library = library;
        def.convention = convention;
        expect(TokenKind::LeftParen, "expected '(' after extern function name");
        if (!check(TokenKind::RightParen)) {
            while (true) {
                const Token& argToken = expectIdentifier("expected extern argument name");
                expect(TokenKind::Colon, "expected ':' after argument name");
                const std::string argType = expectIdentifier("expected ABI argument type").text;
                if (!isCAbiType(argType)) { fail(argToken, "flowmini v17 extern arguments require c_int, c_long, c_ulong, c_size_t, a declared ABI pointer-shaped type, or a declared ABI struct"); }
                for (const auto& existing : def.args) { if (existing.name == argToken.text) { fail(argToken, "duplicate extern argument '" + argToken.text + "'"); } }
                def.args.push_back(FunctionArg{argToken.text, argType});
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightParen, "expected ')' after extern arguments");
        expect(TokenKind::Colon, "expected ':' before extern return type");
        def.returnType = expectIdentifier("expected extern return type").text;
        if (!(def.returnType == "c_int" || def.returnType == "c_long" || def.returnType == "c_ulong" || def.returnType == "c_size_t")) { fail(nameToken, "flowmini v17 extern return type must be c_int, c_long, c_ulong, or c_size_t"); }
        expect(TokenKind::LeftBrace, "expected '{' before extern body");
        skipNewlines();
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            const Token& prop = expectIdentifier("expected symbol or effect in extern body");
            if (prop.text == "symbol") {
                def.symbol = expect(TokenKind::String, "expected quoted symbol name").text;
            } else if (prop.text == "effect") {
                def.effect = expectIdentifier("expected effect name").text;
            } else {
                fail(prop, "flowmini v17 extern body supports only symbol and effect");
            }
            expectLineEnd("expected newline after extern property");
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' after extern body");
        if (def.symbol.empty()) { fail(nameToken, "extern function requires symbol declaration"); }
        if (def.effect.empty()) { fail(nameToken, "extern function requires effect declaration"); }
        const FunctionDef compatibilityCopy = def;
        functions_[def.name] = std::move(def);
        // Transitional compatibility: retain an unqualified alias only while
        // exactly one provider exports the short name. Ambiguity is permanent;
        // a third or later provider must never recreate the alias.
        if (ambiguousFunctionNames_.contains(shortName)) {
            functions_.erase(shortName);
        } else if (functions_.contains(shortName)) {
            functions_.erase(shortName);
            ambiguousFunctionNames_.insert(shortName);
        } else {
            functions_[shortName] = compatibilityCopy;
        }
    }

    // -------- Function declarations --------
    void parseFunctionDecl() {
        const Token& nameToken = expectIdentifier("expected function name after 'fn'");
        FunctionDef def;
        def.name = nameToken.text;
        if (functions_.count(def.name) != 0) { fail(nameToken, "duplicate function declaration '" + def.name + "'"); }
        expect(TokenKind::LeftParen, "expected '(' after function name");
        if (!check(TokenKind::RightParen)) {
            while (true) {
                const Token& argToken = expectIdentifier("expected argument name");
                expect(TokenKind::Colon, "expected ':' after argument name");
                const std::string argType = expectIdentifier("expected argument type").text;
                if (!isCallableType(argType)) { fail(argToken, "flowmini v18 function arguments require a callable value type"); }
                for (const auto& existing : def.args) { if (existing.name == argToken.text) { fail(argToken, "duplicate function argument '" + argToken.text + "'"); } }
                def.args.push_back(FunctionArg{argToken.text, argType});
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightParen, "expected ')' after function arguments");
        expect(TokenKind::Colon, "expected ':' before function return type");
        def.returnType = expectIdentifier("expected function return type").text;
        if (!isCallableType(def.returnType)) { fail(nameToken, "flowmini v18 functions require a callable value return type"); }
        expect(TokenKind::LeftBrace, "expected '{' before function body");
        def.bodyStart = pos_;

        int depth = 1;
        while (depth > 0 && !check(TokenKind::End)) {
            if (check(TokenKind::LeftBrace)) { ++depth; }
            else if (check(TokenKind::RightBrace)) { --depth; }
            ++pos_;
        }
        if (depth != 0) { fail(peek(), "unterminated function body"); }
        def.bodyEnd = pos_ - 1;
        functions_[def.name] = std::move(def);
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
        if (check(TokenKind::Identifier) && isEnumType(peek().text)) {
            const std::string enumType = expectIdentifier("expected enum type").text;
            expect(TokenKind::LeftParen, "enum declaration requires initializer");
            Expr initializer = parsePredicateExpr();
            expect(TokenKind::RightParen, "expected ')' after enum initializer");
            if (exprType(initializer) != enumType) { throw flow::DiagnosticError{"lowerer", "enum initializer for '" + id + "' has wrong enum type"}; }
            declareSymbol(idToken, id, enumType);
            Step step;
            static_cast<void>(lowerExprToPath(initializer, lookup(id)->path, &step));
            return step;
        }
        if (check(TokenKind::Identifier) && isVariantType(peek().text)) {
            const std::string variantType = expectIdentifier("expected variant type").text;
            expect(TokenKind::LeftParen, "variant declaration requires initializer");
            Expr initializer = parsePredicateExpr();
            expect(TokenKind::RightParen, "expected ')' after variant initializer");
            if (exprType(initializer) != variantType) { throw flow::DiagnosticError{"lowerer", "variant initializer for '" + id + "' has wrong variant type"}; }
            declareSymbol(idToken, id, variantType);
            Step step;
            static_cast<void>(lowerExprToPath(initializer, lookup(id)->path, &step));
            return step;
        }
        if (check(TokenKind::Identifier) && peek().text == "array" && lookahead(1).kind != TokenKind::Dot) {
            return parseArrayDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && peek().text == "Bool" && lookahead(1).kind != TokenKind::Dot) {
            return parseBoolDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && isIntLikeType(peek().text) && lookahead(1).kind != TokenKind::Dot) {
            return parseTypedIntegerDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && peek().text == "c_string" && lookahead(1).kind != TokenKind::Dot) {
            return parseCStringDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && isAbiStructType(peek().text) && lookahead(1).kind != TokenKind::Dot) {
            return parseAbiStructValueDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && isRecordType(peek().text) && lookahead(1).kind != TokenKind::Dot) {
            return parseRecordValueDeclaration(idToken, id);
        }
        if (check(TokenKind::Identifier) && isAbiPointerType(peek().text) && lookahead(1).kind != TokenKind::Dot) {
            fail(peek(), "ABI pointer-shaped type '" + peek().text + "' is sealed; normal Flowmini code may not declare or construct it directly");
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

    [[nodiscard]] Step parseConstDeclarationAsStep() {
        expectIdentifier("expected 'const'");
        const bool previous = declaringConstant_;
        declaringConstant_ = true;
        Step result = parseSweetDeclarationAsStep();
        declaringConstant_ = previous;
        return result;
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
        if (declaringConstant_ && initializer.kind == ExprKind::LiteralInt) {
            currentScope().symbols[id].constantInt = initializer.literal;
            return Step{};
        }
        const std::string path = lookup(id)->path;
        Step step;
        static_cast<void>(lowerExprToPath(initializer, path, &step));
        return step;
    }

    [[nodiscard]] Step parseBoolDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'Bool'");
        if (!match(TokenKind::LeftParen)) { fail(peek(), "declaration of '" + id + "' requires initializer"); }
        Expr initializer = parsePredicateExpr();
        expect(TokenKind::RightParen, "expected ')' after Bool initializer");
        if (exprType(initializer) != "Bool") { throw flow::DiagnosticError{"lowerer", "initializer for Bool '" + id + "' is not Bool"}; }
        declareSymbol(idToken, id, "Bool");
        if (declaringConstant_ && initializer.kind == ExprKind::LiteralBool) {
            currentScope().symbols[id].constantBool = initializer.boolLiteral;
            return Step{};
        }
        const std::string path = lookup(id)->path;
        Step step;
        static_cast<void>(lowerExprToPath(initializer, path, &step));
        return step;
}

    [[nodiscard]] Step parseTypedIntegerDeclaration(const Token& idToken, const std::string& id) {
        const std::string declaredType = expectIdentifier("expected integer-like type").text;
        if (!isIntLikeType(declaredType)) { fail(idToken, "unknown integer-like type '" + declaredType + "'"); }
        if (!match(TokenKind::LeftParen)) { fail(peek(), "declaration of '" + id + "' requires initializer"); }
        Expr initializer = parseValueExpr();
        expect(TokenKind::RightParen, "expected ')' after typed integer initializer");
        const std::string actual = exprType(initializer);
        if (initializer.kind == ExprKind::LiteralInt) {
            if (!literalSatisfiesType(initializer.literal, declaredType)) {
                throw flow::DiagnosticError{"lowerer", "literal " + std::to_string(initializer.literal) + " does not satisfy type contract " + declaredType + " for '" + id + "'"};
            }
        } else if (!canAssignType(actual, declaredType)) {
            throw flow::DiagnosticError{"lowerer", "initializer for '" + id + "' has type " + actual + " but target type is " + declaredType};
        }
        if (initializer.kind != ExprKind::LiteralInt && typeHasAnyInvariants(declaredType) && !isSubtypeOf(actual, declaredType)) {
            throw flow::DiagnosticError{"lowerer", "initializer for refined type '" + declaredType + "' must be a satisfying literal or a value already known to satisfy that type"};
        }
        declareSymbol(idToken, id, declaredType);
        if (declaringConstant_ && initializer.kind == ExprKind::LiteralInt) {
            currentScope().symbols[id].constantInt = initializer.literal;
            return Step{};
        }
        const std::string path = lookup(id)->path;
        Step step;
        static_cast<void>(lowerExprToPath(initializer, path, &step));
        return step;
    }


    [[nodiscard]] Step parseCStringDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'c_string'");
        if (!match(TokenKind::LeftParen)) { fail(peek(), "declaration of '" + id + "' requires initializer"); }
        Expr initializer = parseValueExpr();
        expect(TokenKind::RightParen, "expected ')' after c_string initializer");
        const std::string actual = exprType(initializer);
        if (!canAssignType(actual, "c_string")) {
            throw flow::DiagnosticError{"lowerer", "initializer for c_string '" + id + "' is not c_string"};
        }
        declareSymbol(idToken, id, "c_string");
        const std::string path = lookup(id)->path;
        Step step;
        static_cast<void>(lowerExprToPath(initializer, path, &step));
        return step;
    }

    [[nodiscard]] Step parseAbiStructValueDeclaration(const Token& idToken, const std::string& id) {
        const std::string structType = expectIdentifier("expected ABI struct type").text;
        const auto defIt = abiStructs_.find(structType);
        if (defIt == abiStructs_.end()) { fail(idToken, "unknown ABI struct type '" + structType + "'"); }
        const AbiStructDef& def = defIt->second;
        expect(TokenKind::LeftParen, "ABI struct declaration requires initializer");
        expect(TokenKind::LeftBrace, "ABI struct initializer must start with '{'");

        std::map<std::string, Expr> fieldExprs;
        if (!check(TokenKind::RightBrace)) {
            while (true) {
                const Token& fieldToken = expectIdentifier("expected ABI struct initializer field name");
                expect(TokenKind::Colon, "expected ':' after ABI struct initializer field name");
                bool known = false;
                for (const auto& f : def.fields) { if (f.name == fieldToken.text) { known = true; break; } }
                if (!known) { fail(fieldToken, "ABI struct '" + structType + "' has no field '" + fieldToken.text + "'"); }
                if (fieldExprs.count(fieldToken.text) != 0) { fail(fieldToken, "duplicate initializer for ABI struct field '" + fieldToken.text + "'"); }
                fieldExprs.emplace(fieldToken.text, parseValueExpr());
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightBrace, "expected '}' after ABI struct initializer");
        expect(TokenKind::RightParen, "expected ')' after ABI struct initializer");
        for (const auto& f : def.fields) {
            if (fieldExprs.count(f.name) == 0) { fail(idToken, "ABI struct initializer for '" + id + "' is missing field '" + f.name + "'"); }
        }

        declareSymbol(idToken, id, structType);
        Step combined;
        std::ostringstream fields;
        std::ostringstream paths;
        std::ostringstream types;
        for (std::size_t i = 0; i < def.fields.size(); ++i) {
            const auto& f = def.fields[i];
            const Expr& expr = fieldExprs.at(f.name);
            const std::string actual = exprType(expr);
            if (!canAssignType(actual, f.type)) { throw flow::DiagnosticError{"lowerer", "field '" + f.name + "' of ABI struct '" + structType + "' expects " + f.type + " but got " + actual}; }
            Step fieldStep;
            const std::string fieldPath = lowerExprToPath(expr, generatedId("abi_struct_field_" + f.name), &fieldStep);
            appendStep(combined, fieldStep);
            if (i > 0) { fields << ','; paths << ','; types << ','; }
            fields << f.name;
            paths << fieldPath;
            types << f.type;
        }
        const std::string nodeId = generatedId("abi_struct");
        addNode("node", nodeId, "abi.struct.from_fields");
        addPolicy(nodeId, "type", structType);
        addPolicy(nodeId, "fields", fields.str());
        addPolicy(nodeId, "field_paths", paths.str());
        addPolicy(nodeId, "field_types", types.str());
        addPolicy(nodeId, "out", lookup(id)->path);
        appendStep(combined, Step{false, false, {nodeId, "in"}, {nodeId, "out"}});
        return combined;
    }

    [[nodiscard]] Step parseRecordValueDeclaration(const Token& idToken, const std::string& id) {
        const std::string recordType = expectIdentifier("expected record type").text;
        const TypeDef* def = lookupType(recordType);
        if (def == nullptr || def->fields.empty()) { fail(idToken, "unknown record type '" + recordType + "'"); }
        expect(TokenKind::LeftParen, "record declaration requires initializer");
        expect(TokenKind::LeftBrace, "record initializer must start with '{'");

        std::map<std::string, Expr> fieldExprs;
        if (!check(TokenKind::RightBrace)) {
            while (true) {
                const Token& fieldToken = expectIdentifier("expected record initializer field name");
                expect(TokenKind::Colon, "expected ':' after record initializer field name");
                bool known = false;
                for (const auto& f : def->fields) { if (f.name == fieldToken.text) { known = true; break; } }
                if (!known) { fail(fieldToken, "record type '" + recordType + "' has no field '" + fieldToken.text + "'"); }
                if (fieldExprs.count(fieldToken.text) != 0) { fail(fieldToken, "duplicate initializer for record field '" + fieldToken.text + "'"); }
                fieldExprs.emplace(fieldToken.text, parseValueExpr());
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightBrace, "expected '}' after record initializer");
        expect(TokenKind::RightParen, "expected ')' after record initializer");
        for (const auto& f : def->fields) {
            if (fieldExprs.count(f.name) == 0) { fail(idToken, "record initializer for '" + id + "' is missing field '" + f.name + "'"); }
        }

        declareSymbol(idToken, id, recordType);
        Step combined;
        std::ostringstream fields;
        std::ostringstream paths;
        std::ostringstream types;
        for (std::size_t i = 0; i < def->fields.size(); ++i) {
            const auto& f = def->fields[i];
            const Expr& expr = fieldExprs.at(f.name);
            const std::string actual = exprType(expr);
            if (!canAssignType(actual, f.type)) { throw flow::DiagnosticError{"lowerer", "field '" + f.name + "' of record type '" + recordType + "' expects " + f.type + " but got " + actual}; }
            Step fieldStep;
            const std::string fieldPath = lowerExprToPath(expr, generatedId("record_field_" + f.name), &fieldStep);
            appendStep(combined, fieldStep);
            if (i > 0) { fields << ','; paths << ','; types << ','; }
            fields << f.name;
            paths << fieldPath;
            types << f.type;
        }
        const std::string nodeId = generatedId("record_value");
        addNode("node", nodeId, "record.from_fields");
        addPolicy(nodeId, "type", recordType);
        addPolicy(nodeId, "fields", fields.str());
        addPolicy(nodeId, "field_paths", paths.str());
        addPolicy(nodeId, "field_types", types.str());
        addPolicy(nodeId, "out", lookup(id)->path);
        appendStep(combined, Step{false, false, {nodeId, "in"}, {nodeId, "out"}});
        return combined;
    }

    [[nodiscard]] Step parseListDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'list'");
        expect(TokenKind::Less, "expected '<' in list<int> declaration");
        const std::string elementType = expectIdentifier("expected list element type").text;
        const bool integerList = elementType == "int";
        const bool recordList = isRecordType(elementType);
        if (!integerList && !recordList) { fail(peek(), "list element type must be int or a declared record type"); }
        expect(TokenKind::Greater, "expected '>' in list<int> declaration");
        expect(TokenKind::LeftParen, "list declaration requires initializer");
        if (integerList && check(TokenKind::Identifier) && peek().text == "stdin" && lookahead(1).kind == TokenKind::Dot) {
            Expr initializer = parseValueExpr();
            expect(TokenKind::RightParen, "expected ')' after list initializer");
            if (initializer.kind != ExprKind::StdinBytes) { fail(peek(), "list<int> provider initializer requires stdin.bytes()"); }
            declareSymbol(idToken, id, "list<int>");
            const std::string path = lookup(id)->path;
            const std::string stdinId = generatedId("stdin_bytes");
            addNode("producer", stdinId, "stdin.bytes");
            addPolicy(stdinId, "out", path);
            return Step{false, false, {stdinId, "out"}, {stdinId, "out"}};
        }
        expect(TokenKind::LeftBracket, "list<int> initializer must start with '['");
        std::ostringstream values;
        std::ostringstream recordPaths;
        bool first = true;
        if (!check(TokenKind::RightBracket)) {
            while (true) {
                if (integerList) {
                    const Token& token = expect(TokenKind::Number, "expected integer in list initializer");
                    if (!first) { values << ','; }
                    values << parseIntToken(token);
                } else {
                    const Token& token = expectIdentifier("expected record value in list initializer");
                    const Symbol* symbol = lookup(token.text);
                    if (symbol == nullptr || symbol->type != elementType) {
                        fail(token, "record list initializer expects a value of type " + elementType);
                    }
                    if (!first) { recordPaths << ','; }
                    recordPaths << symbol->path;
                }
                first = false;
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightBracket, "expected ']' after list initializer");
        expect(TokenKind::RightParen, "expected ')' after list initializer");
        const std::string listType = "list<" + elementType + ">";
        declareSymbol(idToken, id, listType);
        if (recordList) {
            const std::string nodeId = generatedId("list_records");
            addNode("node", nodeId, "list.from_records");
            addPolicy(nodeId, "records", recordPaths.str());
            addPolicy(nodeId, "out", lookup(id)->path);
            return Step{false, false, {nodeId, "in"}, {nodeId, "out"}};
        }
        if (scopes_.size() == 1) {
            currentScope().symbols[id].path = id;
            addNode("node", id, "list.from_ints"); addPolicy(id, "out", id); addPolicy(id, "values", values.str());
            return Step{};
        }
        const std::string nodeId = generatedId("list");
        addNode("node", nodeId, "list.from_ints"); addPolicy(nodeId, "out", lookup(id)->path); addPolicy(nodeId, "values", values.str());
        return Step{false, false, {nodeId, "in"}, {nodeId, "out"}};
    }


    [[nodiscard]] Step parseArrayDeclaration(const Token& idToken, const std::string& id) {
        expectIdentifier("expected 'array'");
        expect(TokenKind::Less, "expected '<' in array<int> declaration");
        const std::string elementType = expectIdentifier("expected array element type").text;
        if (elementType != "int") { fail(peek(), "only array<int> is supported in flowmini v11 sugar"); }
        expect(TokenKind::Greater, "expected '>' in array<int> declaration");
        expect(TokenKind::LeftBracket, "expected '[' before array shape");
        std::vector<int> shape;
        while (true) {
            const Token& token = expect(TokenKind::Number, "expected positive integer dimension in array shape");
            const int dim = parseIntToken(token);
            if (dim <= 0) { fail(token, "array dimensions must be positive"); }
            shape.push_back(dim);
            if (!match(TokenKind::Comma)) { break; }
        }
        expect(TokenKind::RightBracket, "expected ']' after array shape");
        expect(TokenKind::LeftParen, "array declaration requires initializer");
        expect(TokenKind::LeftBracket, "array<int> initializer must start with '['");
        std::ostringstream values;
        bool first = true;
        std::size_t valueCount = 0;
        if (!check(TokenKind::RightBracket)) {
            while (true) {
                const Token& token = expect(TokenKind::Number, "expected integer in flattened array initializer");
                if (!first) { values << ','; }
                values << parseIntToken(token); first = false; ++valueCount;
                if (!match(TokenKind::Comma)) { break; }
            }
        }
        expect(TokenKind::RightBracket, "expected ']' after array initializer");
        expect(TokenKind::RightParen, "expected ')' after array initializer");
        std::size_t expected = 1;
        for (const int dim : shape) { expected *= static_cast<std::size_t>(dim); }
        if (valueCount != expected) { throw flow::DiagnosticError{"lowerer", "array initializer for '" + id + "' has " + std::to_string(valueCount) + " values but shape requires " + std::to_string(expected)}; }
        declareArraySymbol(idToken, id, shape);
        if (scopes_.size() == 1) {
            currentScope().symbols[id].path = id;
            addNode("node", id, "array.from_ints"); addPolicy(id, "out", id); addPolicy(id, "shape", joinInts(shape)); addPolicy(id, "values", values.str());
            return Step{};
        }
        const std::string nodeId = generatedId("array");
        addNode("node", nodeId, "array.from_ints"); addPolicy(nodeId, "out", lookup(id)->path); addPolicy(nodeId, "shape", joinInts(shape)); addPolicy(nodeId, "values", values.str());
        return Step{false, false, {nodeId, "in"}, {nodeId, "out"}};
    }

    [[nodiscard]] Step lowerAssignmentToTarget(const Expr& expr, const Target& target) {
        const Symbol* targetSymbol = lookup(target.ident);
        if (targetSymbol == nullptr && target.ident == "return" && !activeReturnType_.empty()) {
            currentScope().symbols["return"] = Symbol{activeReturnType_, pathFor("return"), currentScope().qualifiedName + "::return", {}, false, {}, {}};
            targetSymbol = lookup("return");
        }
        if (targetSymbol == nullptr) {
            throw flow::DiagnosticError{"lowerer", "cannot assign to undeclared target '" + target.ident + "'"};
        }
        if (targetSymbol->immutable) {
            throw flow::DiagnosticError{"lowerer", "cannot mutate constant '" + target.ident + "'"};
        }
        const std::string actual = exprType(expr);
        const std::string wanted = targetType(target);
        if (!canAssignType(actual, wanted)) { throw flow::DiagnosticError{"lowerer", "type mismatch assigning " + actual + " to " + wanted + " target '" + target.ident + "'"}; }

        if (target.kind == Target::Kind::Identifier) {
            const Symbol* targetSym = lookup(target.ident);
            const std::string targetPath = targetSym->path;
            Step step;
            if (expr.kind == ExprKind::Identifier) {
                const Symbol* sourceSym = lookup(expr.ident);
                if (sourceSym->constantInt.has_value() || sourceSym->constantBool.has_value()) {
                    static_cast<void>(lowerExprToPath(expr, targetPath, &step));
                } else {
                    const std::string id = generatedId("copy");
                    addNode("node", id, "record.copy");
                    addPolicy(id, "from", sourceSym->path); addPolicy(id, "to", targetPath);
                    step = Step{false, false, {id, "in"}, {id, "out"}};
                }
            } else {
                static_cast<void>(lowerExprToPath(expr, targetPath, &step));
            }
            return step;
        }

        if (target.kind == Target::Kind::FieldAccess) {
            const Symbol* recordSym = lookup(target.ident);
            const std::string recordPath = recordSym->path;
            Step valueStep;
            const std::string valuePath = lowerExprToPath(expr, generatedId("set_field_value"), &valueStep);
            Step combined;
            appendStep(combined, valueStep);
            const std::string id = generatedId("field_set");
            addNode("node", id, "record.field.set");
            addPolicy(id, "record", recordPath);
            addPolicy(id, "fields", joinFields(target.fields));
            addPolicy(id, "value", valuePath);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            return combined;
        }

        const Symbol* containerSym = lookup(target.ident);
        const std::string containerType = containerSym->type;
        const std::string containerPath = containerSym->path;
        Step valueStep;
        const std::string valuePath = lowerExprToPath(expr, generatedId("set_value"), &valueStep);
        Step combined;
        appendStep(combined, valueStep);
        if (target.kind == Target::Kind::ListIndex) {
            Step indexStep;
            const std::string indexPath = lowerExprToPath(*target.index, generatedId("set_index"), &indexStep);
            const bool isList = containerType == "list<int>";
            const std::string id = generatedId(isList ? "list_set" : "array_set");
            addNode("node", id, isList ? "list.set" : "array.set");
            if (isList) { addPolicy(id, "list", containerPath); addPolicy(id, "index", indexPath); }
            else { addPolicy(id, "array", containerPath); addPolicy(id, "indices", indexPath); }
            addPolicy(id, "value", valuePath);
            appendStep(combined, indexStep);
            appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
            return combined;
        }
        std::ostringstream paths;
        for (std::size_t i = 0; i < target.indices.size(); ++i) {
            Step indexStep;
            const std::string indexPath = lowerExprToPath(target.indices[i], generatedId("set_arr_idx"), &indexStep);
            appendStep(combined, indexStep);
            if (i > 0) { paths << ','; }
            paths << indexPath;
        }
        const std::string id = generatedId("array_set");
        addNode("node", id, "array.set");
        addPolicy(id, "array", containerPath); addPolicy(id, "indices", paths.str()); addPolicy(id, "value", valuePath);
        appendStep(combined, Step{false, false, {id, "in"}, {id, "out"}});
        return combined;
    }

    [[nodiscard]] Step parsePlacementAsStep() {
        Expr expr = parsePredicateExpr();
        expect(TokenKind::PlaceArrow, "expected '->' in placement");
        Target target = parseAssignableTarget();
        if (target.kind == Target::Kind::Identifier && target.ident == "return") { currentFunctionSawReturn_ = true; }
        return lowerAssignmentToTarget(expr, target);
    }

    [[nodiscard]] Step parsePrintStatement() {
        expectIdentifier("expected 'print'");
        Expr expr = parseValueExpr();
        const std::string type = exprType(expr);
        Step step;
        if (isIntLikeType(type)) {
            std::string path;
            if (expr.kind == ExprKind::Identifier) { path = lookup(expr.ident)->path; }
            else { path = lowerExprToPath(expr, generatedId("print_tmp"), &step); }
            const std::string id = generatedId("print");
            addNode("node", id, "stdout.int_line"); addPolicy(id, "path", path);
            appendStep(step, Step{false, false, {id, "in"}, {id, "out"}});
            return step;
        }
        if (type == "Bool") {
            std::string path;
            if (expr.kind == ExprKind::Identifier) { path = lookup(expr.ident)->path; }
            else { path = lowerExprToPath(expr, generatedId("print_bool_tmp"), &step); }
            const std::string id = generatedId("print_bool");
            addNode("node", id, "stdout.bool_line"); addPolicy(id, "path", path);
            appendStep(step, Step{false, false, {id, "in"}, {id, "out"}});
            return step;
        }
        if (type == "list<int>" && expr.kind == ExprKind::Identifier) {
            const std::string id = generatedId("print_list");
            addNode("node", id, "list.print_int_lines"); addPolicy(id, "list", lookup(expr.ident)->path);
            return Step{false, false, {id, "in"}, {id, "out"}};
        }
        if (type == "array<int>" && expr.kind == ExprKind::Identifier) {
            const std::string id = generatedId("print_array");
            addNode("node", id, "array.print_int"); addPolicy(id, "array", lookup(expr.ident)->path);
            return Step{false, false, {id, "in"}, {id, "out"}};
        }
        throw flow::DiagnosticError{"lowerer", "print currently supports int expressions, Bool expressions, list<int> identifiers, and array<int> identifiers only"};
    }

    [[nodiscard]] Step parseIfStatement() {
        expect(TokenKind::KeywordIf, "expected 'if'");
        const int ifIndex = ++ifCounter_;

        Expr cond = parsePredicateExpr();
        if (exprType(cond) != "Bool") { throw flow::DiagnosticError{"lowerer", "if condition must be Bool"}; }
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
        if (trueBody.empty) { fail(peek(), "if true block may not be empty in flowmini v11"); }

        const std::size_t afterTrueBrace = pos_;
        skipNewlines();
        Step falseBody;
        const bool hasElse = match(TokenKind::KeywordElse);
        if (!hasElse) { pos_ = afterTrueBrace; }
        if (hasElse) {
            if (check(TokenKind::KeywordIf)) {
                falseBody = parseIfStatement();
            } else {
                expect(TokenKind::LeftBrace, "expected '{' or 'if' after else");
                skipNewlines();
                enterScope("if" + std::to_string(ifIndex) + "_false");
                falseBody = parseBlockStatementsUntilRightBrace();
                leaveScope();
                if (falseBody.empty) { fail(peek(), "else block may not be empty in flowmini v11"); }
            }
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

    [[nodiscard]] Step parseGuardStatement() {
        expectIdentifier("expected 'guard'");
        const int guardIndex = ++ifCounter_;

        Expr cond = parsePredicateExpr();
        if (exprType(cond) != "Bool") { throw flow::DiagnosticError{"lowerer", "guard condition must be Bool"}; }
        expect(TokenKind::KeywordElse, "expected 'else' after guard condition");
        expect(TokenKind::LeftBrace, "expected '{' after guard else");
        skipNewlines();

        Step condStep;
        const std::string condPath = lowerExprToPath(cond, generatedId("guard" + std::to_string(guardIndex) + "_cond_path"), &condStep);
        const std::string routeId = generatedId("guard" + std::to_string(guardIndex) + "_route");
        addNode("node", routeId, "route.bool");
        addPolicy(routeId, "path", condPath);

        const std::string joinId = generatedId("guard" + std::to_string(guardIndex) + "_join");
        addNode("node", joinId, "record.nop");
        appendStep(condStep, Step{false, false, {routeId, "in"}, {routeId, "false"}});

        enterScope("guard" + std::to_string(guardIndex) + "_false");
        Step falseBody = parseBlockStatementsUntilRightBrace();
        leaveScope();
        if (falseBody.empty) { fail(peek(), "guard else block may not be empty"); }

        addWire({routeId, "true"}, {joinId, "in"});
        addWire({routeId, "false"}, falseBody.first);
        if (!falseBody.terminates) { addWire(falseBody.last, {joinId, "in"}); }
        return Step{false, false, condStep.first, {joinId, "out"}};
    }

    [[nodiscard]] Step parseWhenStatement() {
        expectIdentifier("expected 'when'");
        Expr selector = parseValueExpr();
        if (selector.kind != ExprKind::Identifier || (!isIntLikeType(exprType(selector)) && !isEnumType(exprType(selector)) && !isVariantType(exprType(selector)))) {
            throw flow::DiagnosticError{"lowerer", "when requires an integer, enum, or variant identifier selector"};
        }
        expect(TokenKind::LeftBrace, "expected '{' after when selector");
        skipNewlines();

        std::vector<WhenCase> cases;
        std::set<int> seenValues;
        bool hasDefault = false;
        while (!check(TokenKind::RightBrace) && !check(TokenKind::End)) {
            skipNewlines();
            if (check(TokenKind::RightBrace) || check(TokenKind::End)) { break; }

            std::optional<int> value;
            std::string caseVariantMember;
            if (check(TokenKind::Identifier) && peek().text == "case") {
                static_cast<void>(expectIdentifier("expected 'case'"));
                int caseValue = 0;
                if (check(TokenKind::Number)) {
                    caseValue = parseIntToken(expect(TokenKind::Number, "expected integer literal after 'case'"));
                } else {
                    const Token& enumType = expectIdentifier("expected integer literal or enum member after 'case'");
                    expect(TokenKind::Dot, "expected '.' between enum type and member");
                    const Token& member = expectIdentifier("expected enum member after '.'");
                    const TypeDef* enumDef = lookupType(enumType.text);
                    if (enumDef == nullptr || (!enumDef->isEnum && !enumDef->isVariant)) { fail(enumType, "unknown enum or variant type in when case"); }
                    int foundValue = -1;
                    if (enumDef->isEnum) {
                        const auto found = enumDef->enumMembers.find(member.text);
                        if (found == enumDef->enumMembers.end()) { fail(member, "unknown enum member '" + member.text + "'"); }
                        foundValue = found->second;
                    } else {
                        int ordinal = 0;
                        for (const auto& [name, payload] : enumDef->variantMembers) {
                            if (name == member.text) { foundValue = ordinal; break; }
                            ++ordinal;
                        }
                        if (foundValue < 0) { fail(member, "unknown variant member '" + member.text + "'"); }
                        if (enumDef->isVariant) { caseVariantMember = member.text; }
                    }
                    if (exprType(selector) != enumType.text) { fail(enumType, "when case enum type does not match selector"); }
                    caseValue = foundValue;
                }
                int highValue = caseValue;
                if (match(TokenKind::Dot)) {
                    expect(TokenKind::Dot, "expected '..' in when range case");
                    highValue = parseIntToken(expect(TokenKind::Number, "expected range end after '..'"));
                    if (highValue < caseValue || highValue - caseValue > 256) {
                        throw flow::DiagnosticError{"lowerer", "when range must be ascending and at most 257 values"};
                    }
                }
                for (int candidate = caseValue; candidate <= highValue; ++candidate) {
                    if (!seenValues.insert(candidate).second) {
                        throw flow::DiagnosticError{"lowerer", "duplicate when case value " + std::to_string(candidate)};
                    }
                }
                value = caseValue;
                cases.push_back(WhenCase{value, highValue, {}});
            } else if (check(TokenKind::Identifier) && peek().text == "default") {
                static_cast<void>(expectIdentifier("expected 'default'"));
                if (hasDefault) { throw flow::DiagnosticError{"lowerer", "duplicate when default arm"}; }
                hasDefault = true;
            } else {
                fail(peek(), "expected 'case' or 'default' in when block");
            }

            expect(TokenKind::LeftBrace, "expected '{' after when arm");
            skipNewlines();
            const std::string savedVariantType = activeVariantArmType_;
            const std::string savedVariantMember = activeVariantArmMember_;
            if (!caseVariantMember.empty()) {
                activeVariantArmType_ = exprType(selector);
                activeVariantArmMember_ = caseVariantMember;
            } else {
                activeVariantArmType_.clear();
                activeVariantArmMember_.clear();
            }
            enterScope("when_arm" + std::to_string(cases.size()));
            Step body = parseBlockStatementsUntilRightBrace();
            leaveScope();
            activeVariantArmType_ = savedVariantType;
            activeVariantArmMember_ = savedVariantMember;
            if (body.empty) { fail(peek(), "when arm may not be empty"); }
            if (!cases.empty() && cases.back().value == value && cases.back().body.empty) {
                cases.back().body = body;
                cases.back().high = value.has_value() ? cases.back().high : 0;
            } else {
                cases.push_back(WhenCase{value, value.value_or(0), body});
            }
            skipNewlines();
        }
        expect(TokenKind::RightBrace, "expected '}' to close when block");
        if (!hasDefault) {
            const std::string selectorType = exprType(selector);
            if (!isEnumType(selectorType)) {
                throw flow::DiagnosticError{"lowerer", "when requires a default arm for open integer or variant selectors"};
            }
            const TypeDef* enumDef = lookupType(selectorType);
            for (const auto& [memberName, memberValue] : enumDef->enumMembers) {
                if (!seenValues.contains(memberValue)) {
                    throw flow::DiagnosticError{"lowerer", "exhaustive enum when is missing case " + selectorType + "." + memberName};
                }
            }
        }

        const std::string joinId = generatedId("when_join");
        addNode("node", joinId, "record.nop");
        struct RouteChain { ChainEnd first; ChainEnd failure; };
        std::vector<RouteChain> routes;
        std::optional<Step> defaultBody;
        auto addConditionRoute = [&](TokenKind op, int literal, bool negate = false) {
            Expr left;
            if (isVariantType(exprType(selector))) {
                left.kind = ExprKind::FieldAccess;
                left.ident = selector.ident;
                left.fields = {"__tag"};
            } else {
                left.kind = ExprKind::Identifier;
                left.ident = selector.ident;
            }
            Expr right;
            right.kind = ExprKind::LiteralInt;
            right.literal = literal;
            Expr condition;
            condition.kind = ExprKind::Binary;
            condition.op = op;
            condition.left = std::make_unique<Expr>(std::move(left));
            condition.right = std::make_unique<Expr>(std::move(right));
            if (negate) {
                Expr inverted;
                inverted.kind = ExprKind::UnaryNot;
                inverted.left = std::make_unique<Expr>(std::move(condition));
                condition = std::move(inverted);
            }
            Step condStep;
            const std::string condPath = lowerExprToPath(condition, generatedId("when_case_cond"), &condStep);
            const std::string routeId = generatedId("when_case_route");
            addNode("node", routeId, "route.bool");
            addPolicy(routeId, "path", condPath);
            appendStep(condStep, Step{false, false, {routeId, "in"}, {routeId, "false"}});
            return RouteChain{condStep.first, {routeId, "false"}};
        };
        for (auto& arm : cases) {
            if (!arm.value.has_value()) {
                defaultBody = std::move(arm.body);
                continue;
            }
            RouteChain chain = addConditionRoute(
                arm.high == *arm.value ? TokenKind::EqualEqual : TokenKind::Greater,
                arm.high == *arm.value ? *arm.value : *arm.value - 1);
            if (arm.high != *arm.value) {
                RouteChain upper = addConditionRoute(TokenKind::Greater, arm.high, true);
                addWire({chain.failure.node, "true"}, upper.first);
                routes.push_back(chain);
                routes.push_back(upper);
                addWire({upper.failure.node, "true"}, arm.body.first);
            } else {
                routes.push_back(chain);
                addWire({chain.failure.node, "true"}, arm.body.first);
            }
            if (!arm.body.terminates) { addWire(arm.body.last, {joinId, "in"}); }
        }

        if (!defaultBody.has_value()) {
            const std::string unreachableId = generatedId("when_unreachable");
            addNode("node", unreachableId, "record.nop");
            defaultBody = Step{false, false, {unreachableId, "in"}, {unreachableId, "out"}};
        }
        if (routes.empty()) {
            return Step{false, defaultBody->terminates, defaultBody->first, defaultBody->last};
        }
        addWire(routes.back().failure, defaultBody->first);
        if (!defaultBody->terminates) { addWire(defaultBody->last, {joinId, "in"}); }
        for (std::size_t i = 0; i + 1 < routes.size(); ++i) {
            addWire(routes[i].failure, routes[i + 1].first);
        }
        return Step{false, false, routes.front().first, {joinId, "out"}};
    }

    [[nodiscard]] Step parseWhileStatement() {
        expect(TokenKind::KeywordWhile, "expected 'while'");
        const int whileIndex = ++whileCounter_;
        enterScope("while" + std::to_string(whileIndex));
        Expr cond = parsePredicateExpr();
        if (exprType(cond) != "Bool") { throw flow::DiagnosticError{"lowerer", "while condition must be Bool"}; }
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

        if (body.empty) { fail(peek(), "while body may not be empty in flowmini v11"); }
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
        if (check(TokenKind::Identifier) && peek().text == "guard") { Step s = parseGuardStatement(); expectLineEnd("expected newline after guard block"); return s; }
        if (check(TokenKind::Identifier) && peek().text == "when") { Step s = parseWhenStatement(); expectLineEnd("expected newline after when block"); return s; }
        if (check(TokenKind::KeywordBreak)) { Step s = parseBreakStatement(); expectLineEnd("expected newline after break"); return s; }
        if (check(TokenKind::KeywordContinue)) { Step s = parseContinueStatement(); expectLineEnd("expected newline after continue"); return s; }
        if (looksLikePlacement()) { Step s = parsePlacementAsStep(); expectLineEnd("expected newline after placement"); return s; }
        if (check(TokenKind::Identifier) && peek().text == "const") { Step s = parseConstDeclarationAsStep(); expectLineEnd("expected newline after constant declaration"); return s; }
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
        if (check(TokenKind::LeftParen)) {
            std::size_t depth = 0;
            do {
                if (check(TokenKind::LeftParen)) { ++depth; }
                if (check(TokenKind::RightParen) && depth > 0) { --depth; }
                ++pos_;
            } while (depth > 0 && !check(TokenKind::End));
        }
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
    std::map<std::string, FunctionDef> functions_;
    std::set<std::string> ambiguousFunctionNames_;
    std::map<std::string, TypeDef> types_;
    std::map<std::string, AbiTypeDef> abiTypes_;
    std::map<std::string, AbiStructDef> abiStructs_;
    std::vector<std::string> functionCallStack_;
    int functionInstanceCounter_ = 0;
    bool currentFunctionSawReturn_ = false;
    std::string activeReturnType_;
    std::string activeVariantArmType_;
    std::string activeVariantArmMember_;
    bool mainSeen_ = false;
    bool declaringConstant_ = false;
    std::string callStepResultPath_;
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
