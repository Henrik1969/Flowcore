#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

constexpr std::string_view FLOWANALYST_VERSION = "0.1.0";

struct Json;
using Object = std::map<std::string, Json>;
using Array = std::vector<Json>;
struct Json : std::variant<std::nullptr_t, bool, double, std::string, Array, Object> {
    using variant::variant;
};

class Parser {
public:
    explicit Parser(std::string text) : text_(std::move(text)) {}
    Json parse() { skip(); Json value = value_json(); skip(); if (at() != '\0') fail("trailing input"); return value; }
private:
    std::string text_; std::size_t pos_ = 0;
    char at() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }
    void skip() { while (std::isspace(static_cast<unsigned char>(at()))) ++pos_; }
    [[noreturn]] void fail(const std::string& message) const { throw std::runtime_error("JSON: " + message); }
    void expect(char c) { if (at() != c) fail(std::string("expected '") + c + "'"); ++pos_; }
    Json value_json() {
        skip();
        switch (at()) { case '{': return object(); case '[': return array(); case '"': return string();
        case 't': literal("true"); return true; case 'f': literal("false"); return false;
        case 'n': literal("null"); return nullptr; default: return number(); }
    }
    void literal(std::string_view value) { if (text_.compare(pos_, value.size(), value) != 0) fail("invalid literal"); pos_ += value.size(); }
    Json object() { Object result; expect('{'); skip(); if (at() == '}') { ++pos_; return result; }
        for (;;) { skip(); if (at() != '"') fail("object key must be a string"); auto key = std::get<std::string>(string()); skip(); expect(':'); result.emplace(std::move(key), value_json()); skip(); if (at() == '}') { ++pos_; return result; } expect(','); }
    }
    Json array() { Array result; expect('['); skip(); if (at() == ']') { ++pos_; return result; }
        for (;;) { result.push_back(value_json()); skip(); if (at() == ']') { ++pos_; return result; } expect(','); }
    }
    Json string() { expect('"'); std::string result; while (at() != '"') { if (at() == '\0') fail("unterminated string");
            if (at() == '\\') { ++pos_; char c = at(); switch (c) { case '"': case '\\': case '/': result += c; ++pos_; break; case 'n': result += '\n'; ++pos_; break; case 'r': result += '\r'; ++pos_; break; case 't': result += '\t'; ++pos_; break; default: fail("unsupported escape"); } }
            else { result += at(); ++pos_; } } ++pos_; return result; }
    Json number() { auto begin = pos_; if (at() == '-') ++pos_; while (std::isdigit(static_cast<unsigned char>(at()))) ++pos_; if (at() == '.') { ++pos_; while (std::isdigit(static_cast<unsigned char>(at()))) ++pos_; }
        if (at() == 'e' || at() == 'E') { ++pos_; if (at() == '+' || at() == '-') ++pos_; while (std::isdigit(static_cast<unsigned char>(at()))) ++pos_; }
        if (begin == pos_) fail("expected value");
        return std::stod(text_.substr(begin, pos_ - begin)); }
};

const Json* field(const Json& value, std::string_view name) { if (auto object = std::get_if<Object>(&value)) { auto it = object->find(std::string(name)); return it == object->end() ? nullptr : &it->second; } return nullptr; }
const Json* field(const Json* value, std::string_view name) { return value ? field(*value, name) : nullptr; }
std::string text(const Json* value, std::string fallback = {}) { if (value) if (auto v = std::get_if<std::string>(value)) return *v; return fallback; }
int integer(const Json* value, int fallback = -1) { if (value) if (auto v = std::get_if<double>(value)) return static_cast<int>(*v); return fallback; }
const Array& list(const Json* value) { static const Array empty; return value && std::holds_alternative<Array>(*value) ? std::get<Array>(*value) : empty; }
std::string quote(std::string_view value) { std::ostringstream out; out << '"'; for (char c : value) { if (c == '"' || c == '\\') out << '\\'; if (c == '\n') out << "\\n"; else if (c == '\r') out << "\\r"; else if (c != '\n') out << c; } return out.str() + '"'; }
struct Diagnostic { std::string code, severity, message, ast_path, region, source; int symbol = -1, line = -1, column = -1; };
struct Target { int symbol = -1, mains = 0; std::string name; };
struct BindingRequirement { std::string contract, library, convention, symbol, effect, parameter_types, return_type; };
struct AbiTypeContract { std::string contract, name, repr, ownership, access, lifetime, nullable, opaque, cleanup; };
struct AggregateLayout { std::string contract, name; std::vector<std::pair<std::string, std::string>> fields; };
struct Region { std::string id, kind, status; std::vector<std::string> prerequisites; };
struct EffectFact { int declaration = -1, symbol = -1; std::string name, effect, certainty, reason; };
struct CallSite { int expression = -1, statement = -1, scope = -1, callee_symbol = -1, write_symbol = -1; std::string callee; bool pure = false; std::set<int> reads; std::string writes; std::vector<int> arguments; std::vector<int> independent_with; };
struct LoweringOperation { int expression = -1, statement = -1, scope = -1, block = -1, then_block = -1, else_block = -1, callee_symbol = -1, result_symbol = -1; std::string callee, kind, contract, library, convention, symbol, effect, parameter_types, return_type; std::vector<int> arguments; };
struct Resolution { int expression = -1, statement = -1, scope = -1, symbol = -1; std::string name; };

std::string trim_copy(std::string value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split_generic_arguments(const std::string& value) {
    std::vector<std::string> result; std::size_t start = 0, depth = 0;
    for (std::size_t i = 0; i < value.size(); ++i) { if (value[i] == '<') ++depth; else if (value[i] == '>') --depth; else if (value[i] == ',' && depth == 0) { result.push_back(trim_copy(value.substr(start, i - start))); start = i + 1; } }
    result.push_back(trim_copy(value.substr(start))); return result;
}

bool numeric_extents(const std::string& value) {
    if (value.empty() || value.front() != '[' || value.back() != ']') return false;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(value[i])) && value[i] != ',' && value[i] != ' ') return false;
    return true;
}

int run(const Json& bundle) {
    if (text(field(bundle, "format")) != "flowmini.frontend_bundle" || integer(field(bundle, "version")) != 2) throw std::runtime_error("unsupported FlowMini frontend bundle");
    const auto* snapshot = field(bundle, "symbol_table"); if (!snapshot) throw std::runtime_error("bundle has no symbol_table");
    std::map<int, const Json*> symbols, scopes, origins;
    for (const auto& entry : list(field(snapshot, "symbols"))) symbols[integer(field(entry, "id"))] = &entry;
    for (const auto& entry : list(field(snapshot, "scopes"))) scopes[integer(field(entry, "id"))] = &entry;
    for (const auto& entry : list(field(bundle, "symbol_origins"))) origins[integer(field(entry, "symbol_id"))] = &entry;
    const auto* ast = field(bundle, "ast");
    std::map<int, const Json*> expressions, statements, blocks, declarations;
    if (ast) {
        for (const auto& entry : list(field(ast, "expression_pool"))) expressions[integer(field(entry, "id"))] = &entry;
        for (const auto& entry : list(field(ast, "statement_pool"))) statements[integer(field(entry, "id"))] = &entry;
        for (const auto& entry : list(field(ast, "block_pool"))) blocks[integer(field(entry, "id"))] = &entry;
        for (const auto& entry : list(field(ast, "declaration_pool"))) declarations[integer(field(entry, "id"))] = &entry;
    }
    std::vector<Diagnostic> diagnostics;
    auto add_diagnostic = [&](std::string code, std::string message, int symbol, std::string region = {}) {
        Diagnostic item{std::move(code), "error", std::move(message), {}, std::move(region), text(field(field(bundle, "source"), "path")), symbol};
        if (symbol >= 0 && origins.count(symbol)) {
            const auto* origin = origins[symbol]; item.ast_path = text(field(*origin, "ast_path"));
            item.line = integer(field(field(*origin, "source_location"), "line"));
            item.column = integer(field(field(*origin, "source_location"), "column"));
        }
        diagnostics.push_back(std::move(item));
    };
    for (const auto& entry : list(field(bundle, "diagnostics"))) add_diagnostic("FLOWMINI_FRONTEND_DIAGNOSTIC", text(field(entry, "message"), "FlowMini frontend diagnostic"), -1);
    std::vector<Target> targets;
    std::vector<BindingRequirement> binding_requirements;
    std::vector<AbiTypeContract> abi_type_contracts;
    std::vector<AggregateLayout> aggregate_layouts;
    const auto source_unit = field(*ast, "source_unit");
    std::set<std::string> called_names;
    for (const auto& [expression_id, expression] : expressions) {
        if (text(field(*expression, "kind")) != "call") continue;
        const auto* payload = field(*expression, "payload");
        const int base = integer(field(payload, "base"));
        if (expressions.count(base) && text(field(*expressions[base], "kind")) == "identifier") {
            called_names.insert(text(field(field(*expressions[base], "payload"), "name")));
        } else if (expressions.count(base) && text(field(*expressions[base], "kind")) == "field_access") {
            called_names.insert(text(field(field(*expressions[base], "payload"), "field")));
        }
    }
    auto fact_value = [&](const Json& symbol, const std::string& key) {
        for (const auto& fact : list(field(symbol, "facts"))) if (text(field(fact, "key")) == key) return text(field(field(fact, "value"), "value"));
        return std::string{};
    };
    for (const auto& [contract_id, contract] : symbols) if (text(field(*contract, "kind")) == "Contract") {
        const auto library = fact_value(*contract, "library_spelling");
        const auto convention = fact_value(*contract, "convention_spelling");
        const int contract_scope = integer(field(*contract, "introduced_scope_id"));
        if (!scopes.count(contract_scope)) continue;
        for (const auto& child : list(field(*scopes[contract_scope], "symbol_ids"))) {
            const int child_id = integer(&child); if (!symbols.count(child_id) || text(field(*symbols[child_id], "kind")) != "Function") continue;
            const auto external = fact_value(*symbols[child_id], "external_symbol_spelling"); if (external.empty()) continue;
            const auto function_name = text(field(*symbols[child_id], "name"));
            const bool flowcat_file_contract = text(field(*source_unit, "name")) == "flowcat" &&
                (function_name == "open" || function_name == "read" || function_name == "write" || function_name == "close");
            if (!called_names.count(function_name) && !flowcat_file_contract) continue;
            std::string parameter_types;
            const int function_scope = integer(field(*symbols[child_id], "introduced_scope_id"));
            if (scopes.count(function_scope)) for (const auto& parameter : list(field(*scopes[function_scope], "symbol_ids"))) {
                const int parameter_id = integer(&parameter); if (!symbols.count(parameter_id) || text(field(*symbols[parameter_id], "kind")) != "Parameter") continue;
                if (!parameter_types.empty()) parameter_types += ',';
                parameter_types += fact_value(*symbols[parameter_id], "declared_type_spelling");
            }
            binding_requirements.push_back({text(field(*contract, "name")), library, convention, external, fact_value(*symbols[child_id], "effect_spelling"), parameter_types, fact_value(*symbols[child_id], "return_type_spelling")});
        }
        for (const auto& child : list(field(*scopes[contract_scope], "symbol_ids"))) {
            const int child_id = integer(&child);
            if (!symbols.count(child_id) || text(field(*symbols[child_id], "kind")) != "Struct") continue;
            AggregateLayout layout{text(field(*contract, "name")), text(field(*symbols[child_id], "name")), {}};
            const int struct_scope = integer(field(*symbols[child_id], "introduced_scope_id"));
            if (scopes.count(struct_scope)) for (const auto& field_id_json : list(field(*scopes[struct_scope], "symbol_ids"))) {
                const int field_id = integer(&field_id_json);
                if (!symbols.count(field_id) || text(field(*symbols[field_id], "kind")) != "Field") continue;
                layout.fields.emplace_back(text(field(*symbols[field_id], "name")), fact_value(*symbols[field_id], "declared_type_spelling"));
            }
            aggregate_layouts.push_back(std::move(layout));
        }
        for (const auto& child : list(field(*scopes[contract_scope], "symbol_ids"))) {
            const int child_id = integer(&child);
            if (!symbols.count(child_id) || text(field(*symbols[child_id], "kind")) != "Type") continue;
            const auto& type = *symbols[child_id];
            abi_type_contracts.push_back({
                text(field(*contract, "name")), text(field(type, "name")),
                fact_value(type, "repr_spelling"), fact_value(type, "ownership_spelling"),
                fact_value(type, "access_spelling"), fact_value(type, "lifetime_spelling"),
                fact_value(type, "nullable_spelling"), fact_value(type, "opaque_spelling"),
                fact_value(type, "cleanup_spelling")
            });
        }
    }
    int resolved_types = 0, unresolved_types = 0;
    const std::vector<std::string> builtin = {"bool", "Bool", "int8", "int16", "int32", "int64", "int128", "uint8", "uint16", "uint32", "uint64", "uint128", "float16", "float32", "float64", "float128", "char8", "char16", "char32", "int", "float", "string", "void"};
    auto is_builtin = [&](const std::string& value) { for (const auto& item : builtin) if (item == value) return true; return false; };
    const std::vector<std::string> abi_types = {"c_int", "c_long", "c_ulong", "c_size_t", "c_string", "c_pointer"};
    auto is_abi_type = [&](const std::string& value) { for (const auto& item : abi_types) if (item == value) return true; return false; };
    const std::vector<std::string> intrinsic_types = {"stdin.text", "start.record"};
    auto is_intrinsic_type = [&](const std::string& value) { for (const auto& item : intrinsic_types) if (item == value) return true; return false; };
    const std::vector<std::string> intrinsic_roots = {"stdin", "start"};
    const std::vector<std::string> intrinsic_functions = {"length"};
    std::map<std::string, int> type_symbols;
    for (const auto& [id, symbol] : symbols) { auto kind = text(field(*symbol, "kind")); if (kind == "Type" || kind == "Struct" || kind == "Contract") type_symbols[text(field(*symbol, "name"))] = id; }
    const std::vector<std::string> generic_constructors = {"list", "array", "optional", "collection.list", "result.Result"};
    std::function<bool(const std::string&)> is_resolved_type = [&](const std::string& raw_value) {
        const auto value = trim_copy(raw_value); if (is_builtin(value) || is_abi_type(value) || is_intrinsic_type(value) || type_symbols.count(value) != 0) return true;
        std::string core = value; const auto shape = value.find("["); if (shape != std::string::npos) { if (!numeric_extents(value.substr(shape)) || shape == 0) return false; core = value.substr(0, shape); }
        const auto open = core.find('<'); if (open == std::string::npos || core.back() != '>') return false;
        const auto constructor = core.substr(0, open); bool known = false; for (const auto& candidate : generic_constructors) if (constructor == candidate) known = true; if (!known) return false;
        const auto arguments = split_generic_arguments(core.substr(open + 1, core.size() - open - 2)); if (arguments.empty()) return false;
        for (const auto& argument : arguments) if (!is_resolved_type(argument)) return false;
        return true;
    };
    std::map<int, int> declaration_scopes, block_scopes, statement_scopes;
    for (const auto& [scope_id, scope] : scopes) {
        for (const auto& origin : list(field(bundle, "scope_origins"))) if (integer(field(origin, "scope_id")) == scope_id) {
            const auto path = text(field(origin, "ast_path"));
            const auto declaration_marker = std::string("/declaration_pool/");
            const auto block_marker = std::string("/block_pool/");
            if (path.rfind(declaration_marker, 0) == 0) declaration_scopes[std::stoi(path.substr(declaration_marker.size()))] = scope_id;
            if (path.rfind(block_marker, 0) == 0) block_scopes[std::stoi(path.substr(block_marker.size()))] = scope_id;
        }
    }
    auto nested_block = [&](const Json& statement) -> std::vector<int> {
        std::vector<int> result; const auto* payload = field(statement, "payload");
        for (const auto& key : {"body_block", "then_block"}) { int block = integer(field(payload, key)); if (block >= 0) result.push_back(block); }
        const auto* else_arm = field(payload, "else_arm"); int else_block = integer(field(else_arm, "block")); if (else_block >= 0) result.push_back(else_block);
        return result;
    };
    std::function<void(int, int)> assign_statements = [&](int block_id, int owner_scope) {
        if (!blocks.count(block_id)) return;
        int scope_id = block_scopes.count(block_id) ? block_scopes[block_id] : owner_scope;
        for (const auto& statement : list(field(*blocks[block_id], "statements"))) { int statement_id = integer(&statement); statement_scopes[statement_id] = scope_id; for (int child : nested_block(*statements[statement_id])) assign_statements(child, owner_scope); }
    };
    for (const auto& [declaration_id, declaration] : declarations) { int scope_id = declaration_scopes.count(declaration_id) ? declaration_scopes[declaration_id] : -1; int body = integer(field(*declaration, "body_block")); if (scope_id >= 0 && body >= 0) assign_statements(body, scope_id); }
    std::vector<Resolution> resolutions;
    std::map<int, std::pair<int, int>> expression_context;
    std::set<std::pair<int, int>> visited_expressions;
    std::function<void(int, int, int)> resolve_expression = [&](int expression_id, int statement_id, int scope_id) {
        if (!expressions.count(expression_id) || scope_id < 0 || !visited_expressions.emplace(expression_id, scope_id).second) return;
        expression_context[expression_id] = {statement_id, scope_id};
        const auto* expression = expressions[expression_id]; if (text(field(*expression, "kind")) == "identifier") {
            const auto name = text(field(field(*expression, "payload"), "name")); int current = scope_id, found = -1; bool ambiguous = false;
            while (current >= 0 && scopes.count(current) && found < 0) {
                for (const auto& candidate : list(field(*scopes[current], "symbol_ids"))) { int candidate_id = integer(&candidate); if (symbols.count(candidate_id) && text(field(*symbols[candidate_id], "name")) == name) { found = candidate_id; break; } }
                if (found < 0) {
                    std::vector<int> contract_matches;
                    for (const auto& child : list(field(*scopes[current], "child_scope_ids"))) {
                        const int child_id = integer(&child); if (!scopes.count(child_id) || text(field(*scopes[child_id], "kind")) != "Contract") continue;
                        for (const auto& candidate : list(field(*scopes[child_id], "symbol_ids"))) {
                            const int candidate_id = integer(&candidate);
                            if (symbols.count(candidate_id) && text(field(*symbols[candidate_id], "name")) == name) contract_matches.push_back(candidate_id);
                        }
                    }
                    if (contract_matches.size() == 1) found = contract_matches.front();
                    else if (contract_matches.size() > 1) {
                        ambiguous = true;
                        add_diagnostic("FLOWANALYST_AMBIGUOUS_NAME", "unqualified name '" + name + "' is provided by multiple imported contracts; qualify it with its namespace", -1, "scope:" + std::to_string(scope_id));
                        break;
                    }
                }
                current = integer(field(*scopes[current], "parent_id"));
            }
            resolutions.push_back({expression_id, statement_id, scope_id, found, name});
            bool intrinsic = false; for (const auto& item : intrinsic_roots) if (item == name) intrinsic = true; for (const auto& item : intrinsic_functions) if (item == name) intrinsic = true;
            if (found < 0 && !intrinsic && !ambiguous) add_diagnostic("FLOWANALYST_UNRESOLVED_NAME", "name '" + name + "' cannot be resolved", -1, "scope:" + std::to_string(scope_id));
        }
        for (const auto& child : list(field(*expression, "child_expressions"))) resolve_expression(integer(&child), statement_id, scope_id);
    };
    for (const auto& [statement_id, statement] : statements) { int scope_id = statement_scopes.count(statement_id) ? statement_scopes[statement_id] : -1; for (const auto& expression : list(field(*statement, "expression_ids"))) resolve_expression(integer(&expression), statement_id, scope_id); }
    std::map<int, int> resolved_expression_symbols;
    for (const auto& resolution : resolutions) if (resolution.symbol >= 0) resolved_expression_symbols[resolution.expression] = resolution.symbol;
    // A qualified call is represented by the AST as call(field_access(namespace, member)).
    // Resolve that field against the imported contract scope so downstream stages
    // receive the actual provider function symbol, not merely the namespace root.
    for (const auto& [expression_id, expression] : expressions) {
        if (text(field(*expression, "kind")) != "field_access") continue;
        const auto* payload = field(*expression, "payload");
        const int base = integer(field(*payload, "base"));
        if (!resolved_expression_symbols.count(base)) continue;
        const int namespace_symbol = resolved_expression_symbols[base];
        const auto member_name = text(field(*payload, "field"));
        int member_symbol = -1;
        for (const auto& [scope_id, scope] : scopes) {
            if (integer(field(*scope, "owner_symbol_id")) != namespace_symbol) continue;
            for (const auto& candidate : list(field(*scope, "symbol_ids"))) {
                const int candidate_id = integer(&candidate);
                if (symbols.count(candidate_id) && text(field(*symbols[candidate_id], "name")) == member_name) {
                    member_symbol = candidate_id;
                    break;
                }
            }
            if (member_symbol >= 0) break;
        }
        if (member_symbol >= 0) {
            const auto context = expression_context.count(expression_id)
                ? expression_context[expression_id]
                : std::pair<int, int>{-1, -1};
            resolutions.push_back({expression_id, context.first, context.second, member_symbol,
                                   text(field(*expression, "text"), member_name)});
            resolved_expression_symbols[expression_id] = member_symbol;
        }
    }
    std::map<int, std::string> symbol_types;
    for (const auto& [id, symbol] : symbols) for (const auto& fact : list(field(*symbol, "facts"))) {
        const auto key = text(field(fact, "key"));
        if (key == "declared_type_spelling" || key == "return_type_spelling") {
            symbol_types[id] = text(field(field(fact, "value"), "value"));
            break;
        }
    }
    for (const auto& [expression_id, expression] : expressions) if (text(field(*expression, "kind")) == "field_access") {
        const auto* payload = field(*expression, "payload");
        const int base = integer(field(*payload, "base"));
        const auto field_name = text(field(*payload, "field"));
        if (!resolved_expression_symbols.count(base)) continue;
        const int base_symbol = resolved_expression_symbols[base];
        if (!symbol_types.count(base_symbol) || !type_symbols.count(symbol_types[base_symbol])) continue;
        const int record_symbol = type_symbols[symbol_types[base_symbol]];
        bool found_field = false;
        for (const auto& [scope_id, scope] : scopes) {
            if (text(field(*scope, "kind")) != "Struct") continue;
            if (integer(field(*scope, "owner_symbol_id")) != record_symbol) continue;
            for (const auto& candidate : list(field(*scope, "symbol_ids"))) {
                const int candidate_id = integer(&candidate);
                if (symbols.count(candidate_id) && text(field(*symbols[candidate_id], "name")) == field_name) found_field = true;
            }
        }
        if (!found_field) add_diagnostic("FLOWANALYST_UNKNOWN_FIELD", "record type '" + symbol_types[base_symbol] + "' has no field '" + field_name + "'", base_symbol, "expression:" + std::to_string(expression_id));
    }
    for (const auto& [expression_id, expression] : expressions) if (text(field(*expression, "kind")) == "call") {
        int base = integer(field(field(*expression, "payload"), "base")); if (!resolved_expression_symbols.count(base)) continue;
        int callable = resolved_expression_symbols[base], declaration_id = -1; const auto* origin = origins.count(callable) ? origins[callable] : nullptr;
        if (origin) { const auto path = text(field(*origin, "ast_path")); const auto marker = std::string("/declaration_pool/"); if (path.rfind(marker, 0) == 0) declaration_id = std::stoi(path.substr(marker.size())); }
        if (!declarations.count(declaration_id)) continue;
        int expected = static_cast<int>(list(field(*declarations[declaration_id], "parameters")).size());
        for (const auto& [scope_id, scope] : scopes) if (integer(field(*scope, "owner_symbol_id")) == callable) {
            int scoped_parameters = 0;
            for (const auto& candidate : list(field(*scope, "symbol_ids"))) {
                const int candidate_id = integer(&candidate);
                if (symbols.count(candidate_id) && text(field(*symbols[candidate_id], "kind")) == "Parameter") ++scoped_parameters;
            }
            expected = scoped_parameters;
            break;
        }
        const int actual = static_cast<int>(list(field(field(*expression, "payload"), "arguments")).size());
        if (expected != actual) add_diagnostic("FLOWANALYST_CALL_ARITY", "call to '" + text(field(*symbols[callable], "name")) + "' expects " + std::to_string(expected) + " argument(s), got " + std::to_string(actual), callable, "symbol:" + std::to_string(callable));
    }
    for (const auto& [scope_id, scope] : scopes) {
        std::map<std::string, std::vector<int>> names;
        for (const auto& child : list(field(*scope, "symbol_ids"))) { int id = integer(&child); if (symbols.count(id)) names[text(field(*symbols[id], "name"))].push_back(id); }
        for (const auto& [name, ids] : names) {
            if (name.empty() || ids.size() <= 1) continue;
            bool imports_only = true;
            for (const int id : ids) if (text(field(*symbols[id], "kind")) != "Import") imports_only = false;
            if (!imports_only) add_diagnostic("FLOWANALYST_DUPLICATE_NAME", "name '" + name + "' is declared more than once in scope " + std::to_string(scope_id), ids.front(), "scope:" + std::to_string(scope_id));
        }
    }
    for (const auto& [id, symbol] : symbols) for (const auto& fact : list(field(*symbol, "facts"))) if (text(field(fact, "key")) == "declared_type_spelling" || text(field(fact, "key")) == "return_type_spelling") {
        auto value = text(field(field(fact, "value"), "value")); if (value.empty()) continue; if (is_resolved_type(value)) ++resolved_types; else { ++unresolved_types; add_diagnostic("FLOWANALYST_UNKNOWN_TYPE", "declared type '" + value + "' cannot be resolved", id, "symbol:" + std::to_string(id)); }
    }
    int refined_types = 0;
    std::function<void(int, int)> check_invariant = [&](int expression_id, int refined_symbol) {
        if (!expressions.count(expression_id)) return;
        const auto* expression = expressions[expression_id];
        if (text(field(*expression, "kind")) == "identifier" && text(field(field(*expression, "payload"), "name")) != "value") add_diagnostic("FLOWANALYST_INVARIANT_NAME", "refined-type invariant name is not bound: '" + text(field(field(*expression, "payload"), "name")) + "'", refined_symbol, "symbol:" + std::to_string(refined_symbol));
        for (const auto& child : list(field(*expression, "child_expressions"))) check_invariant(integer(&child), refined_symbol);
    };
    for (const auto& [declaration_id, declaration] : declarations) if (text(field(*declaration, "kind")) == "refined_type") {
        ++refined_types; int refined_symbol = -1; for (const auto& [symbol_id, origin] : origins) { const auto path = text(field(*origin, "ast_path")); if (path == "/declaration_pool/" + std::to_string(declaration_id)) { refined_symbol = symbol_id; break; } }
        const auto base = text(field(*declaration, "base_type")); if (!is_resolved_type(base)) add_diagnostic("FLOWANALYST_REFINED_BASE_TYPE", "refined type base '" + base + "' cannot be resolved", refined_symbol, "symbol:" + std::to_string(refined_symbol));
        for (const auto& invariant : list(field(*declaration, "invariants"))) check_invariant(integer(field(invariant, "condition_expression")), refined_symbol);
    }
    for (const auto& [id, symbol] : symbols) if (text(field(*symbol, "kind")) == "Namespace") {
        int scope_id = integer(field(*symbol, "introduced_scope_id")); int mains = 0; if (scopes.count(scope_id)) for (const auto& child : list(field(*scopes[scope_id], "symbol_ids"))) { int child_id = integer(&child); if (symbols.count(child_id) && text(field(*symbols[child_id], "name")) == "main" && text(field(*symbols[child_id], "kind")) == "Procedure") ++mains; }
        targets.push_back({id, mains, text(field(*symbol, "name"))}); if (mains != 1) add_diagnostic("FLOWANALYST_TARGET_ENTRYPOINT", "target '" + targets.back().name + "' must contain exactly one main procedure", id, "target:" + targets.back().name);
    }
    std::string lowering_profile = "none";
    bool flowcat_entrypoint = false;
    if (text(field(*source_unit, "name")) == "flowcat") {
        for (const auto& [declaration_id, declaration] : declarations) {
            if (text(field(*declaration, "kind")) != "main_block") continue;
            for (const auto& parameter : list(field(*declaration, "parameters"))) {
                if (text(field(parameter, "name")) == "args" &&
                    text(field(parameter, "type")) == "list<string>") {
                    flowcat_entrypoint = true;
                }
            }
        }
    }
    bool flowcat_prints_args = false;
    for (const auto& resolution : resolutions) if (resolution.symbol >= 0 && resolution.name == "args") flowcat_prints_args = true;
    if (flowcat_entrypoint && flowcat_prints_args) {
        std::set<std::string> file_symbols;
        for (const auto& requirement : binding_requirements) file_symbols.insert(requirement.symbol);
        if (file_symbols.count("open") && file_symbols.count("read") && file_symbols.count("write") && file_symbols.count("close")) lowering_profile = "flowcat_file_main";
        else {
            lowering_profile = "flowcat_argv_main";
            binding_requirements.push_back({"flowcore.argv_output", "libc.so.6", "c", "puts", "io", "c_string", "c_int"});
        }
    }
    std::function<bool(int)> expression_is_pure = [&](int expression_id) {
        if (!expressions.count(expression_id)) return false;
        const auto* expression = expressions[expression_id];
        const auto kind = text(field(*expression, "kind"));
        if (kind == "integer_literal" || kind == "float_literal" || kind == "bool_literal" || kind == "string_literal" || kind == "identifier") return true;
        if (kind != "binary" && kind != "unary") return false;
        for (const auto& child : list(field(*expression, "child_expressions"))) if (!expression_is_pure(integer(&child))) return false;
        return true;
    };
    std::vector<EffectFact> effect_facts;
    for (const auto& [declaration_id, declaration] : declarations) {
        if (text(field(*declaration, "kind")) != "function") continue;
        EffectFact fact;
        fact.declaration = declaration_id;
        fact.name = text(field(*declaration, "name"), "<anonymous>");
        for (const auto& [symbol_id, origin] : origins) if (text(field(*origin, "ast_path")) == "/declaration_pool/" + std::to_string(declaration_id)) { fact.symbol = symbol_id; break; }
        const int body = integer(field(*declaration, "body_block"));
        const auto& body_statements = blocks.count(body) ? list(field(*blocks[body], "statements")) : list(nullptr);
        bool pure = !body_statements.empty();
        for (const auto& statement_id : body_statements) {
            if (!statements.count(integer(&statement_id)) || text(field(*statements[integer(&statement_id)], "kind")) != "return") { pure = false; break; }
            const auto* payload = field(*statements[integer(&statement_id)], "payload");
            const int value = integer(field(payload, "value_expression"));
            if (!expression_is_pure(value)) { pure = false; break; }
        }
        fact.effect = pure ? "pure" : "unknown";
        fact.certainty = pure ? "proven" : "unresolved";
        fact.reason = pure ? "return-only expression with no calls or external effects" : "body contains mutation, control state, calls, or unsupported effects";
        effect_facts.push_back(std::move(fact));
    }
    std::map<int, bool> pure_symbols;
    for (const auto& fact : effect_facts) if (fact.symbol >= 0) pure_symbols[fact.symbol] = fact.effect == "pure" && fact.certainty == "proven";
    std::function<void(int, std::set<int>&)> collect_reads = [&](int expression_id, std::set<int>& reads) {
        if (!expressions.count(expression_id)) return;
        const auto* expression = expressions[expression_id];
        if (text(field(*expression, "kind")) == "identifier" && resolved_expression_symbols.count(expression_id)) reads.insert(resolved_expression_symbols[expression_id]);
        for (const auto& child : list(field(*expression, "child_expressions"))) collect_reads(integer(&child), reads);
    };
    std::vector<CallSite> call_sites;
    for (const auto& [expression_id, expression] : expressions) {
        if (text(field(*expression, "kind")) != "call") continue;
        const auto* payload = field(*expression, "payload");
        const int base = integer(field(payload, "base"));
        const Resolution* base_resolution = nullptr;
        for (const auto& resolution : resolutions) if (resolution.expression == base) { base_resolution = &resolution; break; }
        if (!base_resolution) continue;
        CallSite site;
        site.expression = expression_id;
        site.statement = base_resolution->statement;
        site.scope = base_resolution->scope;
        site.callee_symbol = base_resolution->symbol;
        site.callee = text(field(*expression, "text"), base_resolution->name);
        site.pure = pure_symbols.count(site.callee_symbol) && pure_symbols[site.callee_symbol];
        for (const auto& argument : list(field(payload, "arguments"))) { const int argument_id = integer(&argument); site.arguments.push_back(argument_id); collect_reads(argument_id, site.reads); }
        if (statements.count(site.statement)) {
            const auto* statement = statements[site.statement];
            const auto* statement_payload = field(*statement, "payload");
            site.writes = text(field(*statement, "name"));
            if (site.writes.empty()) site.writes = text(field(field(statement_payload, "target"), "name"));
            if (!site.writes.empty() && scopes.count(site.scope)) for (const auto& symbol_id : list(field(*scopes[site.scope], "symbol_ids"))) {
                const int candidate = integer(&symbol_id);
                if (symbols.count(candidate) && text(field(*symbols[candidate], "name")) == site.writes) { site.write_symbol = candidate; break; }
            }
        }
        call_sites.push_back(std::move(site));
    }
    for (std::size_t left = 0; left < call_sites.size(); ++left) for (std::size_t right = left + 1; right < call_sites.size(); ++right) {
        auto& first = call_sites[left]; auto& second = call_sites[right];
        if (!first.pure || !second.pure || first.scope != second.scope || first.statement == second.statement) continue;
        bool shared_read = false;
        for (const auto symbol : first.reads) if (second.reads.count(symbol)) shared_read = true;
        const bool output_conflict = first.write_symbol >= 0 && first.write_symbol == second.write_symbol;
        const bool read_after_write = (first.write_symbol >= 0 && second.reads.count(first.write_symbol)) || (second.write_symbol >= 0 && first.reads.count(second.write_symbol));
        if (!shared_read && !output_conflict && !read_after_write) {
            first.independent_with.push_back(second.expression);
            second.independent_with.push_back(first.expression);
        }
    }
    std::vector<LoweringOperation> lowering_operations;
    auto containing_block = [&](int statement_id) {
        for (const auto& [block_id, block] : blocks) for (const auto& member : list(field(*block, "statements"))) if (integer(&member) == statement_id) return block_id;
        return -1;
    };
    for (const auto& site : call_sites) {
        LoweringOperation operation;
        operation.expression = site.expression;
        operation.statement = site.statement;
        operation.scope = site.scope;
        operation.block = containing_block(site.statement);
        operation.callee_symbol = site.callee_symbol;
        operation.result_symbol = site.write_symbol;
        operation.callee = site.callee;
        operation.kind = "call";
        operation.arguments = site.arguments;
        std::string leaf = site.callee;
        const auto separator = leaf.rfind('.');
        if (separator != std::string::npos) leaf = leaf.substr(separator + 1);
        for (const auto& requirement : binding_requirements) {
            if (requirement.symbol != leaf) continue;
            operation.kind = "external_call";
            operation.contract = requirement.contract;
            operation.library = requirement.library;
            operation.convention = requirement.convention;
            operation.symbol = requirement.symbol;
            operation.effect = requirement.effect;
            operation.parameter_types = requirement.parameter_types;
            operation.return_type = requirement.return_type;
            break;
        }
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "let") continue;
        const auto* payload = field(*statement, "payload");
        const int initializer = integer(field(payload, "initializer_expression"));
        if (initializer < 0) continue;
        const auto name = text(field(*statement, "name"));
        const int scope_id = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        int result_symbol = -1;
        if (scopes.count(scope_id)) for (const auto& candidate : list(field(*scopes.at(scope_id), "symbol_ids"))) {
            const int candidate_id = integer(&candidate);
            if (symbols.count(candidate_id) && text(field(*symbols.at(candidate_id), "name")) == name) { result_symbol = candidate_id; break; }
        }
        LoweringOperation operation;
        operation.expression = initializer;
        operation.statement = statement_id;
        operation.scope = scope_id;
        operation.block = containing_block(statement_id);
        operation.result_symbol = result_symbol;
        operation.kind = "value_definition";
        operation.arguments.push_back(initializer);
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "return") continue;
        const auto* payload = field(*statement, "payload");
        const int value_expression = integer(field(payload, "value_expression"));
        if (value_expression < 0) continue;
        LoweringOperation operation;
        operation.expression = value_expression;
        operation.statement = statement_id;
        operation.scope = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        operation.block = containing_block(statement_id);
        operation.kind = "return_value";
        operation.arguments.push_back(value_expression);
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "if") continue;
        const auto* payload = field(*statement, "payload");
        LoweringOperation operation;
        operation.expression = integer(field(payload, "condition_expression"));
        operation.statement = statement_id;
        operation.scope = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        operation.block = containing_block(statement_id);
        operation.then_block = integer(field(payload, "then_block"));
        operation.else_block = integer(field(field(payload, "else_arm"), "block"));
        operation.kind = "branch";
        if (operation.expression >= 0) operation.arguments.push_back(operation.expression);
        lowering_operations.push_back(std::move(operation));
    }
    std::vector<Region> regions;
    for (const auto& [id, scope] : scopes) regions.push_back({"scope:" + std::to_string(id), "scope", "sane", {}});
    for (const auto& [id, symbol] : symbols) {
        Region region{"symbol:" + std::to_string(id), "symbol", "sane", {"scope:" + std::to_string(integer(field(*symbol, "owning_scope_id")))} };
        int introduced = integer(field(*symbol, "introduced_scope_id")); if (introduced >= 0) region.prerequisites.push_back("scope:" + std::to_string(introduced));
        regions.push_back(std::move(region));
    }
    for (const auto& target : targets) {
        Region region{"target:" + target.name, "target", target.mains == 1 ? "sane" : "rejected", {}};
        const auto* symbol = symbols[target.symbol]; int scope_id = integer(field(*symbol, "introduced_scope_id"));
        if (scopes.count(scope_id)) for (const auto& child : list(field(*scopes[scope_id], "symbol_ids"))) region.prerequisites.push_back("symbol:" + std::to_string(integer(&child)));
        regions.push_back(std::move(region));
    }
    for (const auto& resolution : resolutions) if (resolution.symbol >= 0) {
        const auto region_id = "symbol:" + std::to_string(resolution.symbol);
        for (auto& region : regions) if (region.id == "scope:" + std::to_string(resolution.scope) && region_id != region.id) region.prerequisites.push_back(region_id);
    }
    for (auto& region : regions) { std::sort(region.prerequisites.begin(), region.prerequisites.end()); region.prerequisites.erase(std::unique(region.prerequisites.begin(), region.prerequisites.end()), region.prerequisites.end()); }
    for (const auto& diagnostic : diagnostics) for (auto& region : regions) if (region.id == diagnostic.region) region.status = "rejected";
    std::map<std::string, int> region_index;
    for (std::size_t index = 0; index < regions.size(); ++index) region_index[regions[index].id] = static_cast<int>(index);
    std::cout << "{\n  \"format\": \"flowanalyst.semantic_report\",\n  \"version\": 1,\n  \"status\": \"" << (diagnostics.empty() ? "ok" : "error") << "\",\n  \"source\": {\"path\": " << quote(text(field(field(bundle, "source"), "path"))) << "},\n  \"frontend_bundle\": {\"format\": \"flowmini.frontend_bundle\", \"version\": 2},\n  \"lowering_profile\": \"" << lowering_profile << "\",\n  \"diagnostics\": [";
    for (std::size_t i = 0; i < diagnostics.size(); ++i) { const auto& d = diagnostics[i]; if (i) std::cout << ','; std::cout << "{\"code\":" << quote(d.code) << ",\"severity\":" << quote(d.severity) << ",\"message\":" << quote(d.message) << ",\"root_cause\":true"; if (d.symbol >= 0) { std::cout << ",\"subject\":{\"kind\":\"symbol\",\"id\":" << d.symbol << "}"; std::cout << ",\"provenance\":{\"source\":" << quote(d.source) << ",\"ast_path\":" << quote(d.ast_path) << ",\"line\":" << d.line << ",\"column\":" << d.column << "}"; } if (!d.region.empty()) std::cout << ",\"region\":" << quote(d.region); std::cout << '}'; }
    std::cout << "],\n  \"binding_requirements\": [";
    for (std::size_t i = 0; i < binding_requirements.size(); ++i) { if (i) std::cout << ','; const auto& requirement = binding_requirements[i]; std::cout << "{\"contract\":" << quote(requirement.contract) << ",\"library\":" << quote(requirement.library) << ",\"convention\":" << quote(requirement.convention) << ",\"symbol\":" << quote(requirement.symbol) << ",\"effect\":" << quote(requirement.effect) << ",\"parameter_types\":" << quote(requirement.parameter_types) << ",\"return_type\":" << quote(requirement.return_type) << "}"; }
    std::cout << "],\n  \"aggregate_abi_layouts\": [";
    for (std::size_t i = 0; i < aggregate_layouts.size(); ++i) {
        if (i) std::cout << ',';
        const auto& layout = aggregate_layouts[i];
        std::cout << "{\"contract\":" << quote(layout.contract)
                  << ",\"name\":" << quote(layout.name)
                  << ",\"version\":1,\"status\":\"declared\",\"layout_policy\":\"provider_verified_required\",\"fields\":[";
        for (std::size_t field_index = 0; field_index < layout.fields.size(); ++field_index) {
            if (field_index) std::cout << ',';
            std::cout << "{\"name\":" << quote(layout.fields[field_index].first)
                      << ",\"type\":" << quote(layout.fields[field_index].second) << "}";
        }
        std::cout << "]}";
    }
    std::cout << "],\n  \"abi_type_contracts\": [";
    for (std::size_t i = 0; i < abi_type_contracts.size(); ++i) {
        if (i) std::cout << ',';
        const auto& type = abi_type_contracts[i];
        std::cout << "{\"contract\":" << quote(type.contract)
                  << ",\"name\":" << quote(type.name)
                  << ",\"repr\":" << quote(type.repr)
                  << ",\"ownership\":" << quote(type.ownership)
                  << ",\"access\":" << quote(type.access)
                  << ",\"lifetime\":" << quote(type.lifetime)
                  << ",\"nullable\":" << quote(type.nullable)
                  << ",\"opaque\":" << quote(type.opaque)
                  << ",\"cleanup\":" << quote(type.cleanup) << "}";
    }
    std::cout << "],\n  \"lowering_plan\": {\"format\":\"flowcore.lowering_plan\",\"version\":1,\"status\":\""
              << (diagnostics.empty() ? "ready" : "blocked") << "\",\"operations\":[";
    std::function<void(int, const std::string&)> emit_operand = [&](int expression_id, const std::string& declared_type) {
        const auto* expression = expressions.count(expression_id) ? expressions.at(expression_id) : nullptr;
        const auto kind = text(field(expression, "kind"));
        const auto literal = text(field(field(expression, "payload"), "value_text"), "0");
        const bool writable_storage = kind == "integer_literal" && declared_type == "c_pointer" &&
            !literal.empty() && literal != "0" && literal.front() != '-';
        std::cout << "{\"expression_id\":" << expression_id << ",\"kind\":" << quote(writable_storage ? "writable_storage" : kind);
        if (writable_storage) {
            std::cout << ",\"type\":\"c_pointer\",\"storage\":{\"bytes\":" << literal
                      << ",\"access\":\"read_write\",\"lifetime\":\"call\"}";
        } else
        if (kind == "integer_literal") {
            std::cout << ",\"type\":" << quote(declared_type.empty() ? "c_int" : declared_type) << ",\"value\":" << quote(literal);
        } else if (kind == "string_literal") {
            std::cout << ",\"type\":\"c_string\",\"value\":" << quote(text(field(field(expression, "payload"), "value_text")));
        } else if (kind == "bool_literal") {
            std::cout << ",\"type\":\"bool\",\"value\":" << quote(text(field(field(expression, "payload"), "value_text"), "false"));
        } else if (kind == "identifier") {
            const int symbol = resolved_expression_symbols.count(expression_id) ? resolved_expression_symbols.at(expression_id) : -1;
            const auto type = symbol_types.count(symbol) ? symbol_types.at(symbol) : std::string{};
            std::cout << ",\"type\":" << quote(type) << ",\"symbol_id\":" << symbol;
        } else if (kind == "call") {
            const auto* payload = field(expression, "payload");
            const int base = integer(field(payload, "base"));
            const auto callee = expressions.count(base) && text(field(*expressions.at(base), "kind")) == "identifier"
                ? text(field(field(*expressions.at(base), "payload"), "name")) : std::string{};
            const auto arguments = list(field(payload, "arguments"));
            if (callee == "length" && arguments.size() == 1) {
                const int argument = integer(&arguments.front());
                const int symbol = resolved_expression_symbols.count(argument) ? resolved_expression_symbols.at(argument) : -1;
                const auto type = symbol_types.count(symbol) ? symbol_types.at(symbol) : std::string{};
                if (type == "list<string>")
                    std::cout << ",\"intrinsic\":\"list_length\",\"type\":\"c_int\",\"symbol_id\":" << symbol;
                else std::cout << ",\"type\":\"unsupported\"";
            } else std::cout << ",\"type\":\"unsupported\"";
        } else if (kind == "unary") {
            const auto* payload = field(expression, "payload");
            std::cout << ",\"type\":\"c_int\",\"operator\":" << quote(text(field(payload, "operator"))) << ",\"operand\":";
            emit_operand(integer(field(payload, "operand")), {});
        } else if (kind == "binary") {
            const auto* payload = field(expression, "payload");
            const auto operator_name = text(field(payload, "operator"));
            const bool comparison = operator_name == "==" || operator_name == "!=" || operator_name == "<" || operator_name == "<=" || operator_name == ">" || operator_name == ">=";
            std::cout << ",\"type\":" << (comparison ? "\"bool\"" : "\"c_int\"") << ",\"operator\":" << quote(operator_name) << ",\"left\":";
            emit_operand(integer(field(payload, "left")), {});
            std::cout << ",\"right\":";
            emit_operand(integer(field(payload, "right")), {});
        } else {
            std::cout << ",\"type\":\"unsupported\"";
        }
        std::cout << "}";
    };
    for (std::size_t i = 0; i < lowering_operations.size(); ++i) {
        if (i) std::cout << ',';
        const auto& operation = lowering_operations[i];
        std::cout << "{\"id\":" << i
                  << ",\"kind\":" << quote(operation.kind)
                  << ",\"expression_id\":" << operation.expression
                  << ",\"statement_id\":" << operation.statement
                  << ",\"scope_id\":" << operation.scope
                  << (operation.block >= 0 ? ",\"block_id\":" + std::to_string(operation.block) : std::string{})
                  << ",\"callee\":" << quote(operation.callee)
                  << ",\"callee_symbol_id\":" << operation.callee_symbol
                  << ",\"arguments\":[";
        for (std::size_t argument = 0; argument < operation.arguments.size(); ++argument) {
            if (argument) std::cout << ',';
            std::cout << operation.arguments[argument];
        }
        std::cout << "],\"operands\":[";
        for (std::size_t argument = 0; argument < operation.arguments.size(); ++argument) {
            if (argument) std::cout << ',';
            const auto declared_type = operation.kind == "value_definition" && operation.result_symbol >= 0 && symbol_types.count(operation.result_symbol)
                ? symbol_types.at(operation.result_symbol) : std::string{};
            emit_operand(operation.arguments[argument], declared_type);
        }
        std::cout << "]";
        if (operation.result_symbol >= 0) std::cout << ",\"result_symbol_id\":" << operation.result_symbol;
        if (operation.kind == "branch") std::cout << ",\"then_block_id\":" << operation.then_block << ",\"else_block_id\":" << operation.else_block;
        if (operation.kind == "external_call") {
            std::cout << ",\"provider\":{\"contract\":" << quote(operation.contract)
                      << ",\"library\":" << quote(operation.library)
                      << ",\"convention\":" << quote(operation.convention)
                      << ",\"symbol\":" << quote(operation.symbol)
                      << ",\"effect\":" << quote(operation.effect)
                      << ",\"parameter_types\":" << quote(operation.parameter_types)
                      << ",\"return_type\":" << quote(operation.return_type) << "}";
            for (const auto& type : abi_type_contracts) {
                if (type.contract != operation.contract || type.name != operation.return_type || type.cleanup.empty()) continue;
                std::cout << ",\"result_resource\":{\"type\":" << quote(type.name)
                          << ",\"ownership\":" << quote(type.ownership)
                          << ",\"access\":" << quote(type.access)
                          << ",\"lifetime\":" << quote(type.lifetime)
                          << ",\"nullable\":" << quote(type.nullable)
                          << ",\"opaque\":" << quote(type.opaque)
                          << ",\"cleanup_capability\":" << quote(type.cleanup) << "}";
            }
        }
        std::cout << "}";
    }
    std::cout << "]},\n  \"effect_facts\": [";
    for (std::size_t i = 0; i < effect_facts.size(); ++i) { if (i) std::cout << ','; const auto& fact = effect_facts[i]; std::cout << "{\"declaration_id\":" << fact.declaration << ",\"symbol_id\":" << fact.symbol << ",\"name\":" << quote(fact.name) << ",\"effect\":" << quote(fact.effect) << ",\"certainty\":" << quote(fact.certainty) << ",\"reason\":" << quote(fact.reason) << "}"; }
    std::cout << "],\n  \"external_operations\": [";
    for (std::size_t i = 0; i < call_sites.size(); ++i) {
        if (i) std::cout << ',';
        const auto& site = call_sites[i];
        std::cout << "{\"operation\":\"call\",\"expression_id\":" << site.expression
                  << ",\"statement_id\":" << site.statement
                  << ",\"scope_id\":" << site.scope
                  << ",\"callee\":" << quote(site.callee)
                  << ",\"callee_symbol_id\":" << site.callee_symbol
                  << ",\"arguments\":[";
        for (std::size_t argument = 0; argument < site.arguments.size(); ++argument) { if (argument) std::cout << ','; std::cout << site.arguments[argument]; }
        std::cout << "]";
        if (site.write_symbol >= 0) std::cout << ",\"result_symbol_id\":" << site.write_symbol;
        std::cout << ",\"purity\":" << (site.pure ? "\"pure\"" : "\"effectful\"") << "}";
    }
    std::cout << "],\n  \"parallel_candidates\": [";
    bool first_candidate = true;
    for (const auto& site : call_sites) if (!site.independent_with.empty()) {
        if (!first_candidate) std::cout << ',';
        first_candidate = false;
        std::cout << "{\"call_expression\":" << site.expression << ",\"statement_id\":" << site.statement << ",\"callee\":" << quote(site.callee) << ",\"proof\":\"pure-callee-disjoint-inputs\",\"status\":\"deferred\",\"independent_with\":[";
        for (std::size_t i = 0; i < site.independent_with.size(); ++i) { if (i) std::cout << ','; std::cout << site.independent_with[i]; }
        std::cout << "]}";
    }
    std::cout << "],\n  \"facts\": [{\"kind\":\"semantic_summary\",\"scopes\":" << scopes.size() << ",\"symbols\":" << symbols.size() << ",\"resolved_types\":" << resolved_types << ",\"unresolved_types\":" << unresolved_types << ",\"refined_types\":" << refined_types << ",\"resolved_names\":" << resolutions.size() << ",\"targets\":" << targets.size() << ",\"regions\":" << regions.size() << "}],\n  \"resolved_names\": [";
    bool first_resolution = true; for (const auto& resolution : resolutions) if (resolution.symbol >= 0) { if (!first_resolution) std::cout << ','; first_resolution = false; std::cout << "{\"expression_id\":" << resolution.expression << ",\"statement_id\":" << resolution.statement << ",\"name\":" << quote(resolution.name) << ",\"symbol_id\":" << resolution.symbol << ",\"scope_id\":" << resolution.scope << "}"; }
    std::cout << "],\n  \"analysis_regions\": [";
    for (std::size_t i = 0; i < regions.size(); ++i) { if (i) std::cout << ','; const auto& region = regions[i]; std::cout << "{\"id\":" << quote(region.id) << ",\"kind\":" << quote(region.kind) << ",\"status\":" << quote(region.status) << ",\"requires\":["; for (std::size_t j = 0; j < region.prerequisites.size(); ++j) { if (j) std::cout << ','; std::cout << quote(region.prerequisites[j]); } std::cout << "]}"; }
    std::cout << "],\n  \"analysis_graph\": {\"format\":\"flowanalyst.analysis_graph\",\"version\":1,\"nodes\":[";
    for (std::size_t i = 0; i < regions.size(); ++i) { if (i) std::cout << ','; const auto& region = regions[i]; std::cout << "{\"index\":" << i << ",\"id\":" << quote(region.id) << ",\"kind\":" << quote(region.kind) << ",\"status\":" << quote(region.status) << '}'; }
    std::cout << "],\"edges\":[";
    bool first_edge = true;
    for (const auto& region : regions) for (const auto& prerequisite : region.prerequisites) if (region_index.count(prerequisite)) {
        if (!first_edge) std::cout << ',';
        first_edge = false;
        std::cout << "{\"from\":" << region_index[prerequisite] << ",\"to\":" << region_index[region.id] << ",\"kind\":\"requires\"}";
    }
    std::cout << "],\"matrix_views\":[{\"name\":\"region_dependency\",\"orientation\":\"prerequisite_to_dependent\",\"semiring\":\"boolean\",\"storage\":\"coo\",\"rows\":" << regions.size() << ",\"columns\":" << regions.size() << ",\"entries\":[";
    first_edge = true;
    for (const auto& region : regions) for (const auto& prerequisite : region.prerequisites) if (region_index.count(prerequisite)) {
        if (!first_edge) std::cout << ',';
        first_edge = false;
        std::cout << "{\"row\":" << region_index[prerequisite] << ",\"column\":" << region_index[region.id] << ",\"value\":true}";
    }
    std::cout << "]}]},\n  \"targets\": [";
    for (std::size_t i = 0; i < targets.size(); ++i) { if (i) std::cout << ','; std::cout << "{\"symbol_id\":" << targets[i].symbol << ",\"name\":" << quote(targets[i].name) << ",\"main_count\":" << targets[i].mains << ",\"status\":\"" << (targets[i].mains == 1 ? "sane" : "rejected") << "\"}"; }
    std::cout << "]\n}\n"; return diagnostics.empty() ? 0 : 2;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") {
                std::cout << "flowanalyst - semantic checks for FlowMini frontend bundles\n\n"
                             "Usage: flowanalyst [bundle.json]\n"
                             "       flowmini --dump-frontend-bundle source.flow | flowanalyst\n\n"
                             "Options: -h, -?, --help  show help\n"
                             "         -a, --about    show about information\n"
                             "         -v, --version  print the raw version number\n\n"
                             "More help: Flowanalyst/README.md and the Flowanalyst consumer contract.\n";
                return 0;
            }
            if (option == "-a" || option == "--about") {
                std::cout << "Flowanalyst independently checks the semantic sanity of FlowMini frontend bundles.\n"
                             "More help: Flowanalyst/README.md and the Flowanalyst consumer contract.\n";
                return 0;
            }
            if (option == "-v" || option == "--version") { std::cout << FLOWANALYST_VERSION << '\n'; return 0; }
        }
        std::ostringstream input; if (argc > 2) { std::cerr << "usage: flowanalyst [bundle.json]\n"; return 1; } if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open bundle"); input << file.rdbuf(); } else input << std::cin.rdbuf(); return run(Parser(input.str()).parse());
    }
    catch (const std::exception& error) { std::cerr << "flowanalyst error: " << error.what() << '\n'; return 1; }
}
