#include <flowcontracts/json.hpp>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
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

using Json = flowcontracts::json::Value;
using Object = flowcontracts::json::Object;
using Array = flowcontracts::json::Array;

class Parser {
public:
    explicit Parser(std::string text) : text_(std::move(text)) {}
    Json parse() const { return flowcontracts::json::parse(text_); }
private:
    std::string text_;
};

const Json* field(const Json& value, std::string_view name) { if (auto object = std::get_if<Object>(&value)) { auto it = object->find(std::string(name)); return it == object->end() ? nullptr : &it->second; } return nullptr; }
const Json* field(const Json* value, std::string_view name) { return value ? field(*value, name) : nullptr; }
std::string text(const Json* value, std::string fallback = {}) { if (value) if (auto v = std::get_if<std::string>(value)) return *v; return fallback; }
int integer(const Json* value, int fallback = -1) {
    if (!value || std::holds_alternative<std::nullptr_t>(*value)) return fallback;
    const auto parsed = flowcontracts::json::integer(*value, "$ integer field");
    if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) throw std::runtime_error("JSON integer is outside int range");
    return static_cast<int>(parsed);
}
const Array& list(const Json* value) { static const Array empty; return value && std::holds_alternative<Array>(*value) ? std::get<Array>(*value) : empty; }
std::string quote(std::string_view value) {
    std::ostringstream out;
    out << '"';
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) out << "\\u00" << hex[c >> 4] << hex[c & 0x0f];
                else out << static_cast<char>(c);
                break;
        }
    }
    return out.str() + '"';
}
struct Diagnostic { std::string code, severity, message, ast_path, region, source; int symbol = -1, line = -1, column = -1; };
struct Target { int symbol = -1, mains = 0; std::string name; };
struct BindingRequirement { int source_symbol = -1; std::string contract, library, convention, symbol, effect, parameter_types, return_type; };
struct AbiTypeContract { std::string contract, name, repr, ownership, access, lifetime, nullable, opaque, cleanup; };
struct AggregateLayout { std::string contract, name; std::vector<std::pair<std::string, std::string>> fields; };
struct Region { std::string id, kind, status; std::vector<std::string> prerequisites; };
struct EffectFact { int declaration = -1, symbol = -1; std::string name, effect, certainty, reason; };
struct VariantPayloadBinding { int symbol = -1; std::string name, type; };
struct GenericSignature { int symbol = -1, declaration = -1; std::string name, return_type; std::vector<std::string> parameters, value_parameters, parameter_types; bool forward_first_argument = false; };
struct ResourceUse { int argument = -1, symbol = -1; std::string type, identity, alias_status, access, ownership, lifetime, opaque; };
struct CallSite { int expression = -1, statement = -1, scope = -1, callee_symbol = -1, write_symbol = -1; std::string callee, effect = "unknown", provider_contract, provider_symbol; bool pure = false, external = false, produces_resource = false; std::set<int> reads; std::string writes; std::vector<int> arguments; std::vector<ResourceUse> resources; ResourceUse produced_resource; std::vector<int> independent_with; };
struct ParallelRejection { int left = -1, right = -1; std::string reason, left_effect, right_effect; std::vector<ResourceUse> left_resources, right_resources; };
struct LoweringOperation { int expression = -1, statement = -1, scope = -1, block = -1, function_symbol = -1, then_block = -1, else_block = -1, body_block = -1, failure_block = -1, default_block = -1, join_block = -1, callee_symbol = -1, result_symbol = -1, variant_discriminant = -1; std::string callee, kind, contract, library, convention, symbol, effect, effect_class, parameter_types, return_type, selector_type, selector_kind, compile_time_value, variant_type, variant_member, variant_generic_owner, variant_instance_id, generic_owner, generic_return_type, instantiation_id; std::vector<int> arguments, match_values, match_highs, match_blocks; std::vector<std::string> match_label_types, match_label_members, variant_payload_types, variant_type_arguments, generic_type_arguments; std::vector<std::pair<std::string, std::string>> variant_substitutions, generic_substitutions; std::vector<ResourceUse> resources; ResourceUse produced_resource; bool produces_resource = false; std::vector<std::vector<VariantPayloadBinding>> match_payload_bindings; };
struct Callable { int symbol = -1, scope = -1, body_block = -1; bool entry = false; std::string name, return_type, availability; std::vector<std::pair<int, std::string>> parameters; };
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

int run(const Json& bundle, int lowering_plan_version) {
    if (text(field(bundle, "format")) != "flowmini.frontend_bundle" || integer(field(bundle, "version")) != 2) throw std::runtime_error("unsupported FlowMini frontend bundle");
    const auto* snapshot = field(bundle, "symbol_table"); if (!snapshot) throw std::runtime_error("bundle has no symbol_table");
    std::map<int, const Json*> symbols, scopes, origins;
    auto insert_identity = [](auto& index, int identity, const Json& entry, std::string_view kind) {
        if (identity < 0) throw std::runtime_error(std::string(kind) + " identity must be non-negative");
        if (!index.emplace(identity, &entry).second) throw std::runtime_error("duplicate " + std::string(kind) + " identity " + std::to_string(identity));
    };
    for (const auto& entry : list(field(snapshot, "symbols"))) insert_identity(symbols, integer(field(entry, "id")), entry, "symbol");
    for (const auto& entry : list(field(snapshot, "scopes"))) insert_identity(scopes, integer(field(entry, "id")), entry, "scope");
    for (const auto& entry : list(field(bundle, "symbol_origins"))) insert_identity(origins, integer(field(entry, "symbol_id")), entry, "symbol origin");
    const auto* ast = field(bundle, "ast");
    std::map<int, const Json*> expressions, statements, blocks, declarations;
    if (ast) {
        for (const auto& entry : list(field(ast, "expression_pool"))) insert_identity(expressions, integer(field(entry, "id")), entry, "expression");
        for (const auto& entry : list(field(ast, "statement_pool"))) insert_identity(statements, integer(field(entry, "id")), entry, "statement");
        for (const auto& entry : list(field(ast, "block_pool"))) insert_identity(blocks, integer(field(entry, "id")), entry, "block");
        for (const auto& entry : list(field(ast, "declaration_pool"))) insert_identity(declarations, integer(field(entry, "id")), entry, "declaration");
    }
    std::set<std::string> enum_types, variant_types;
    std::map<std::string, std::map<std::string, int>> enum_members;
    std::map<std::string, std::map<std::string, int>> variant_members;
    std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>> variant_payloads;
    std::map<std::string, std::vector<std::string>> generic_variant_parameters;
    std::map<std::string, std::size_t> generic_variant_arities;
    std::map<std::string, std::size_t> generic_record_arities;
    std::set<std::string> generic_parameter_names;
    std::vector<std::pair<int, std::string>> duplicate_generic_parameters;
    for (const auto& [identity, declaration] : declarations) {
        const auto kind = text(field(*declaration, "kind"));
        if (kind == "enum") {
            const auto type = text(field(*declaration, "name"));
            enum_types.insert(type);
            int tag = 0;
            for (const auto& member : list(field(*declaration, "members"))) enum_members[type][text(field(member, "name"))] = tag++;
        }
        if (kind == "variant") {
            const auto type = text(field(*declaration, "name"));
            variant_types.insert(type);
            const auto parameters = list(field(*declaration, "type_parameters"));
            if (!parameters.empty()) {
                generic_variant_arities[type] = parameters.size();
                std::set<std::string> seen;
                for (const auto& parameter : parameters) {
                    const auto name = text(field(parameter, "name"));
                    if (!seen.insert(name).second) duplicate_generic_parameters.emplace_back(identity, name);
                    generic_variant_parameters[type].push_back(name);
                    generic_parameter_names.insert(name);
                }
            }
            int tag = 0;
            for (const auto& member : list(field(*declaration, "members"))) {
                const auto member_name = text(field(member, "name"));
                variant_members[type][member_name] = tag++;
                for (const auto& payload : list(field(member, "fields"))) {
                    variant_payloads[type][member_name].emplace_back(
                        text(field(payload, "name")), text(field(payload, "type")));
                }
            }
        }
        if (kind == "record") {
            const auto parameters = list(field(*declaration, "type_parameters"));
            if (!parameters.empty()) {
                generic_record_arities[text(field(*declaration, "name"))] = parameters.size();
                std::set<std::string> seen;
                for (const auto& parameter : parameters) {
                    const auto name = text(field(parameter, "name"));
                    if (!seen.insert(name).second) duplicate_generic_parameters.emplace_back(identity, name);
                    generic_parameter_names.insert(name);
                }
            }
        }
    }
    std::map<int, GenericSignature> generic_functions;
    for (const auto& [declaration_id, declaration] : declarations) {
        if (text(field(*declaration, "kind")) != "function") continue;
        const auto parameters = list(field(*declaration, "type_parameters"));
        if (parameters.empty()) continue;
        int function_symbol = -1;
        for (const auto& [symbol_id, origin] : origins) {
            if (text(field(*origin, "ast_path")) == "/declaration_pool/" + std::to_string(declaration_id)) {
                function_symbol = symbol_id;
                break;
            }
        }
        if (function_symbol < 0) continue;
        GenericSignature signature;
        signature.symbol = function_symbol;
        signature.declaration = declaration_id;
        signature.name = text(field(*declaration, "name"));
        signature.return_type = text(field(*declaration, "return_type"));
        for (const auto& parameter : parameters) {
            const auto name = text(field(parameter, "name"));
            signature.parameters.push_back(name);
            if (std::count(signature.parameters.begin(), signature.parameters.end() - 1, name) != 0)
                duplicate_generic_parameters.emplace_back(function_symbol, name);
            generic_parameter_names.insert(name);
        }
        for (const auto& parameter : list(field(*declaration, "parameters"))) {
            signature.value_parameters.push_back(text(field(parameter, "name")));
            signature.parameter_types.push_back(text(field(parameter, "type")));
        }
        const int body = integer(field(*declaration, "body_block"));
        const auto body_statements = blocks.count(body) ? list(field(*blocks.at(body), "statements")) : list(nullptr);
        if (signature.parameters.size() == 1 && signature.parameter_types.size() == 1 && signature.return_type == signature.parameter_types.front() && body_statements.size() == 1 && statements.count(integer(&body_statements.front()))) {
            const auto* statement = statements.at(integer(&body_statements.front()));
            if (text(field(*statement, "kind")) == "return" || text(field(*statement, "kind")) == "placement") {
                const auto* payload = field(*statement, "payload");
                const int value = integer(field(payload, "value_expression"));
                const auto* expression = expressions.count(value) ? expressions.at(value) : nullptr;
                signature.forward_first_argument = expression != nullptr &&
                    text(field(*statement, "kind")) == "return" &&
                    text(field(*expression, "kind")) == "identifier" &&
                    !signature.value_parameters.empty() &&
                    text(field(field(*expression, "payload"), "name")) == signature.value_parameters.front();
            }
        }
        generic_functions[function_symbol] = std::move(signature);
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
    for (const auto& [symbol, name] : duplicate_generic_parameters)
        add_diagnostic("FLOWANALYST_GENERIC_DUPLICATE_PARAMETER", "generic parameter '" + name + "' is declared more than once", symbol, "generic:" + name);
    std::vector<Target> targets;
    std::vector<BindingRequirement> binding_requirements;
    std::vector<AbiTypeContract> abi_type_contracts;
    std::vector<AggregateLayout> aggregate_layouts;
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
    auto symbol_is_const = [&](int symbol_id) {
        return symbol_id >= 0 && symbols.count(symbol_id) && fact_value(*symbols.at(symbol_id), "mutability") == "const";
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
            if (!called_names.count(function_name)) continue;
            std::string parameter_types;
            const int function_scope = integer(field(*symbols[child_id], "introduced_scope_id"));
            if (scopes.count(function_scope)) for (const auto& parameter : list(field(*scopes[function_scope], "symbol_ids"))) {
                const int parameter_id = integer(&parameter); if (!symbols.count(parameter_id) || text(field(*symbols[parameter_id], "kind")) != "Parameter") continue;
                if (!parameter_types.empty()) parameter_types += ',';
                parameter_types += fact_value(*symbols[parameter_id], "declared_type_spelling");
            }
            binding_requirements.push_back({child_id, text(field(*contract, "name")), library, convention, external, fact_value(*symbols[child_id], "effect_spelling"), parameter_types, fact_value(*symbols[child_id], "return_type_spelling")});
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
    const std::vector<std::string> intrinsic_types = {"stdin.text", "stdin.bytes", "start.record"};
    auto is_intrinsic_type = [&](const std::string& value) { for (const auto& item : intrinsic_types) if (item == value) return true; return false; };
    const std::vector<std::string> intrinsic_roots = {"stdin", "start"};
    const std::vector<std::string> intrinsic_functions = {"length"};
    std::map<std::string, int> type_symbols;
    for (const auto& [id, symbol] : symbols) { auto kind = text(field(*symbol, "kind")); if (kind == "Type" || kind == "Struct" || kind == "Contract") type_symbols[text(field(*symbol, "name"))] = id; }
    const std::vector<std::string> generic_constructors = {"list", "array", "optional", "collection.list", "result.Result"};
    std::function<bool(const std::string&)> is_resolved_type = [&](const std::string& raw_value) {
        const auto value = trim_copy(raw_value); if (is_builtin(value) || is_abi_type(value) || is_intrinsic_type(value) || type_symbols.count(value) != 0 || enum_types.count(value) != 0 || variant_types.count(value) != 0 || generic_parameter_names.count(value) != 0) return true;
        std::string core = value; const auto shape = value.find("["); if (shape != std::string::npos) { if (!numeric_extents(value.substr(shape)) || shape == 0) return false; core = value.substr(0, shape); }
        const auto open = core.find('<'); if (open == std::string::npos || core.back() != '>') return false;
        const auto constructor = core.substr(0, open); bool known = false; for (const auto& candidate : generic_constructors) if (constructor == candidate) known = true;
        const auto arguments = split_generic_arguments(core.substr(open + 1, core.size() - open - 2));
        if (generic_record_arities.count(constructor) != 0) known = arguments.size() == generic_record_arities.at(constructor);
        if (generic_variant_arities.count(constructor) != 0) known = arguments.size() == generic_variant_arities.at(constructor);
        if (!known) return false;
        if (arguments.empty()) return false;
        for (const auto& argument : arguments) if (!is_resolved_type(argument)) return false;
        return true;
    };
    auto split_type_instance = [&](const std::string& raw) {
        std::pair<std::string, std::vector<std::string>> result{trim_copy(raw), {}};
        const auto open = raw.find('<');
        if (open == std::string::npos || raw.back() != '>') return result;
        result.first = trim_copy(raw.substr(0, open));
        result.second = split_generic_arguments(raw.substr(open + 1, raw.size() - open - 2));
        return result;
    };
    auto variant_substitution = [&](const std::string& concrete_type, const std::string& fallback_type) {
        const auto instance = split_type_instance(concrete_type.empty() ? fallback_type : concrete_type);
        std::map<std::string, std::string> substitutions;
        const auto parameters = generic_variant_parameters.find(instance.first);
        if (parameters != generic_variant_parameters.end() && parameters->second.size() == instance.second.size())
            for (std::size_t index = 0; index < instance.second.size(); ++index)
                substitutions[parameters->second[index]] = instance.second[index];
        return std::make_pair(instance.first, substitutions);
    };
    auto variant_instance_id = [&](const std::string& owner, const std::vector<std::string>& arguments) {
        std::string result = owner + "<";
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index) result += ",";
            result += trim_copy(arguments[index]);
        }
        return result + ">";
    };
    for (const auto& [declaration_id, declaration] : declarations) {
        if (text(field(*declaration, "kind")) != "variant") continue;
        std::set<std::string> owned_parameters;
        for (const auto& parameter : list(field(*declaration, "type_parameters")))
            owned_parameters.insert(text(field(parameter, "name")));
        for (const auto& member : list(field(*declaration, "members"))) {
            for (const auto& payload : list(field(member, "fields"))) {
                const auto payload_type = text(field(payload, "type"));
                if (!owned_parameters.count(payload_type) && !is_resolved_type(payload_type))
                    add_diagnostic("FLOWANALYST_GENERIC_VARIANT_UNKNOWN_TYPE",
                                   "generic variant payload type '" + payload_type + "' cannot be resolved",
                                   -1, "declaration:" + std::to_string(declaration_id));
            }
        }
    }
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
        for (const auto& key : {"body_block", "then_block", "failure_block"}) { int block = integer(field(payload, key)); if (block >= 0) result.push_back(block); }
        const auto* else_arm = field(payload, "else_arm"); int else_block = integer(field(else_arm, "block")); if (else_block >= 0) result.push_back(else_block);
        for (const auto& arm : list(field(payload, "cases"))) {
            const int block = integer(field(arm, "block")); if (block >= 0) result.push_back(block);
        }
        const int default_block = integer(field(payload, "default_block")); if (default_block >= 0) result.push_back(default_block);
        return result;
    };
    std::function<void(int, int)> assign_statements = [&](int block_id, int owner_scope) {
        if (!blocks.count(block_id)) return;
        int scope_id = block_scopes.count(block_id) ? block_scopes[block_id] : owner_scope;
        for (const auto& statement : list(field(*blocks[block_id], "statements"))) {
            int statement_id = integer(&statement); statement_scopes[statement_id] = scope_id;
            for (int child : nested_block(*statements[statement_id])) assign_statements(child, owner_scope);
            const int else_if = integer(field(field(field(*statements[statement_id], "payload"), "else_arm"), "if_statement"));
            if (else_if >= 0 && statements.count(else_if)) {
                statement_scopes[else_if] = scope_id;
                for (int child : nested_block(*statements[else_if])) assign_statements(child, owner_scope);
            }
        }
    };
    for (const auto& [declaration_id, declaration] : declarations) { int scope_id = declaration_scopes.count(declaration_id) ? declaration_scopes[declaration_id] : -1; int body = integer(field(*declaration, "body_block")); if (scope_id >= 0 && body >= 0) assign_statements(body, scope_id); }
    std::vector<Callable> callables;
    for (const auto& [declaration_id, declaration] : declarations) {
        const auto kind = text(field(*declaration, "kind"));
        if (kind != "function" && kind != "main_block") continue;
        const int scope_id = declaration_scopes.count(declaration_id) ? declaration_scopes[declaration_id] : -1;
        if (!scopes.count(scope_id)) continue;
        const int symbol_id = integer(field(*scopes.at(scope_id), "owner_symbol_id"));
        if (!symbols.count(symbol_id)) continue;
        Callable callable{symbol_id, scope_id, integer(field(*declaration, "body_block")), kind == "main_block",
                          kind == "main_block" ? "main" : text(field(*declaration, "name")),
                          kind == "main_block" ? "c_int" : fact_value(*symbols.at(symbol_id), "return_type_spelling"), "definition", {}};
        for (const auto& child : list(field(*scopes.at(scope_id), "symbol_ids"))) {
            const int parameter = integer(&child);
            if (symbols.count(parameter) && text(field(*symbols.at(parameter), "kind")) == "Parameter")
                callable.parameters.emplace_back(parameter, fact_value(*symbols.at(parameter), "declared_type_spelling"));
        }
        callables.push_back(std::move(callable));
    }
    std::set<int> catalogued_callables;
    for (const auto& callable : callables) catalogued_callables.insert(callable.symbol);
    for (const auto& [symbol_id, symbol] : symbols) {
        if (text(field(*symbol, "kind")) != "Function" || catalogued_callables.count(symbol_id)) continue;
        const int scope_id = integer(field(*symbol, "introduced_scope_id"));
        Callable callable{symbol_id, scope_id, -1, false, text(field(*symbol, "name")),
                          fact_value(*symbol, "return_type_spelling"), "declaration", {}};
        if (scopes.count(scope_id)) for (const auto& child : list(field(*scopes.at(scope_id), "symbol_ids"))) {
            const int parameter = integer(&child);
            if (symbols.count(parameter) && text(field(*symbols.at(parameter), "kind")) == "Parameter")
                callable.parameters.emplace_back(parameter, fact_value(*symbols.at(parameter), "declared_type_spelling"));
        }
        callables.push_back(std::move(callable));
    }
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
    auto generic_argument_texts = [&](const Json& expression) {
        std::vector<std::string> result;
        const auto* payload = field(expression, "payload");
        for (const auto& argument : list(field(payload, "type_arguments")))
            result.push_back(text(field(argument, "text")));
        return result;
    };
    auto substitute_type = [](const std::string& type, const std::map<std::string, std::string>& substitutions) {
        const auto found = substitutions.find(type);
        return found == substitutions.end() ? type : found->second;
    };
    std::function<std::string(int)> expression_type = [&](int expression_id) -> std::string {
        if (!expressions.count(expression_id)) return {};
        const auto* expression = expressions.at(expression_id);
        const auto kind = text(field(*expression, "kind"));
        if (kind == "integer_literal") return "int";
        if (kind == "bool_literal") return "Bool";
        if (kind == "string_literal") return "string";
        if (kind == "identifier") {
            const auto found = resolved_expression_symbols.find(expression_id);
            return found != resolved_expression_symbols.end() && symbol_types.count(found->second) ? symbol_types.at(found->second) : std::string{};
        }
        if (kind == "field_access") {
            const auto* payload = field(*expression, "payload");
            const int base = integer(field(payload, "base"));
            const auto found = resolved_expression_symbols.find(base);
            const auto base_type = found != resolved_expression_symbols.end() && symbol_types.count(found->second) ? symbol_types.at(found->second) : std::string{};
            const auto member = text(field(payload, "field"));
            if (variant_payloads.count(base_type)) {
                for (const auto& [member_name, fields] : variant_payloads.at(base_type)) {
                    if (member_name != member) for (const auto& [field_name, field_type] : fields) if (field_name == member) return field_type;
                }
            }
            const auto enum_found = enum_members.find(base_type);
            if (enum_found != enum_members.end() && enum_found->second.count(member)) return base_type;
            return {};
        }
        if (kind == "call") {
            const int base = integer(field(field(*expression, "payload"), "base"));
            if (expressions.count(base) && text(field(*expressions.at(base), "kind")) == "field_access") {
                const auto* base_payload = field(*expressions.at(base), "payload");
                const int type_expression = integer(field(base_payload, "base"));
                const auto type_name = expressions.count(type_expression) && text(field(*expressions.at(type_expression), "kind")) == "identifier"
                    ? text(field(field(*expressions.at(type_expression), "payload"), "name")) : std::string{};
                const auto member = text(field(base_payload, "field"));
                if (variant_members.count(type_name) && variant_members.at(type_name).count(member)) return type_name;
            }
            const auto found = resolved_expression_symbols.find(base);
            if (found == resolved_expression_symbols.end()) return {};
            const auto generic = generic_functions.find(found->second);
            if (generic == generic_functions.end())
                return symbol_types.count(found->second) ? symbol_types.at(found->second) : std::string{};
            const auto* call_payload = field(*expression, "payload");
            auto arguments = generic_argument_texts(*expression);
            const auto call_arguments = list(field(call_payload, "arguments"));
            if (arguments.empty()) {
                for (std::size_t index = 0; index < call_arguments.size() && index < generic->second.parameters.size(); ++index)
                    arguments.push_back(expression_type(integer(&call_arguments[index])));
            }
            if (arguments.size() != generic->second.parameters.size()) {
                add_diagnostic("FLOWANALYST_GENERIC_ARITY",
                               "generic function '" + generic->second.name + "' expects " + std::to_string(generic->second.parameters.size()) + " type argument(s), got " + std::to_string(arguments.size()),
                               found->second, "expression:" + std::to_string(expression_id));
                return {};
            }
            std::map<std::string, std::string> substitutions;
            for (std::size_t index = 0; index < arguments.size(); ++index) {
                if (!is_resolved_type(arguments[index])) {
                    add_diagnostic("FLOWANALYST_GENERIC_UNKNOWN_TYPE", "generic type argument '" + arguments[index] + "' cannot be resolved", found->second, "expression:" + std::to_string(expression_id));
                    continue;
                }
                substitutions[generic->second.parameters[index]] = arguments[index];
            }
            for (std::size_t index = 0; index < call_arguments.size() && index < generic->second.parameter_types.size(); ++index) {
                const auto actual = expression_type(integer(&call_arguments[index]));
                const auto expected = substitute_type(generic->second.parameter_types[index], substitutions);
                if (!actual.empty() && !expected.empty() && actual != expected)
                    add_diagnostic("FLOWANALYST_GENERIC_TYPE_MISMATCH", "generic call argument does not match substituted parameter type", found->second, "expression:" + std::to_string(expression_id));
            }
            return substitute_type(generic->second.return_type, substitutions);
        }
        if (kind == "binary") {
            const auto op = text(field(field(*expression, "payload"), "operator"));
            return (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") ? "Bool" : "int";
        }
        if (kind == "unary") return expression_type(integer(field(field(*expression, "payload"), "operand")));
        return {};
    };
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
    auto visible_symbol = [&](int scope_id, const std::string& name) {
        int current = scope_id;
        while (current >= 0 && scopes.count(current)) {
            for (const auto& symbol_id : list(field(*scopes.at(current), "symbol_ids"))) {
                const int candidate = integer(&symbol_id);
                if (symbols.count(candidate) && text(field(*symbols.at(candidate), "name")) == name) return candidate;
            }
            current = integer(field(*scopes.at(current), "parent_id"));
        }
        return -1;
    };
    auto binding_for_call = [&](const CallSite& site) -> const BindingRequirement* {
        std::string leaf = site.callee;
        const auto separator = leaf.rfind('.');
        if (separator != std::string::npos) leaf = leaf.substr(separator + 1);
        const bool qualified = site.callee.find('.') != std::string::npos;
        for (const auto& requirement : binding_requirements) {
            if (qualified ? requirement.source_symbol == site.callee_symbol : requirement.symbol == leaf) return &requirement;
        }
        return nullptr;
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
        if (!site.writes.empty()) site.write_symbol = visible_symbol(site.scope, site.writes);
        }
        if (const auto* requirement = binding_for_call(site)) {
            site.external = true;
            site.provider_contract = requirement->contract;
            site.provider_symbol = requirement->symbol;
            site.effect = requirement->effect.empty() ? "unknown" : requirement->effect;
            const auto parameter_types = split_generic_arguments(requirement->parameter_types);
            for (std::size_t index = 0; index < parameter_types.size() && index < site.arguments.size(); ++index) {
                const auto type_name = parameter_types[index];
                const AbiTypeContract* contract = nullptr;
                for (const auto& candidate : abi_type_contracts)
                    if (candidate.contract == requirement->contract && candidate.name == type_name) { contract = &candidate; break; }
                if (!contract) continue;
                const int argument_expression = site.arguments[index];
                const int argument_symbol = resolved_expression_symbols.count(argument_expression) ? resolved_expression_symbols.at(argument_expression) : -1;
                const bool resource_carrier = contract->opaque == "true" || contract->ownership == "external" || contract->access == "read_write" || contract->access == "write" || contract->lifetime == "external";
                if (!resource_carrier) continue;
                ResourceUse use;
                use.argument = static_cast<int>(index);
                use.symbol = argument_symbol;
                use.type = type_name;
                use.identity = argument_symbol >= 0 ? "symbol:" + std::to_string(argument_symbol) : "unknown";
                use.alias_status = argument_symbol >= 0 ? "symbol-derived" : "unknown";
                use.access = contract->access;
                use.ownership = contract->ownership;
                use.lifetime = contract->lifetime;
                use.opaque = contract->opaque;
                site.resources.push_back(std::move(use));
            }
            for (const auto& candidate : abi_type_contracts) {
                if (candidate.contract != requirement->contract || candidate.name != requirement->return_type || candidate.cleanup.empty()) continue;
                site.produces_resource = true;
                site.produced_resource = {-1, site.write_symbol, candidate.name,
                                          site.write_symbol >= 0 ? "symbol:" + std::to_string(site.write_symbol) : "unknown",
                                          site.write_symbol >= 0 ? "symbol-derived" : "unknown", candidate.access,
                                          candidate.ownership, candidate.lifetime, candidate.opaque};
                break;
            }
            site.pure = site.effect == "pure" && site.resources.empty();
        } else {
            site.effect = site.pure ? "pure" : "unknown";
        }
        call_sites.push_back(std::move(site));
    }
    std::vector<ParallelRejection> parallel_rejections;
    for (std::size_t left = 0; left < call_sites.size(); ++left) for (std::size_t right = left + 1; right < call_sites.size(); ++right) {
        auto& first = call_sites[left]; auto& second = call_sites[right];
        auto reject = [&](std::string reason) { parallel_rejections.push_back({first.expression, second.expression, std::move(reason), first.effect, second.effect, first.resources, second.resources}); };
        if (!first.pure || !second.pure) {
            const bool resource_pair = !first.resources.empty() || !second.resources.empty();
            if (resource_pair) {
                const bool same_resource = std::any_of(first.resources.begin(), first.resources.end(), [&](const auto& lhs) {
                    return std::any_of(second.resources.begin(), second.resources.end(), [&](const auto& rhs) {
                        return lhs.identity != "unknown" && lhs.identity == rhs.identity;
                    });
                });
                const bool unknown_alias = std::any_of(first.resources.begin(), first.resources.end(), [](const auto& use) { return use.identity == "unknown"; }) ||
                    std::any_of(second.resources.begin(), second.resources.end(), [](const auto& use) { return use.identity == "unknown"; }) ||
                    (!first.resources.empty() && !second.resources.empty() && !same_resource);
                const bool write = std::any_of(first.resources.begin(), first.resources.end(), [](const auto& use) { return use.access == "write" || use.access == "read_write"; }) ||
                    std::any_of(second.resources.begin(), second.resources.end(), [](const auto& use) { return use.access == "write" || use.access == "read_write"; });
                if (write && unknown_alias) reject("resource-alias-unknown");
                else if (write && same_resource) {
                    const bool both_write = std::all_of(first.resources.begin(), first.resources.end(), [](const auto& use) { return use.access == "write" || use.access == "read_write"; }) &&
                        std::all_of(second.resources.begin(), second.resources.end(), [](const auto& use) { return use.access == "write" || use.access == "read_write"; });
                    reject(both_write ? "resource-write-write-conflict" : "resource-read-write-conflict");
                } else reject("provider-concurrency-unknown");
            } else if (first.effect == "unknown" || second.effect == "unknown") reject("unknown-effect");
            else reject("external-effect");
            continue;
        }
        if (first.scope != second.scope) { reject("different-scope"); continue; }
        if (first.statement == second.statement) { reject("same-statement"); continue; }
        bool shared_read = false;
        for (const auto symbol : first.reads) if (second.reads.count(symbol)) shared_read = true;
        const bool output_conflict = first.write_symbol >= 0 && first.write_symbol == second.write_symbol;
        const bool read_after_write = (first.write_symbol >= 0 && second.reads.count(first.write_symbol)) || (second.write_symbol >= 0 && first.reads.count(second.write_symbol));
        if (!shared_read && !output_conflict && !read_after_write) {
            first.independent_with.push_back(second.expression);
            second.independent_with.push_back(first.expression);
        } else if (output_conflict) reject("conflicting-output");
        else if (read_after_write) reject("read-after-write-dependency");
        else reject("shared-input-disjointness-proof-missing");
    }
    std::vector<LoweringOperation> lowering_operations;
    auto containing_function = [&](int scope_id) {
        int current = scope_id;
        while (current >= 0 && scopes.count(current)) {
            const int owner = integer(field(*scopes.at(current), "owner_symbol_id"));
            if (symbols.count(owner)) {
                const auto kind = text(field(*symbols.at(owner), "kind"));
                if (kind == "Function" || kind == "Procedure") return owner;
            }
            current = integer(field(*scopes.at(current), "parent_id"));
        }
        return -1;
    };
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
        operation.function_symbol = containing_function(site.scope);
        operation.callee_symbol = site.callee_symbol;
        operation.result_symbol = site.write_symbol;
        operation.callee = site.callee;
        operation.effect_class = site.effect;
        operation.resources = site.resources;
        operation.produces_resource = site.produces_resource;
        operation.produced_resource = site.produced_resource;
        operation.kind = "call";
        operation.arguments = site.arguments;
        std::string leaf = site.callee;
        const auto separator = leaf.rfind('.');
        if (separator != std::string::npos) leaf = leaf.substr(separator + 1);
        for (const auto& requirement : binding_requirements) {
            /* Qualified ABI calls resolve through declaration identity, so a
             * semantic name can differ from its provider export. Preserve the
             * established unqualified-call boundary until those calls carry
             * an explicit contract qualification of their own. */
            const bool qualified = site.callee.find('.') != std::string::npos;
            if (qualified ? requirement.source_symbol != site.callee_symbol : requirement.symbol != leaf) continue;
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
        const auto generic = generic_functions.find(operation.callee_symbol);
        if (generic != generic_functions.end() && operation.kind == "call") {
            const auto analyzed_return_type = expression_type(site.expression);
            if (!generic->second.forward_first_argument) {
                add_diagnostic("FLOWANALYST_GENERIC_BODY_UNSUPPORTED",
                               "generic function body is outside the initial supported forwarding subset",
                               operation.callee_symbol, "expression:" + std::to_string(site.expression));
            }
            operation.kind = "generic_call";
            operation.generic_owner = generic->second.name;
            auto type_arguments = generic_argument_texts(*expressions.at(site.expression));
            if (type_arguments.empty()) {
                for (const auto argument : site.arguments) type_arguments.push_back(expression_type(argument));
            }
            if (type_arguments.size() == generic->second.parameters.size()) {
                for (std::size_t index = 0; index < type_arguments.size(); ++index)
                operation.generic_substitutions.emplace_back(generic->second.parameters[index], type_arguments[index]);
                operation.generic_type_arguments = type_arguments;
                std::ostringstream identity;
                identity << generic->second.name << "<";
                for (std::size_t index = 0; index < type_arguments.size(); ++index) {
                    if (index) identity << ",";
                    identity << type_arguments[index];
                }
                identity << ">";
                operation.instantiation_id = identity.str();
                operation.generic_return_type = analyzed_return_type.empty()
                    ? substitute_type(generic->second.return_type,
                                      std::map<std::string, std::string>(operation.generic_substitutions.begin(), operation.generic_substitutions.end()))
                    : analyzed_return_type;
            }
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
        operation.function_symbol = containing_function(scope_id);
        operation.result_symbol = result_symbol;
        operation.kind = "value_definition";
        const auto* is_const = field(payload, "is_const");
        if (is_const && std::holds_alternative<bool>(*is_const) && std::get<bool>(*is_const)) {
            operation.compile_time_value = text(field(payload, "compile_time_value"));
            if (operation.compile_time_value.empty()) {
                add_diagnostic("FLOWANALYST_CONST_NOT_COMPILE_TIME",
                               "const initializer is not a deterministic compile-time value",
                               result_symbol,
                               "statement:" + std::to_string(statement_id));
            }
        }
        operation.arguments.push_back(initializer);
        if (expressions.count(initializer) && text(field(*expressions.at(initializer), "kind")) == "call") {
            const auto* call_payload = field(*expressions.at(initializer), "payload");
            const int base = integer(field(call_payload, "base"));
            if (expressions.count(base) && text(field(*expressions.at(base), "kind")) == "field_access") {
                const auto* member_payload = field(*expressions.at(base), "payload");
                const int type_expression = integer(field(member_payload, "base"));
                const auto variant_type = expressions.count(type_expression) && text(field(*expressions.at(type_expression), "kind")) == "identifier"
                    ? text(field(field(*expressions.at(type_expression), "payload"), "name")) : std::string{};
                const auto variant_member = text(field(member_payload, "field"));
                if (variant_members.count(variant_type) && variant_members.at(variant_type).count(variant_member)) {
                    operation.kind = "variant_construct";
                    const auto concrete_type = symbol_types.count(result_symbol) ? symbol_types.at(result_symbol) : variant_type;
                    const auto instance = variant_substitution(concrete_type, variant_type);
                    operation.variant_type = concrete_type.empty() ? variant_type : concrete_type;
                    operation.variant_generic_owner = instance.first == variant_type && generic_variant_parameters.count(variant_type) ? variant_type : std::string{};
                    if (!operation.variant_generic_owner.empty()) {
                        operation.variant_type_arguments = split_type_instance(operation.variant_type).second;
                        operation.variant_instance_id = variant_instance_id(operation.variant_generic_owner, operation.variant_type_arguments);
                        if (operation.variant_type_arguments.size() != generic_variant_arities.at(variant_type))
                            add_diagnostic("FLOWANALYST_GENERIC_VARIANT_ARITY", "generic variant instance has the wrong number of type arguments", result_symbol, "statement:" + std::to_string(statement_id));
                        for (const auto& argument : operation.variant_type_arguments)
                            if (!is_resolved_type(argument))
                                add_diagnostic("FLOWANALYST_GENERIC_VARIANT_UNKNOWN_TYPE", "generic variant type argument '" + argument + "' cannot be resolved", result_symbol, "statement:" + std::to_string(statement_id));
                        for (const auto& parameter : generic_variant_parameters.at(variant_type)) {
                            const auto found = instance.second.find(parameter);
                            if (found != instance.second.end()) operation.variant_substitutions.emplace_back(parameter, found->second);
                        }
                    }
                    operation.variant_member = variant_member;
                    operation.variant_discriminant = variant_members.at(variant_type).at(variant_member);
                    for (const auto& [field_name, field_type] : variant_payloads.at(variant_type).at(variant_member)) {
                        static_cast<void>(field_name);
                        operation.variant_payload_types.push_back(substitute_type(field_type, instance.second));
                    }
                    const auto arguments = list(field(call_payload, "arguments"));
                    operation.arguments.clear();
                    for (const auto& argument : arguments) operation.arguments.push_back(integer(&argument));
                    if (operation.arguments.size() != operation.variant_payload_types.size()) {
                        add_diagnostic("FLOWANALYST_VARIANT_PAYLOAD_ARITY",
                                       "variant member '" + variant_type + "." + variant_member + "' payload count does not match its declaration",
                                       result_symbol, "statement:" + std::to_string(statement_id));
                    }
                    for (std::size_t index = 0; index < operation.arguments.size() && index < operation.variant_payload_types.size(); ++index) {
                        const auto actual_type = expression_type(operation.arguments[index]);
                        if (!actual_type.empty() && actual_type != operation.variant_payload_types[index]) {
                            add_diagnostic("FLOWANALYST_VARIANT_PAYLOAD_TYPE",
                                           "variant member '" + variant_type + "." + variant_member + "' payload type does not match its declaration",
                                           result_symbol, "statement:" + std::to_string(statement_id));
                        }
                    }
                }
            }
        }
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
        operation.function_symbol = containing_function(operation.scope);
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
        operation.function_symbol = containing_function(operation.scope);
        operation.then_block = integer(field(payload, "then_block"));
        operation.else_block = integer(field(field(payload, "else_arm"), "block"));
        operation.kind = "branch";
        if (operation.expression >= 0) operation.arguments.push_back(operation.expression);
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "guard") continue;
        const auto* payload = field(*statement, "payload");
        LoweringOperation operation;
        operation.expression = integer(field(payload, "condition_expression"));
        operation.statement = statement_id;
        operation.scope = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        operation.block = containing_block(statement_id);
        operation.function_symbol = containing_function(operation.scope);
        operation.failure_block = integer(field(payload, "failure_block"));
        operation.join_block = operation.block;
        operation.kind = "guard";
        if (operation.expression >= 0) operation.arguments.push_back(operation.expression);
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "when") continue;
        const auto* payload = field(*statement, "payload");
        LoweringOperation operation;
        operation.expression = integer(field(payload, "selector_expression"));
        operation.statement = statement_id;
        operation.scope = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        operation.block = containing_block(statement_id);
        operation.function_symbol = containing_function(operation.scope);
        operation.default_block = integer(field(payload, "default_block"));
        operation.join_block = operation.block;
        const int selector_symbol = resolved_expression_symbols.count(operation.expression) ? resolved_expression_symbols.at(operation.expression) : -1;
        operation.selector_type = symbol_types.count(selector_symbol) ? symbol_types.at(selector_symbol) : std::string{};
        const auto selector_instance = split_type_instance(operation.selector_type);
        operation.selector_kind = operation.selector_type == "int" || operation.selector_type.rfind("c_", 0) == 0 ? "integer" :
            (enum_types.count(operation.selector_type) ? "enum" : (variant_types.count(operation.selector_type) || variant_types.count(selector_instance.first) ? "variant" : "named"));
        if (operation.selector_kind == "variant" && generic_variant_parameters.count(selector_instance.first)) {
            operation.variant_generic_owner = selector_instance.first;
            operation.variant_type_arguments = selector_instance.second;
            operation.variant_instance_id = variant_instance_id(operation.variant_generic_owner, operation.variant_type_arguments);
            const auto& parameters = generic_variant_parameters.at(selector_instance.first);
            if (parameters.size() != selector_instance.second.size())
                add_diagnostic("FLOWANALYST_GENERIC_VARIANT_ARITY", "generic variant instance has the wrong number of type arguments", selector_symbol, "statement:" + std::to_string(statement_id));
            for (std::size_t index = 0; index < parameters.size() && index < selector_instance.second.size(); ++index)
                operation.variant_substitutions.emplace_back(parameters[index], selector_instance.second[index]);
        }
        operation.kind = "match";
        if (operation.expression >= 0) operation.arguments.push_back(operation.expression);
        for (const auto& arm : list(field(payload, "cases"))) {
            operation.match_values.push_back(integer(field(arm, "value")));
            operation.match_highs.push_back(integer(field(arm, "high")));
            operation.match_blocks.push_back(integer(field(arm, "block")));
            const auto label_type = text(field(arm, "label_type"));
            const auto label_member = text(field(arm, "label_member"));
            operation.match_label_types.push_back(label_type);
            operation.match_label_members.push_back(label_member);
            std::vector<VariantPayloadBinding> bindings;
            const int arm_block = integer(field(arm, "block"));
            const int arm_scope = block_scopes.count(arm_block) ? block_scopes.at(arm_block) : -1;
            if (operation.selector_kind == "variant" && arm_scope >= 0) {
                const auto prefix = label_type + "." + label_member + ".";
                for (const auto& candidate : list(field(*scopes.at(arm_scope), "symbol_ids"))) {
                    const int candidate_id = integer(&candidate);
                    if (!symbols.count(candidate_id)) continue;
                    const auto binding = fact_value(*symbols.at(candidate_id), "variant_payload_binding");
                    if (binding.rfind(prefix, 0) == 0) {
                        const auto declared = symbol_types.count(candidate_id) ? symbol_types.at(candidate_id) : std::string{};
                        bindings.push_back({candidate_id, text(field(*symbols.at(candidate_id), "name")),
                                            substitute_type(declared, variant_substitution(operation.selector_type, label_type).second)});
                    }
                }
                std::sort(bindings.begin(), bindings.end(), [](const auto& left, const auto& right) { return left.name < right.name; });
            }
            operation.match_payload_bindings.push_back(std::move(bindings));
        }
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "placement") continue;
        const auto* payload = field(*statement, "payload");
        const int value_expression = integer(field(payload, "value_expression"));
        if (value_expression < 0) continue;
        const int scope_id = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        LoweringOperation operation;
        operation.expression = value_expression;
        operation.statement = statement_id;
        operation.scope = scope_id;
        operation.block = containing_block(statement_id);
        operation.function_symbol = containing_function(scope_id);
        operation.result_symbol = visible_symbol(scope_id, text(field(field(payload, "target"), "name")));
        if (operation.result_symbol < 0) {
            const auto target_name = text(field(field(payload, "target"), "name"));
            int candidate = -1;
            for (const auto& [symbol_id, symbol] : symbols) if (text(field(*symbol, "name")) == target_name) {
                if (candidate >= 0) { candidate = -1; break; }
                candidate = symbol_id;
            }
            operation.result_symbol = candidate;
        }
        if (symbol_is_const(operation.result_symbol)) {
            add_diagnostic("FLOWANALYST_CONST_MUTATION",
                           "cannot assign to const binding",
                           operation.result_symbol,
                           "statement:" + std::to_string(statement_id));
        }
        if (expressions.count(value_expression) && text(field(*expressions.at(value_expression), "kind")) == "call") continue;
        operation.kind = "assignment";
        operation.arguments.push_back(value_expression);
        lowering_operations.push_back(std::move(operation));
    }
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "while") continue;
        const auto* payload = field(*statement, "payload");
        LoweringOperation operation;
        operation.expression = integer(field(payload, "condition_expression"));
        operation.statement = statement_id;
        operation.scope = statement_scopes.count(statement_id) ? statement_scopes.at(statement_id) : -1;
        operation.block = containing_block(statement_id);
        operation.function_symbol = containing_function(operation.scope);
        operation.body_block = integer(field(payload, "body_block"));
        operation.kind = "loop";
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
    std::cout << "{\n  \"format\": \"flowanalyst.semantic_report\",\n  \"version\": 1,\n  \"status\": \"" << (diagnostics.empty() ? "ok" : "error") << "\",\n  \"source\": {\"path\": " << quote(text(field(field(bundle, "source"), "path"))) << "},\n  \"frontend_bundle\": {\"format\": \"flowmini.frontend_bundle\", \"version\": 2},\n  \"diagnostics\": [";
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
    std::cout << "],\n  \"lowering_plan\": {\"format\":\"flowcore.lowering_plan\",\"version\":" << lowering_plan_version << ",\"status\":\""
              << (diagnostics.empty() ? "ready" : "blocked") << "\"";
    if (lowering_plan_version == 2) {
        std::cout << ",\"functions\":[";
        bool emitted_callable = false;
        for (std::size_t index = 0; index < callables.size(); ++index) {
            if (generic_functions.count(callables[index].symbol)) continue;
            if (emitted_callable) std::cout << ',';
            emitted_callable = true;
            const auto& callable = callables[index];
            std::cout << "{\"symbol_id\":" << callable.symbol << ",\"name\":" << quote(callable.name)
                      << ",\"scope_id\":" << callable.scope << ",\"body_block_id\":" << callable.body_block
                      << ",\"return_type\":" << quote(callable.return_type) << ",\"entry\":" << (callable.entry ? "true" : "false")
                      << ",\"availability\":" << quote(callable.availability)
                      << ",\"parameters\":[";
            for (std::size_t parameter = 0; parameter < callable.parameters.size(); ++parameter) {
                if (parameter) std::cout << ',';
                std::cout << "{\"symbol_id\":" << callable.parameters[parameter].first
                          << ",\"type\":" << quote(callable.parameters[parameter].second) << "}";
            }
            std::cout << "]}";
        }
        std::cout << "]";
        std::cout << ",\"generic_declarations\":[";
        bool emitted_generic = false;
        for (const auto& [symbol, generic] : generic_functions) {
            if (emitted_generic) std::cout << ',';
            emitted_generic = true;
            std::cout << "{\"symbol_id\":" << symbol
                      << ",\"name\":" << quote(generic.name)
                      << ",\"declaration_id\":" << generic.declaration
                      << ",\"type_parameters\":[";
            for (std::size_t index = 0; index < generic.parameters.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(generic.parameters[index]);
            }
            std::cout << "],\"parameter_types\":[";
            for (std::size_t index = 0; index < generic.parameter_types.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(generic.parameter_types[index]);
            }
            std::cout << "],\"return_type\":" << quote(generic.return_type) << "}";
        }
        std::cout << "]";
        std::cout << ",\"generic_records\":[";
        bool emitted_record = false;
        for (const auto& [declaration_id, declaration] : declarations) {
            if (text(field(*declaration, "kind")) != "record") continue;
            const auto parameters = list(field(*declaration, "type_parameters"));
            if (parameters.empty()) continue;
            int record_symbol = -1;
            for (const auto& [symbol_id, symbol] : symbols)
                if (text(field(*symbol, "kind")) == "Struct" && text(field(*symbol, "name")) == text(field(*declaration, "name"))) { record_symbol = symbol_id; break; }
            if (emitted_record) std::cout << ',';
            emitted_record = true;
            std::cout << "{\"symbol_id\":" << record_symbol
                      << ",\"name\":" << quote(text(field(*declaration, "name")))
                      << ",\"declaration_id\":" << declaration_id << ",\"type_parameters\":[";
            for (std::size_t index = 0; index < parameters.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(text(field(parameters[index], "name")));
            }
            std::cout << "],\"fields\":[";
            const auto fields = list(field(*declaration, "fields"));
            for (std::size_t index = 0; index < fields.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << "{\"name\":" << quote(text(field(fields[index], "name")))
                          << ",\"type\":" << quote(text(field(fields[index], "type"))) << "}";
            }
            std::cout << "]}";
        }
        std::cout << "]";
        std::cout << ",\"generic_variants\":[";
        bool emitted_generic_variant = false;
        for (const auto& [declaration_id, declaration] : declarations) {
            if (text(field(*declaration, "kind")) != "variant") continue;
            const auto parameters = list(field(*declaration, "type_parameters"));
            if (parameters.empty()) continue;
            if (emitted_generic_variant) std::cout << ',';
            emitted_generic_variant = true;
            std::cout << "{\"name\":" << quote(text(field(*declaration, "name")))
                      << ",\"declaration_id\":" << declaration_id << ",\"type_parameters\":[";
            for (std::size_t index = 0; index < parameters.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(text(field(parameters[index], "name")));
            }
            std::cout << "],\"members\":[";
            const auto members = list(field(*declaration, "members"));
            for (std::size_t member_index = 0; member_index < members.size(); ++member_index) {
                if (member_index) std::cout << ',';
                const auto& member = members[member_index];
                std::cout << "{\"name\":" << quote(text(field(member, "name")))
                          << ",\"discriminant\":" << member_index << ",\"payload_types\":[";
                const auto fields = list(field(member, "fields"));
                for (std::size_t field_index = 0; field_index < fields.size(); ++field_index) {
                    if (field_index) std::cout << ',';
                    std::cout << quote(text(field(fields[field_index], "type")));
                }
                std::cout << "]}";
            }
            std::cout << "]}";
        }
        std::cout << "]";
    }
    std::cout << ",\"operations\":[";
    std::map<int, std::string> generic_expression_types;
    for (const auto& operation : lowering_operations)
        if (operation.kind == "generic_call" && !operation.generic_return_type.empty())
            generic_expression_types[operation.expression] = operation.generic_return_type;
    std::function<void(int, const std::string&)> emit_operand = [&](int expression_id, const std::string& declared_type) {
        const auto* expression = expressions.count(expression_id) ? expressions.at(expression_id) : nullptr;
        const auto kind = text(field(expression, "kind"));
        const auto literal = text(field(field(expression, "payload"), "value_text"), "0");
        const int identifier_symbol = kind == "identifier" && resolved_expression_symbols.count(expression_id)
            ? resolved_expression_symbols.at(expression_id) : -1;
        const auto identifier_type = symbol_types.count(identifier_symbol) ? symbol_types.at(identifier_symbol) : std::string{};
        const bool carrier_conversion = kind == "identifier" && !declared_type.empty() &&
            !identifier_type.empty() && identifier_type != declared_type;
        const bool writable_storage = kind == "integer_literal" && declared_type == "c_pointer" &&
            !literal.empty() && literal != "0" && literal.front() != '-';
        bool ordinary_call = false;
        if (lowering_plan_version == 2 && kind == "call") {
            const int base = integer(field(field(expression, "payload"), "base"));
            const auto callee = expressions.count(base) && text(field(*expressions.at(base), "kind")) == "identifier"
                ? text(field(field(*expressions.at(base), "payload"), "name")) : std::string{};
            ordinary_call = callee != "length";
        }
        std::cout << "{\"expression_id\":" << expression_id << ",\"kind\":" << quote(carrier_conversion ? "conversion" : (writable_storage ? "writable_storage" : (ordinary_call ? "call_result" : kind)));
        if (carrier_conversion) {
            std::cout << ",\"type\":" << quote(declared_type) << ",\"from_type\":" << quote(identifier_type)
                      << ",\"conversion\":\"explicit_typed_initializer\",\"operand\":";
            emit_operand(expression_id, {});
        } else if (writable_storage) {
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
            std::cout << ",\"type\":" << quote(identifier_type) << ",\"symbol_id\":" << identifier_symbol;
        } else if (kind == "field_access") {
            const auto* payload = field(expression, "payload");
            const auto member = text(field(payload, "field"));
            const auto base = integer(field(payload, "base"));
            const auto type = expressions.count(base) && text(field(*expressions.at(base), "kind")) == "identifier"
                ? text(field(field(*expressions.at(base), "payload"), "name")) : std::string{};
            const int base_symbol = resolved_expression_symbols.count(base) ? resolved_expression_symbols.at(base) : -1;
            const auto base_type = symbol_types.count(base_symbol) ? symbol_types.at(base_symbol) : std::string{};
            const auto base_instance = split_type_instance(base_type);
            if (variant_payloads.count(base_instance.first)) {
                std::string payload_type;
                std::string payload_member;
                int discriminant = -1;
                for (const auto& [member_name, fields] : variant_payloads.at(base_instance.first)) {
                    for (const auto& [field_name, field_type] : fields) {
                        if (field_name == member) {
                            if (!payload_type.empty()) {
                                add_diagnostic("FLOWANALYST_VARIANT_PAYLOAD_AMBIGUOUS",
                                               "variant payload field '" + member + "' is ambiguous for type '" + base_type + "'",
                                               base_symbol, "expression:" + std::to_string(expression_id));
                            }
                            payload_type = substitute_type(field_type, variant_substitution(base_type, base_instance.first).second);
                            payload_member = member_name;
                            discriminant = variant_members.at(base_instance.first).at(member_name);
                        }
                    }
                }
                if (!payload_type.empty()) {
                    std::cout << ",\"type\":" << quote(payload_type)
                              << ",\"variant_type\":" << quote(base_type)
                              << ",\"variant_member\":" << quote(payload_member)
                              << ",\"discriminant\":" << discriminant
                              << ",\"payload_field\":" << quote(member)
                              << ",\"selector_symbol_id\":" << base_symbol
                              << ",\"provenance\":{\"source\":\"variant\",\"member\":" << quote(base_type + "." + payload_member)
                              << "}";
                    std::cout << "}";
                    return;
                }
            }
            const auto found_type = enum_members.find(type);
            if (found_type == enum_members.end() || !found_type->second.count(member)) std::cout << ",\"type\":\"unsupported\"";
            else std::cout << ",\"type\":\"int\",\"value\":\"" << found_type->second.at(member) << "\"";
        } else if (kind == "index") {
            const auto* payload = field(expression, "payload");
            const int base = integer(field(payload, "base"));
            const int symbol = resolved_expression_symbols.count(base) ? resolved_expression_symbols.at(base) : -1;
            const auto type = symbol_types.count(symbol) ? symbol_types.at(symbol) : std::string{};
            const auto indexes = list(field(payload, "indexes"));
            if (type == "list<string>" && indexes.size() == 1) {
                std::cout << ",\"intrinsic\":\"list_index\",\"type\":\"c_string\",\"symbol_id\":" << symbol << ",\"index\":";
                emit_operand(integer(&indexes.front()), "c_int");
            } else std::cout << ",\"type\":\"unsupported\"";
        } else if (kind == "call") {
            const auto* payload = field(expression, "payload");
            const int base = integer(field(payload, "base"));
            const auto callee = expressions.count(base) && text(field(*expressions.at(base), "kind")) == "identifier"
                ? text(field(field(*expressions.at(base), "payload"), "name")) : std::string{};
            if (callee.empty() && expressions.count(base) && text(field(*expressions.at(base), "kind")) == "field_access") {
                const auto* member_payload = field(*expressions.at(base), "payload");
                const auto member = text(field(member_payload, "field"));
                const auto member_base = integer(field(member_payload, "base"));
                const auto type = expressions.count(member_base) && text(field(*expressions.at(member_base), "kind")) == "identifier"
                    ? text(field(field(*expressions.at(member_base), "payload"), "name")) : std::string{};
                if (variant_members.count(type) && variant_members.at(type).count(member)) {
                    std::cout << ",\"type\":\"int\",\"value\":\"" << variant_members.at(type).at(member) << "\"";
                    std::cout << "}";
                    return;
                }
            }
            const auto arguments = list(field(payload, "arguments"));
            if (callee == "length" && arguments.size() == 1) {
                const int argument = integer(&arguments.front());
                const int symbol = resolved_expression_symbols.count(argument) ? resolved_expression_symbols.at(argument) : -1;
                const auto type = symbol_types.count(symbol) ? symbol_types.at(symbol) : std::string{};
                if (type == "list<string>")
                    std::cout << ",\"intrinsic\":\"list_length\",\"type\":\"c_int\",\"symbol_id\":" << symbol;
                else std::cout << ",\"type\":\"unsupported\"";
            } else {
                const int callee_symbol = resolved_expression_symbols.count(base) ? resolved_expression_symbols.at(base) : -1;
                if (lowering_plan_version == 2 && symbols.count(callee_symbol) && text(field(*symbols.at(callee_symbol), "kind")) == "Function") {
                    const auto return_type = generic_expression_types.count(expression_id) ? generic_expression_types.at(expression_id) : fact_value(*symbols.at(callee_symbol), "return_type_spelling");
                    std::cout << ",\"type\":" << quote(return_type)
                              << ",\"callee_symbol_id\":" << callee_symbol << ",\"arguments\":[";
                    for (std::size_t index = 0; index < arguments.size(); ++index) {
                        if (index) std::cout << ',';
                        emit_operand(integer(&arguments[index]), {});
                    }
                    std::cout << "]";
                } else std::cout << ",\"type\":\"unsupported\"";
            }
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
    bool emitted_operation = false;
    for (std::size_t i = 0; i < lowering_operations.size(); ++i) {
        const auto& operation = lowering_operations[i];
        if (generic_functions.count(operation.function_symbol)) continue;
        if (emitted_operation) std::cout << ',';
        emitted_operation = true;
        std::cout << "{\"id\":" << i
                  << ",\"kind\":" << quote(operation.kind)
                  << ",\"expression_id\":" << operation.expression
                  << ",\"statement_id\":" << operation.statement
                  << ",\"scope_id\":" << operation.scope;
        if (lowering_plan_version == 2) std::cout << ",\"function_symbol_id\":" << operation.function_symbol;
        std::cout
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
            std::string declared_type;
            if (operation.kind == "variant_construct" && argument < operation.variant_payload_types.size()) {
                declared_type = operation.variant_payload_types[argument];
            } else if ((operation.kind == "value_definition" || operation.kind == "assignment") && operation.result_symbol >= 0 && symbol_types.count(operation.result_symbol)) {
                declared_type = symbol_types.at(operation.result_symbol);
            }
            emit_operand(operation.arguments[argument], declared_type);
        }
        std::cout << "]";
        if (operation.result_symbol >= 0) std::cout << ",\"result_symbol_id\":" << operation.result_symbol;
        if (operation.kind == "value_definition" && !operation.compile_time_value.empty()) {
            std::cout << ",\"evaluation\":\"compile_time\",\"compile_time_value\":" << quote(operation.compile_time_value);
        }
        if (operation.kind == "variant_construct") {
            std::cout << ",\"variant_type\":" << quote(operation.variant_type)
                      << ",\"variant_member\":" << quote(operation.variant_member)
                      << ",\"variant_discriminant\":" << operation.variant_discriminant
                      << ",\"payload_types\":[";
            for (std::size_t index = 0; index < operation.variant_payload_types.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(operation.variant_payload_types[index]);
            }
            std::cout << "]";
            if (!operation.variant_generic_owner.empty()) {
                std::cout << ",\"generic_owner\":" << quote(operation.variant_generic_owner)
                          << ",\"instance_id\":" << quote(operation.variant_instance_id)
                          << ",\"type_arguments\":[";
                for (std::size_t index = 0; index < operation.variant_type_arguments.size(); ++index) {
                    if (index) std::cout << ',';
                    std::cout << quote(operation.variant_type_arguments[index]);
                }
                std::cout << "],\"substitutions\":[";
                for (std::size_t index = 0; index < operation.variant_substitutions.size(); ++index) {
                    if (index) std::cout << ',';
                    std::cout << "{\"parameter\":" << quote(operation.variant_substitutions[index].first)
                              << ",\"type\":" << quote(operation.variant_substitutions[index].second) << "}";
                }
                std::cout << "]";
            }
        }
        if (operation.kind == "generic_call") {
            std::cout << ",\"generic_owner\":" << quote(operation.generic_owner)
                      << ",\"generic_mode\":\"forward_first_argument\""
                      << ",\"generic_return_type\":" << quote(operation.generic_return_type)
                      << ",\"instantiation_id\":" << quote(operation.instantiation_id)
                      << ",\"type_arguments\":[";
            for (std::size_t index = 0; index < operation.generic_type_arguments.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(operation.generic_type_arguments[index]);
            }
            std::cout << "],\"substitutions\":[";
            for (std::size_t index = 0; index < operation.generic_substitutions.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << "{\"parameter\":" << quote(operation.generic_substitutions[index].first)
                          << ",\"type\":" << quote(operation.generic_substitutions[index].second) << "}";
            }
            std::cout << "]";
        }
        if (operation.kind == "branch") std::cout << ",\"then_block_id\":" << operation.then_block << ",\"else_block_id\":" << operation.else_block;
        if (operation.kind == "guard") std::cout << ",\"failure_block_id\":" << operation.failure_block << ",\"join_block_id\":" << operation.join_block;
        if (operation.kind == "loop") std::cout << ",\"body_block_id\":" << operation.body_block;
        if (operation.kind == "match") {
            std::cout << ",\"selector_type\":" << quote(operation.selector_type)
                      << ",\"selector_kind\":" << quote(operation.selector_kind) << ",\"default_block_id\":" << operation.default_block
                      << ",\"join_block_id\":" << operation.join_block;
            if (!operation.variant_generic_owner.empty()) {
                std::cout << ",\"variant_generic_owner\":" << quote(operation.variant_generic_owner)
                          << ",\"variant_instance_id\":" << quote(operation.variant_instance_id)
                          << ",\"variant_type_arguments\":[";
                for (std::size_t index = 0; index < operation.variant_type_arguments.size(); ++index) {
                    if (index) std::cout << ',';
                    std::cout << quote(operation.variant_type_arguments[index]);
                }
                std::cout << "],\"variant_substitutions\":[";
                for (std::size_t index = 0; index < operation.variant_substitutions.size(); ++index) {
                    if (index) std::cout << ',';
                    std::cout << "{\"parameter\":" << quote(operation.variant_substitutions[index].first)
                              << ",\"type\":" << quote(operation.variant_substitutions[index].second) << "}";
                }
                std::cout << "]";
            }
            std::cout << ",\"cases\":[";
            for (std::size_t arm = 0; arm < operation.match_values.size(); ++arm) {
                if (arm) std::cout << ',';
                std::cout << "{\"value\":" << operation.match_values[arm]
                          << ",\"high\":" << operation.match_highs[arm]
                          << ",\"body_block_id\":" << operation.match_blocks[arm];
                if (arm < operation.match_label_types.size() && !operation.match_label_types[arm].empty()) {
                    std::cout << ",\"label_type\":" << quote(operation.match_label_types[arm])
                              << ",\"label_member\":" << quote(operation.match_label_members[arm]);
                }
                if (arm < operation.match_payload_bindings.size() && !operation.match_payload_bindings[arm].empty()) {
                    std::cout << ",\"payload_bindings\":[";
                    for (std::size_t binding = 0; binding < operation.match_payload_bindings[arm].size(); ++binding) {
                        if (binding) std::cout << ',';
                        const auto& item = operation.match_payload_bindings[arm][binding];
                        std::cout << "{\"symbol_id\":" << item.symbol
                                  << ",\"name\":" << quote(item.name)
                                  << ",\"type\":" << quote(item.type) << "}";
                    }
                    std::cout << "]";
                }
                std::cout << "}";
            }
            std::cout << "]";
        }
        if (operation.kind == "external_call") {
            std::cout << ",\"provider\":{\"contract\":" << quote(operation.contract)
                      << ",\"library\":" << quote(operation.library)
                      << ",\"convention\":" << quote(operation.convention)
                      << ",\"symbol\":" << quote(operation.symbol)
                      << ",\"effect\":" << quote(operation.effect)
                      << ",\"parameter_types\":" << quote(operation.parameter_types)
                      << ",\"return_type\":" << quote(operation.return_type) << "}"
                      << ",\"effect_class\":" << quote(operation.effect_class.empty() ? operation.effect : operation.effect_class);
            std::cout << ",\"effect_contract\":{\"external\":" << quote(operation.effect)
                      << ",\"determinism\":" << quote(operation.effect == "pure" ? "deterministic" : "unspecified")
                      << ",\"certainty\":\"declared\"}";
            std::cout << ",\"argument_resources\":[";
            const auto parameter_carriers = operation.parameter_types.empty()
                ? std::vector<std::string>{} : split_generic_arguments(operation.parameter_types);
            for (std::size_t parameter = 0; parameter < parameter_carriers.size(); ++parameter) {
                if (parameter) std::cout << ',';
                const AbiTypeContract* contract = nullptr;
                for (const auto& type : abi_type_contracts) if (type.contract == operation.contract && type.name == parameter_carriers[parameter]) { contract = &type; break; }
                const auto memory_effect = contract == nullptr ? "none" :
                    (contract->access == "read" ? "read" : (contract->access == "read_write" || contract->access == "write" ? "read_write" : "opaque"));
                std::cout << "{\"index\":" << parameter << ",\"type\":" << quote(parameter_carriers[parameter])
                          << ",\"memory_effect\":" << quote(memory_effect)
                          << ",\"ownership\":" << quote(contract ? contract->ownership : "none")
                          << ",\"access\":" << quote(contract ? contract->access : "value")
                          << ",\"lifetime\":" << quote(contract ? contract->lifetime : "value")
                          << ",\"nullable\":" << quote(contract ? contract->nullable : "not_applicable")
                          << ",\"opaque\":" << quote(contract ? contract->opaque : "false");
                for (const auto& use : operation.resources) if (use.argument == static_cast<int>(parameter)) {
                    std::cout << ",\"resource_identity\":" << quote(use.identity)
                              << ",\"alias_status\":" << quote(use.alias_status)
                              << ",\"resource_kind\":" << quote(use.type)
                              << ",\"concurrency\":\"unknown\"";
                    break;
                }
                std::cout << "}";
            }
            std::cout << "]";
            for (const auto& type : abi_type_contracts) {
                if (type.contract != operation.contract || type.name != operation.return_type || type.cleanup.empty()) continue;
                std::cout << ",\"result_resource\":{\"type\":" << quote(type.name)
                          << ",\"ownership\":" << quote(type.ownership)
                          << ",\"access\":" << quote(type.access)
                          << ",\"lifetime\":" << quote(type.lifetime)
                          << ",\"nullable\":" << quote(type.nullable)
                          << ",\"opaque\":" << quote(type.opaque)
                          << ",\"cleanup_capability\":" << quote(type.cleanup);
                if (operation.produces_resource) {
                    std::cout << ",\"resource_identity\":" << quote(operation.produced_resource.identity)
                              << ",\"alias_status\":" << quote(operation.produced_resource.alias_status)
                              << ",\"resource_kind\":" << quote(operation.produced_resource.type)
                              << ",\"concurrency\":\"unknown\"";
                }
                std::cout << "}";
            }
        }
        std::cout << "}";
    }
    std::cout << "]},\n  \"match_facts\": [";
    bool first_match = true;
    for (const auto& [statement_id, statement] : statements) {
        if (text(field(*statement, "kind")) != "when") continue;
        const auto* payload = field(*statement, "payload");
        const int selector = integer(field(payload, "selector_expression"));
        const int selector_symbol = resolved_expression_symbols.count(selector) ? resolved_expression_symbols.at(selector) : -1;
        const auto selector_type = symbol_types.count(selector_symbol) ? symbol_types.at(selector_symbol) : std::string{};
        const auto selector_instance = split_type_instance(selector_type);
        const std::string selector_kind = selector_type == "int" || selector_type.rfind("c_", 0) == 0 ? "integer" :
            (enum_types.count(selector_type) ? "enum" : (variant_types.count(selector_type) || variant_types.count(selector_instance.first) ? "variant" : (selector_type.empty() ? "unknown" : "named")));
        if (!first_match) std::cout << ',';
        first_match = false;
        std::cout << "{\"kind\":\"match\",\"statement_id\":" << statement_id
                  << ",\"selector_expression\":" << selector
                  << ",\"selector_symbol_id\":" << selector_symbol
                  << ",\"selector_type\":" << quote(selector_type)
                  << ",\"selector_kind\":" << quote(selector_kind)
                  << ",\"cases\":[";
        const auto cases = list(field(payload, "cases"));
        for (std::size_t index = 0; index < cases.size(); ++index) {
            if (index) std::cout << ',';
            const auto& arm = cases[index];
            std::cout << "{\"value\":" << integer(field(arm, "value"))
                      << ",\"high\":" << integer(field(arm, "high"))
                      << ",\"body_block_id\":" << integer(field(arm, "block"));
            if (const auto* label = field(arm, "label_type")) {
                const auto label_type = text(label);
                const auto label_instance = split_type_instance(label_type);
                const auto selector_instance = split_type_instance(selector_type);
                const bool same_generic_owner =
                    ((label_instance.second.empty() && selector_instance.first == label_type && generic_variant_parameters.count(label_type)) ||
                     (!label_instance.second.empty() && selector_instance.first == label_instance.first &&
                      selector_instance.second.size() == label_instance.second.size()));
                const bool generic_base_label = label_instance.second.empty() && selector_instance.first == label_type && generic_variant_parameters.count(label_type);
                const bool label_compatible = label_type == selector_type || generic_base_label ||
                    (same_generic_owner && !label_instance.second.empty() && label_instance.second == selector_instance.second);
                if (!selector_type.empty() && !label_compatible)
                    add_diagnostic("FLOWANALYST_MATCH_LABEL_TYPE", "match label type '" + label_type + "' does not match selector type '" + selector_type + "'", selector_symbol, "statement:" + std::to_string(statement_id));
                if (selector_kind == "integer")
                    add_diagnostic("FLOWANALYST_MATCH_LABEL_ON_INTEGER", "integer match selector cannot use named member label", selector_symbol, "statement:" + std::to_string(statement_id));
                std::cout << ",\"label_type\":" << quote(text(label));
                if (const auto* member = field(arm, "label_member")) std::cout << ",\"label_member\":" << quote(text(member));
            }
            const LoweringOperation* match_operation = nullptr;
            for (const auto& candidate : lowering_operations) {
                if (candidate.kind == "match" && candidate.statement == statement_id) {
                    match_operation = &candidate;
                    break;
                }
            }
            if (match_operation != nullptr && index < match_operation->match_payload_bindings.size() &&
                !match_operation->match_payload_bindings[index].empty()) {
                std::cout << ",\"payload_bindings\": [";
                const auto& bindings = match_operation->match_payload_bindings[index];
                for (std::size_t binding = 0; binding < bindings.size(); ++binding) {
                    if (binding) std::cout << ',';
                    std::cout << "{\"symbol_id\":" << bindings[binding].symbol
                              << ",\"name\":" << quote(bindings[binding].name)
                              << ",\"type\":" << quote(bindings[binding].type) << "}";
                }
                std::cout << "]";
            }
            std::cout << "}";
        }
        std::cout << "] ,\"default_block_id\":" << integer(field(payload, "default_block"))
                  << ",\"join_block_id\":" << containing_block(statement_id) << "}";
    }
    std::cout << "],\n  \"variant_carriers\": [";
    bool first_variant = true;
    for (const auto& [variant_type, members] : variant_payloads) {
        if (!first_variant) std::cout << ',';
        first_variant = false;
        std::cout << "{\"variant_type\":" << quote(variant_type);
        if (generic_variant_parameters.count(variant_type)) {
            std::cout << ",\"generic\":true,\"type_parameters\":[";
            for (std::size_t index = 0; index < generic_variant_parameters.at(variant_type).size(); ++index) {
                if (index) std::cout << ',';
                std::cout << quote(generic_variant_parameters.at(variant_type)[index]);
            }
            std::cout << "]";
        }
        std::cout << ",\"members\":[";
        bool first_member = true;
        for (const auto& [member_name, fields] : members) {
            if (!first_member) std::cout << ',';
            first_member = false;
            std::cout << "{\"member\":" << quote(member_name)
                      << ",\"discriminant\":" << variant_members.at(variant_type).at(member_name)
                      << ",\"payload_fields\":[";
            for (std::size_t field_index = 0; field_index < fields.size(); ++field_index) {
                if (field_index) std::cout << ',';
                std::cout << "{\"name\":" << quote(fields[field_index].first)
                          << ",\"type\":" << quote(fields[field_index].second) << "}";
            }
            std::cout << "]}";
        }
        std::cout << "]}";
    }
    std::cout << "],\n  \"enum_types\":[";
    bool first_enum_type = true;
    for (const auto& enum_type : enum_types) {
        if (!first_enum_type) std::cout << ',';
        first_enum_type = false;
        std::cout << quote(enum_type);
    }
    std::cout << "],\n  \"effect_facts\": [";
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
        std::cout << ",\"purity\":" << (site.pure ? "\"pure\"" : "\"effectful\"")
                  << ",\"effect_class\":" << quote(site.effect);
        if (!site.resources.empty()) {
            std::cout << ",\"resource_uses\":[";
            for (std::size_t resource = 0; resource < site.resources.size(); ++resource) {
                if (resource) std::cout << ',';
                const auto& use = site.resources[resource];
                std::cout << "{\"argument\":" << use.argument
                          << ",\"symbol_id\":" << use.symbol
                          << ",\"resource_identity\":" << quote(use.identity)
                          << ",\"alias_status\":" << quote(use.alias_status)
                          << ",\"resource_kind\":" << quote(use.type)
                          << ",\"access\":" << quote(use.access)
                          << ",\"ownership\":" << quote(use.ownership)
                          << ",\"lifetime\":" << quote(use.lifetime)
                          << ",\"opaque\":" << quote(use.opaque)
                          << ",\"concurrency\":\"unknown\"}";
            }
            std::cout << "]";
        }
        if (site.produces_resource) {
            const auto& resource = site.produced_resource;
            std::cout << ",\"result_resource\":{\"resource_identity\":" << quote(resource.identity)
                      << ",\"alias_status\":" << quote(resource.alias_status)
                      << ",\"resource_kind\":" << quote(resource.type)
                      << ",\"access\":" << quote(resource.access)
                      << ",\"ownership\":" << quote(resource.ownership)
                      << ",\"lifetime\":" << quote(resource.lifetime)
                      << ",\"opaque\":" << quote(resource.opaque) << "}";
        }
        std::cout << "}";
    }
    std::cout << "],\n  \"parallel_candidates\": [";
    bool first_candidate = true;
    for (const auto& site : call_sites) if (!site.independent_with.empty()) {
        if (!first_candidate) std::cout << ',';
        first_candidate = false;
        const auto resource_compatibility = site.external ? "value-only" : "not-applicable";
        std::cout << "{\"call_expression\":" << site.expression << ",\"statement_id\":" << site.statement << ",\"callee\":" << quote(site.callee) << ",\"proof\":\"pure-callee-disjoint-inputs\",\"proof_status\":\"proven\",\"status\":\"deferred\",\"provenance\":{\"source\":" << quote(text(field(field(bundle, "source"), "path"))) << ",\"ast_path\":\"/statement_pool/" << site.statement << "\"},\"evidence\":{\"dependency_independent\":true,\"effect_compatible\":true,\"mutation_conflict\":false,\"resource_compatibility\":" << quote(resource_compatibility) << ",\"alias_status\":\"not-applicable\",\"provider_contract\":" << quote(site.external ? site.provider_contract : "internal") << ",\"input_symbols\":[";
        bool first_read = true;
        for (const auto symbol : site.reads) { if (!first_read) std::cout << ','; first_read = false; std::cout << symbol; }
        std::cout << "],\"output_symbol\":" << site.write_symbol << "},\"independent_with\":[";
        for (std::size_t i = 0; i < site.independent_with.size(); ++i) { if (i) std::cout << ','; std::cout << site.independent_with[i]; }
        std::cout << "]}";
    }
    std::cout << "],\n  \"parallel_rejections\":[";
    for (std::size_t index = 0; index < parallel_rejections.size(); ++index) {
        if (index) std::cout << ',';
        const auto& rejection = parallel_rejections[index];
        const bool resource_reason = rejection.reason.rfind("resource-", 0) == 0 || rejection.reason == "provider-concurrency-unknown";
        const bool conflict_reason = rejection.reason == "conflicting-output" || rejection.reason == "read-after-write-dependency" || rejection.reason == "resource-read-write-conflict" || rejection.reason == "resource-write-write-conflict" || rejection.reason == "resource-alias-unknown";
        std::cout << "{\"left_call_expression\":" << rejection.left << ",\"right_call_expression\":" << rejection.right << ",\"reason\":" << quote(rejection.reason) << ",\"proof_status\":\"not-proven\",\"fallback\":\"serial\",\"evidence\":{\"dependency_independent\":false,\"effect_compatible\":" << (rejection.left_effect == "pure" && rejection.right_effect == "pure" ? "true" : "false")
                  << ",\"mutation_conflict\":" << (conflict_reason ? "true" : "false")
                  << ",\"resource_compatibility\":" << quote(resource_reason ? (conflict_reason ? "conflict-or-alias" : "unknown") : "not-applicable")
                  << ",\"alias_status\":" << quote(resource_reason ? "unknown" : "not-applicable")
                  << ",\"left_effect\":" << quote(rejection.left_effect)
                  << ",\"right_effect\":" << quote(rejection.right_effect) << "},\"provenance\":{\"source\":" << quote(text(field(field(bundle, "source"), "path"))) << "}}";
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
        int lowering_plan_version = 1; std::string input_path;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--lowering-plan-version") {
                if (++index >= argc) throw std::runtime_error("--lowering-plan-version requires 1 or 2");
                lowering_plan_version = std::stoi(argv[index]);
                if (lowering_plan_version != 1 && lowering_plan_version != 2) throw std::runtime_error("unsupported lowering plan version");
            } else if (!argument.empty() && argument.front() == '-') throw std::runtime_error("unknown option: " + argument);
            else if (input_path.empty()) input_path = argument;
            else throw std::runtime_error("too many input paths");
        }
        std::ostringstream input;
        if (!input_path.empty()) { std::ifstream file(input_path); if (!file) throw std::runtime_error("cannot open bundle"); input << file.rdbuf(); }
        else input << std::cin.rdbuf();
        return run(Parser(input.str()).parse(), lowering_plan_version);
    }
    catch (const std::exception& error) { std::cerr << "flowanalyst error: " << error.what() << '\n'; return 1; }
}
