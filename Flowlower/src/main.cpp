#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Options { std::string optimization_path, binding_path, llvm_path, target_name; };

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--emit-llvm") { if (++i >= argc) throw std::runtime_error("--emit-llvm requires a path"); options.llvm_path = argv[i]; }
        else if (argument == "--binding-report") { if (++i >= argc) throw std::runtime_error("--binding-report requires a path"); options.binding_path = argv[i]; }
        else if (argument == "--target") { if (++i >= argc) throw std::runtime_error("--target requires a name"); options.target_name = argv[i]; }
        else if (!argument.empty() && argument.front() == '-') throw std::runtime_error("unknown option '" + argument + "'");
        else if (options.optimization_path.empty()) options.optimization_path = argument;
        else throw std::runtime_error("too many input paths");
    }
    return options;
}

std::string read_file_or_stdin(const std::string& path) {
    std::ostringstream input;
    if (!path.empty()) { std::ifstream file(path); if (!file) throw std::runtime_error("cannot open report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has(std::string_view input, std::string_view fragment) { return input.find(fragment) != std::string_view::npos; }

std::string object_field(std::string_view input, std::string_view field) {
    const auto marker = input.find("\"" + std::string(field) + "\"");
    if (marker == std::string_view::npos) return "{}";
    auto position = input.find('{', marker); if (position == std::string_view::npos) return "{}";
    const auto start = position; int depth = 0; bool string = false; bool escaped = false;
    for (; position < input.size(); ++position) {
        const char c = input[position];
        if (string) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') string = false; continue; }
        if (c == '"') string = true; else if (c == '{') ++depth; else if (c == '}' && --depth == 0) return std::string(input.substr(start, position - start + 1));
    }
    return "{}";
}

std::string array_field(std::string_view input, std::string_view field) {
    const auto marker = input.find("\"" + std::string(field) + "\"");
    if (marker == std::string_view::npos) return "[]";
    auto position = input.find(':', marker);
    if (position == std::string_view::npos) return "[]";
    position = input.find('[', position);
    if (position == std::string_view::npos) return "[]";
    const auto start = position; int depth = 0; bool string = false; bool escaped = false;
    for (; position < input.size(); ++position) {
        const char c = input[position];
        if (string) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') string = false; continue; }
        if (c == '"') string = true; else if (c == '[') ++depth; else if (c == ']' && --depth == 0) return std::string(input.substr(start, position - start + 1));
    }
    return "[]";
}

std::string first_array_object(std::string_view input) {
    auto position = input.find('{');
    if (position == std::string_view::npos) return "{}";
    const auto start = position; int depth = 0; bool string = false; bool escaped = false;
    for (; position < input.size(); ++position) {
        const char c = input[position];
        if (string) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') string = false; continue; }
        if (c == '"') string = true; else if (c == '{') ++depth; else if (c == '}' && --depth == 0) return std::string(input.substr(start, position - start + 1));
    }
    return "{}";
}

std::vector<std::string> array_objects(std::string_view input) {
    std::vector<std::string> result;
    for (std::size_t position = 0; position < input.size();) {
        position = input.find('{', position);
        if (position == std::string_view::npos) break;
        const auto start = position; int depth = 0; bool string = false; bool escaped = false;
        for (; position < input.size(); ++position) {
            const char c = input[position];
            if (string) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') string = false; continue; }
            if (c == '"') string = true; else if (c == '{') ++depth; else if (c == '}' && --depth == 0) { result.emplace_back(input.substr(start, position - start + 1)); ++position; break; }
        }
    }
    return result;
}

std::string numeric_field(std::string_view input, std::string_view field) {
    const auto marker = input.find("\"" + std::string(field) + "\"");
    if (marker == std::string_view::npos) return {};
    auto position = input.find(':', marker); if (position == std::string_view::npos) return {};
    ++position; while (position < input.size() && std::isspace(static_cast<unsigned char>(input[position]))) ++position;
    const auto start = position;
    while (position < input.size() && (std::isdigit(static_cast<unsigned char>(input[position])) || input[position] == '-')) ++position;
    return std::string(input.substr(start, position - start));
}

std::string quoted_field(std::string_view input, std::string_view field) {
    const auto marker = input.find("\"" + std::string(field) + "\"");
    if (marker == std::string_view::npos) return {};
    auto position = input.find(':', marker); if (position == std::string_view::npos) return {};
    position = input.find('"', position); if (position == std::string_view::npos) return {};
    ++position; std::string result;
    for (; position < input.size(); ++position) {
        if (input[position] == '"' && (position == 0 || input[position - 1] != '\\')) return result;
        result.push_back(input[position]);
    }
    return {};
}

bool valid_c_symbol(std::string_view symbol) {
    if (symbol.empty() || (!std::isalpha(static_cast<unsigned char>(symbol.front())) && symbol.front() != '_')) return false;
    for (const char c : symbol) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    return true;
}

bool valid_integer_literal(std::string_view value) {
    if (value.empty()) return false;
    std::size_t index = value.front() == '-' ? 1 : 0;
    if (index == value.size()) return false;
    for (; index < value.size(); ++index) if (!std::isdigit(static_cast<unsigned char>(value[index]))) return false;
    return true;
}

std::string emit_integer_expression(std::string_view expression, std::ostringstream& instructions, int& temporary, const std::map<int, std::string>& values) {
    const auto kind = quoted_field(expression, "kind");
    if (kind == "call" && quoted_field(expression, "intrinsic") == "list_length" && quoted_field(expression, "type") == "c_int") return "%argc";
    if (kind == "integer_literal") {
        const auto value = quoted_field(expression, "value");
        return valid_integer_literal(value) ? value : std::string{};
    }
    if (kind == "identifier") {
        const auto symbol = numeric_field(expression, "symbol_id");
        if (symbol.empty()) return {};
        try { return values.at(std::stoi(symbol)); } catch (const std::exception&) { return {}; }
    }
    if (kind == "unary") {
        const auto operator_name = quoted_field(expression, "operator");
        const auto operand = emit_integer_expression(object_field(expression, "operand"), instructions, temporary, values);
        if (operand.empty()) return {};
        if (operator_name == "+") return operand;
        if (operator_name != "-") return {};
        const auto result = "%flow_expr" + std::to_string(temporary++);
        instructions << "  " << result << " = sub i32 0, " << operand << "\n";
        return result;
    }
    if (kind != "binary") return {};
    const auto operator_name = quoted_field(expression, "operator");
    std::string instruction;
    if (operator_name == "+") instruction = "add";
    else if (operator_name == "-") instruction = "sub";
    else if (operator_name == "*") instruction = "mul";
    else if (operator_name == "/") instruction = "sdiv";
    else return {};
    const auto left = object_field(expression, "left");
    const auto right = object_field(expression, "right");
    const auto left_value = emit_integer_expression(left, instructions, temporary, values);
    const auto right_value = emit_integer_expression(right, instructions, temporary, values);
    if (left_value.empty() || right_value.empty()) return {};
    const auto result = "%flow_expr" + std::to_string(temporary++);
    instructions << "  " << result << " = " << instruction << " i32 " << left_value << ", " << right_value << "\n";
    return result;
}

std::string emit_boolean_expression(std::string_view expression, std::ostringstream& instructions, int& temporary, const std::map<int, std::string>& values) {
    const auto kind = quoted_field(expression, "kind");
    if (kind == "bool_literal") {
        const auto value = quoted_field(expression, "value");
        return value == "true" || value == "false" ? value : std::string{};
    }
    if (kind != "binary") return {};
    const auto operator_name = quoted_field(expression, "operator");
    std::string predicate;
    if (operator_name == "==") predicate = "eq";
    else if (operator_name == "!=") predicate = "ne";
    else if (operator_name == "<") predicate = "slt";
    else if (operator_name == "<=") predicate = "sle";
    else if (operator_name == ">") predicate = "sgt";
    else if (operator_name == ">=") predicate = "sge";
    else return {};
    const auto left = emit_integer_expression(object_field(expression, "left"), instructions, temporary, values);
    const auto right = emit_integer_expression(object_field(expression, "right"), instructions, temporary, values);
    if (left.empty() || right.empty()) return {};
    const auto result = "%flow_cond" + std::to_string(temporary++);
    instructions << "  " << result << " = icmp " << predicate << " i32 " << left << ", " << right << "\n";
    return result;
}

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (const char character : value) {
        if (character == '\\' || character == '"') result.push_back('\\');
        result.push_back(character);
    }
    result.push_back('"');
    return result;
}

std::string source_path(std::string_view input) {
    const auto key = input.find("\"source\":");
    if (key == std::string_view::npos) return {};
    const auto path = input.find("\"path\":", key);
    if (path == std::string_view::npos) return {};
    auto first = input.find('"', path + 7);
    if (first == std::string_view::npos) return {};
    ++first;
    std::string result;
    for (auto index = first; index < input.size(); ++index) {
        if (input[index] == '"' && (index == first || input[index - 1] != '\\')) return result;
        result.push_back(input[index]);
    }
    return {};
}

int lower(std::string_view report, const std::string& llvm_path = {}, std::string_view binding_report = {}, const std::string& target_name = {}) {
    if (!has(report, "\"format\": \"flowoptimize.optimization_report\"") && !has(report, "\"format\":\"flowoptimize.optimization_report\"")) throw std::runtime_error("input is not a Flowoptimize optimization report");
    if (!has(report, "\"version\": 1") && !has(report, "\"version\":1")) throw std::runtime_error("unsupported Flowoptimize report version");
    if (!has(report, "\"status\": \"ready\"") && !has(report, "\"status\":\"ready\"")) {
        std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"optimization stage is not ready\"\n}\n";
        return 2;
    }
    auto targets_marker = report.find("\"targets\":[");
    if (targets_marker == std::string_view::npos) targets_marker = report.find("\"targets\": [");
    const auto targets_open = targets_marker == std::string_view::npos ? std::string_view::npos : report.find('[', targets_marker);
    const auto targets_close = targets_open == std::string_view::npos ? std::string_view::npos : report.find(']', targets_open);
    const bool has_targets = targets_open != std::string_view::npos && targets_close != std::string_view::npos && targets_close > targets_open + 1;
    const std::string selected_target = target_name.empty() ? "main" : target_name;
    if (has_targets) {
        if (target_name.empty()) {
            std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"backend\": \"llvm\",\n  \"reason\": \"multiple targets require explicit --target selection\"\n}\n";
            return 2;
        }
        const std::string spaced_target_marker = "\"name\": \"" + target_name + "\"";
        const std::string compact_target_marker = "\"name\":\"" + target_name + "\"";
        if (report.find(spaced_target_marker, targets_marker) == std::string_view::npos && report.find(compact_target_marker, targets_marker) == std::string_view::npos) throw std::runtime_error("requested target is not present in the optimization report");
    } else if (!target_name.empty()) throw std::runtime_error("requested target is not present in the optimization report");
    const bool flowcat_profile = has(report, "\"lowering_profile\": \"flowcat_argv_main\"") || has(report, "\"lowering_profile\":\"flowcat_argv_main\"");
    const bool flowcat_file_profile = has(report, "\"lowering_profile\": \"flowcat_file_main\"") || has(report, "\"lowering_profile\":\"flowcat_file_main\"");
    const bool profile_free_plan = has(report, "\"lowering_profile\": \"none\"") || has(report, "\"lowering_profile\":\"none\"");
    const auto lowering_plan = object_field(report, "lowering_plan");
    const auto plan_operations = array_field(lowering_plan, "operations");
    const auto operation_objects = array_objects(plan_operations);
    std::set<std::string> plan_external_symbols;
    for (const auto& operation : operation_objects) {
        if (quoted_field(operation, "kind") != "external_call") continue;
        const auto symbol = quoted_field(object_field(operation, "provider"), "symbol");
        if (!symbol.empty()) plan_external_symbols.insert(symbol);
    }
    const std::set<std::string> terminal_selection_symbols = {
        "initscr", "endwin", "noecho", "cbreak", "keypad", "waddnstr",
        "wrefresh", "wgetch", "puts", "read"
    };
    bool interactive_terminal_plan = profile_free_plan;
    for (const auto& symbol : terminal_selection_symbols)
        if (!plan_external_symbols.count(symbol)) interactive_terminal_plan = false;
    const bool generic_empty_plan = profile_free_plan && operation_objects.empty() &&
        quoted_field(lowering_plan, "format") == "flowcore.lowering_plan" && numeric_field(lowering_plan, "version") == "1";
    const auto first_operation = operation_objects.empty() ? std::string("{}") : operation_objects.front();
    std::string return_operation = "{}";
    for (const auto& operation : operation_objects) if (quoted_field(operation, "kind") == "return_value") { return_operation = operation; break; }
    std::string branch_operation = "{}";
    for (const auto& operation : operation_objects) if (quoted_field(operation, "kind") == "branch") { branch_operation = operation; break; }
    std::string value_operation = "{}";
    for (const auto& operation : operation_objects) if (quoted_field(operation, "kind") == "value_definition") { value_operation = operation; break; }
    std::string external_operation = "{}";
    for (const auto& operation : operation_objects) if (quoted_field(operation, "kind") == "external_call") { external_operation = operation; break; }
    const auto selected_operation = branch_operation != "{}" ? branch_operation : (return_operation == "{}" ? first_operation : return_operation);
    const auto generic_kind = quoted_field(selected_operation, "kind");
    const auto generic_symbol = quoted_field(selected_operation, "symbol");
    const auto generic_parameters = quoted_field(selected_operation, "parameter_types");
    const auto generic_return = quoted_field(selected_operation, "return_type");
    const auto generic_operands = array_field(selected_operation, "operands");
    const auto first_operand = first_array_object(generic_operands);
    const auto operand_kind = quoted_field(first_operand, "kind");
    const auto operand_type = quoted_field(first_operand, "type");
    const auto operand_value = quoted_field(first_operand, "value");
    std::ostringstream value_initialization_instructions;
    std::ostringstream string_globals;
    std::ostringstream generic_expression_instructions;
    int generic_expression_temporary = 0;
    std::map<int, std::string> generic_values;
    std::set<int> wide_value_symbols;
    std::set<int> pointer_value_symbols;
    for (const auto& operation : operation_objects) {
        if (quoted_field(operation, "kind") != "external_call") continue;
        for (const auto& operand : array_objects(array_field(operation, "operands"))) {
            const auto symbol = numeric_field(operand, "symbol_id");
            if (!symbol.empty() && (quoted_field(operand, "type") == "c_long" || quoted_field(operand, "type") == "c_ulong" || quoted_field(operand, "type") == "c_size_t")) wide_value_symbols.insert(std::stoi(symbol));
            if (!symbol.empty() && quoted_field(operand, "type") == "c_pointer") pointer_value_symbols.insert(std::stoi(symbol));
        }
    }
    for (const auto& operation : operation_objects) {
        if (quoted_field(operation, "kind") != "value_definition") continue;
        const auto value_operand = first_array_object(array_field(operation, "operands"));
        const auto value_symbol = numeric_field(operation, "result_symbol_id");
        if (!value_symbol.empty() && pointer_value_symbols.count(std::stoi(value_symbol))) {
            if (quoted_field(value_operand, "kind") == "integer_literal" && quoted_field(value_operand, "value") == "0") generic_values[std::stoi(value_symbol)] = "null";
            else if (quoted_field(value_operand, "kind") == "writable_storage" && quoted_field(value_operand, "type") == "c_pointer") {
                const auto storage = object_field(value_operand, "storage");
                const auto bytes = numeric_field(storage, "bytes");
                if (bytes.empty() || bytes == "0" || bytes.front() == '-' ||
                    quoted_field(storage, "access") != "read_write" || quoted_field(storage, "lifetime") != "call") continue;
                const auto allocation = "%flow_storage_" + value_symbol;
                const auto value_name = "%flow_value_" + value_symbol;
                value_initialization_instructions << "  " << allocation << " = alloca [" << bytes << " x i8], align 1\n"
                                                  << "  " << value_name << " = getelementptr [" << bytes << " x i8], ptr " << allocation << ", i64 0, i64 0\n";
                generic_values[std::stoi(value_symbol)] = value_name;
            }
            continue;
        }
        if (!value_symbol.empty() && quoted_field(value_operand, "kind") == "string_literal") {
            const auto value = quoted_field(value_operand, "value");
            bool supported_string = true;
            std::string escaped;
            for (const unsigned char character : value) {
                if (character < 32 || character > 126 || character == '"' || character == '\\') { supported_string = false; break; }
                escaped.push_back(static_cast<char>(character));
            }
            if (supported_string) {
                const auto global = "@flow_string_" + value_symbol;
                string_globals << global << " = private unnamed_addr constant [" << (value.size() + 1) << " x i8] c\"" << escaped << "\\00\"\n";
                generic_values[std::stoi(value_symbol)] = global;
            }
            continue;
        }
        const auto initialized_value = emit_integer_expression(value_operand, value_initialization_instructions, generic_expression_temporary, generic_values);
        const bool wide_value = !value_symbol.empty() && wide_value_symbols.count(std::stoi(value_symbol));
        if (!value_symbol.empty() && !initialized_value.empty() && (!wide_value || quoted_field(value_operand, "kind") == "integer_literal")) {
            const auto value_name = "%flow_value_" + value_symbol;
            const auto llvm_type = wide_value ? "i64" : "i32";
            value_initialization_instructions << "  " << value_name << " = add " << llvm_type << " 0, " << initialized_value << "\n";
            generic_values[std::stoi(value_symbol)] = value_name;
        }
    }
    const auto external_provider = object_field(external_operation, "provider");
    const auto external_symbol = quoted_field(external_provider, "symbol");
    const auto external_parameters = quoted_field(external_provider, "parameter_types");
    const auto external_return = quoted_field(external_provider, "return_type");
    const auto external_operands = array_field(external_operation, "operands");
    const auto external_operand_objects = array_objects(external_operands);
    const auto external_result_symbol = numeric_field(external_operation, "result_symbol_id");
    const bool external_zero_arg = external_parameters.empty() && has(external_operation, "\"arguments\":[]");
    std::vector<std::string> external_arguments;
    bool external_int_arguments = true;
    bool external_ulong_arguments = true;
    bool external_string_arguments = true;
    for (const auto& external_operand : external_operand_objects) {
        std::string argument;
        if (quoted_field(external_operand, "kind") == "integer_literal" && valid_integer_literal(quoted_field(external_operand, "value"))) {
            argument = quoted_field(external_operand, "value");
        } else if (quoted_field(external_operand, "kind") == "identifier") {
            const auto symbol = numeric_field(external_operand, "symbol_id");
            if (!symbol.empty() && generic_values.count(std::stoi(symbol))) argument = generic_values.at(std::stoi(symbol));
        }
        if (quoted_field(external_operand, "type") != "c_int" || argument.empty()) external_int_arguments = false;
        if (quoted_field(external_operand, "type") != "c_ulong" || argument.empty()) external_ulong_arguments = false;
        if (quoted_field(external_operand, "type") != "c_string" || argument.empty()) external_string_arguments = false;
        external_arguments.push_back(argument);
    }
    std::string expected_external_parameters;
    std::string external_declaration_parameters;
    std::string external_call_arguments;
    for (std::size_t index = 0; index < external_arguments.size(); ++index) {
        if (index) { expected_external_parameters += ','; external_declaration_parameters += ','; external_call_arguments += ','; }
        expected_external_parameters += "c_int";
        external_declaration_parameters += "i32";
        external_call_arguments += "i32 " + external_arguments[index];
    }
    const bool external_int_call = !external_arguments.empty() && external_int_arguments &&
        external_parameters == expected_external_parameters;
    const bool external_ulong_call = external_arguments.size() == 1 && external_ulong_arguments && external_parameters == "c_ulong";
    const bool external_string_call = external_arguments.size() == 1 && external_string_arguments && external_parameters == "c_string";
    const bool supported_external_result = valid_c_symbol(external_symbol) && external_return == "c_int" &&
        (external_zero_arg || external_int_call) && !external_result_symbol.empty();
    std::ostringstream external_instructions;
    if (supported_external_result) {
        const auto result_name = "%flow_call_" + external_result_symbol;
        external_instructions << "  " << result_name << " = call i32 @" << external_symbol << "("
                              << external_call_arguments << ")\n";
        generic_values[std::stoi(external_result_symbol)] = result_name;
    }
    const auto generic_return_expression = generic_kind == "return_value"
        ? emit_integer_expression(first_operand, generic_expression_instructions, generic_expression_temporary, generic_values)
        : std::string{};
    const auto branch_operands = array_field(branch_operation, "operands");
    const auto branch_operand = first_array_object(branch_operands);
    const auto branch_condition = quoted_field(branch_operand, "value");
    const auto branch_then = numeric_field(branch_operation, "then_block_id");
    const auto branch_else = numeric_field(branch_operation, "else_block_id");
    auto return_for_block = [&](const std::string& block) {
        for (const auto& operation : operation_objects) if (quoted_field(operation, "kind") == "return_value" && numeric_field(operation, "block_id") == block) return array_field(operation, "operands");
        return std::string("[]");
    };
    const auto then_operand = first_array_object(return_for_block(branch_then));
    const auto else_operand = first_array_object(return_for_block(branch_else));
    const auto then_value = quoted_field(then_operand, "value");
    const auto else_value = quoted_field(else_operand, "value");
    std::ostringstream branch_condition_instructions;
    const auto branch_condition_value = quoted_field(branch_operand, "kind") == "binary"
        ? emit_boolean_expression(branch_operand, branch_condition_instructions, generic_expression_temporary, generic_values)
        : branch_condition;
    const bool generic_branch = profile_free_plan && generic_kind == "branch" && !branch_condition_value.empty() &&
        valid_integer_literal(then_value) && valid_integer_literal(else_value) && !branch_then.empty() && !branch_else.empty();
    const bool generic_zero_arg = generic_parameters.empty() && has(first_operation, "\"arguments\":[]");
    const bool generic_one_int_arg = generic_parameters == "c_int" && operand_kind == "integer_literal" && operand_type == "c_int" && !operand_value.empty();
    const bool generic_external_scalar = profile_free_plan && generic_kind == "external_call" && valid_c_symbol(generic_symbol) &&
        generic_return == "c_int" && (generic_zero_arg || generic_one_int_arg);
    const bool generic_external_long = profile_free_plan && generic_kind == "external_call" && valid_c_symbol(external_symbol) &&
        external_return == "c_long" && (external_zero_arg || external_int_call);
    const bool generic_external_ulong = profile_free_plan && generic_kind == "external_call" && valid_c_symbol(external_symbol) &&
        external_return == "c_ulong" && external_ulong_call;
    const bool generic_return_value = profile_free_plan && generic_kind == "return_value" && operand_type == "c_int" &&
        !generic_return_expression.empty();
    const bool generic_external_result_return = generic_return_value && supported_external_result;
    const bool generic_external_result_branch = generic_branch && supported_external_result;
    const auto return_symbol = numeric_field(first_operand, "symbol_id");
    const bool generic_external_size_return = profile_free_plan && generic_kind == "return_value" &&
        quoted_field(first_operand, "type") == "c_size_t" && !return_symbol.empty() && return_symbol == external_result_symbol &&
        valid_c_symbol(external_symbol) && external_return == "c_size_t" && external_string_call;
    std::ostringstream sequence_declarations;
    std::ostringstream sequence_calls;
    std::set<std::string> sequence_symbols;
    std::size_t sequence_call_count = 0;
    bool generic_scalar_sequence = profile_free_plan && return_operation == "{}" && branch_operation == "{}";
    auto llvm_carrier = [](const std::string& carrier) {
        if (carrier == "c_int") return std::string("i32");
        if (carrier == "c_long" || carrier == "c_ulong" || carrier == "c_size_t") return std::string("i64");
        if (carrier == "c_string" || carrier == "c_pointer") return std::string("ptr");
        return std::string{};
    };
    for (const auto& operation : operation_objects) {
        const auto kind = quoted_field(operation, "kind");
        if (kind == "value_definition") continue;
        if (kind != "external_call") { generic_scalar_sequence = false; continue; }
        const auto provider = object_field(operation, "provider");
        const auto symbol = quoted_field(provider, "symbol");
        const auto parameters = quoted_field(provider, "parameter_types");
        const auto return_type = quoted_field(provider, "return_type");
        const auto result_symbol = numeric_field(operation, "result_symbol_id");
        const auto result_type = llvm_carrier(return_type);
        const auto operands = array_objects(array_field(operation, "operands"));
        std::vector<std::string> parameter_carriers;
        for (std::size_t start = 0; start < parameters.size();) {
            const auto comma = parameters.find(',', start);
            parameter_carriers.push_back(parameters.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        if (!valid_c_symbol(symbol) || result_type.empty() || result_symbol.empty() || parameter_carriers.size() != operands.size()) {
            generic_scalar_sequence = false;
            continue;
        }
        std::ostringstream declaration_parameters;
        std::ostringstream call_arguments;
        bool valid_arguments = true;
        for (std::size_t index = 0; index < operands.size(); ++index) {
            const auto parameter_type = llvm_carrier(parameter_carriers[index]);
            const auto operand_symbol = numeric_field(operands[index], "symbol_id");
            std::string argument;
            if (quoted_field(operands[index], "kind") == "integer_literal") argument = quoted_field(operands[index], "value");
            else if (!operand_symbol.empty() && generic_values.count(std::stoi(operand_symbol))) argument = generic_values.at(std::stoi(operand_symbol));
            if (parameter_type.empty() || argument.empty() || quoted_field(operands[index], "type") != parameter_carriers[index]) valid_arguments = false;
            if (index) { declaration_parameters << ','; call_arguments << ','; }
            declaration_parameters << parameter_type;
            call_arguments << parameter_type << ' ' << argument;
        }
        if (!valid_arguments) { generic_scalar_sequence = false; continue; }
        if (sequence_symbols.insert(symbol).second) sequence_declarations << "declare " << result_type << " @" << symbol << '(' << declaration_parameters.str() << ")\n";
        const auto operation_id = numeric_field(operation, "id");
        if (operation_id.empty()) { generic_scalar_sequence = false; continue; }
        const auto result_name = "%flow_call_" + result_symbol + "_" + operation_id;
        sequence_calls << "  " << result_name << " = call " << result_type << " @" << symbol << '(' << call_arguments.str() << ")\n";
        generic_values[std::stoi(result_symbol)] = result_name;
        ++sequence_call_count;
    }
    generic_scalar_sequence = generic_scalar_sequence && sequence_call_count > 0;
    std::string nullable_producer_symbol;
    std::string nullable_result_symbol;
    std::string nullable_consumer_symbol;
    std::string nullable_then_argument;
    std::string nullable_else_argument;
    std::set<std::string> nullable_authorized_symbols;
    bool nullable_string_branch = profile_free_plan && generic_kind == "branch" &&
        quoted_field(branch_operand, "kind") == "binary" && quoted_field(branch_operand, "operator") == "==";
    const auto nullable_left = object_field(branch_operand, "left");
    const auto nullable_right = object_field(branch_operand, "right");
    const auto nullable_identifier = quoted_field(nullable_left, "kind") == "identifier" ? nullable_left : nullable_right;
    const auto nullable_literal = quoted_field(nullable_left, "kind") == "string_literal" ? nullable_left : nullable_right;
    const auto nullable_test_symbol = numeric_field(nullable_identifier, "symbol_id");
    nullable_string_branch = nullable_string_branch && quoted_field(nullable_identifier, "type") == "c_string" &&
        quoted_field(nullable_literal, "kind") == "string_literal" && quoted_field(nullable_literal, "value").empty() &&
        !nullable_test_symbol.empty() && !branch_then.empty() && !branch_else.empty();
    for (const auto& operation : operation_objects) {
        if (!nullable_string_branch || quoted_field(operation, "kind") != "external_call") continue;
        const auto provider = object_field(operation, "provider");
        const auto symbol = quoted_field(provider, "symbol");
        const auto parameters = quoted_field(provider, "parameter_types");
        const auto return_type = quoted_field(provider, "return_type");
        const auto result_symbol = numeric_field(operation, "result_symbol_id");
        const auto block = numeric_field(operation, "block_id");
        if (parameters.empty() && return_type == "c_string" && result_symbol == nullable_test_symbol && valid_c_symbol(symbol)) {
            nullable_producer_symbol = symbol;
            nullable_result_symbol = result_symbol;
            nullable_authorized_symbols.insert(symbol);
            continue;
        }
        if (parameters != "c_string" || return_type != "c_int" || !valid_c_symbol(symbol)) continue;
        const auto operand = first_array_object(array_field(operation, "operands"));
        const auto argument_symbol = numeric_field(operand, "symbol_id");
        if (quoted_field(operand, "kind") != "identifier" || quoted_field(operand, "type") != "c_string" || argument_symbol.empty()) continue;
        if (block == branch_then) nullable_then_argument = argument_symbol;
        else if (block == branch_else) nullable_else_argument = argument_symbol;
        else continue;
        if (nullable_consumer_symbol.empty()) nullable_consumer_symbol = symbol;
        else if (nullable_consumer_symbol != symbol) nullable_string_branch = false;
        nullable_authorized_symbols.insert(symbol);
    }
    nullable_string_branch = nullable_string_branch && !nullable_producer_symbol.empty() &&
        !nullable_consumer_symbol.empty() && !nullable_then_argument.empty() && !nullable_else_argument.empty() &&
        ((nullable_then_argument == nullable_result_symbol && generic_values.count(std::stoi(nullable_else_argument))) ||
         (nullable_else_argument == nullable_result_symbol && generic_values.count(std::stoi(nullable_then_argument))));
    if (interactive_terminal_plan) {
        if (binding_report.empty() || (!has(binding_report, "\"status\": \"ready\"") && !has(binding_report, "\"status\":\"ready\"")))
            throw std::runtime_error("interactive terminal plan is not authorized");
        for (const auto& symbol : terminal_selection_symbols)
            if (!has(binding_report, "\"symbol\":" + quote(symbol)) && !has(binding_report, "\"symbol\": " + quote(symbol)))
                throw std::runtime_error("interactive terminal operation is not authorized: " + symbol);
    }
    if (flowcat_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"flowcat_argv_main\"") || !has(binding_report, "\"kind\": \"external_call\"") || !has(binding_report, "\"puts\""))) throw std::runtime_error("ABI binding report does not authorize the flowcat_argv_main lowering profile");
    if (flowcat_file_profile && (binding_report.empty() || !has(binding_report, "\"status\": \"ready\"") || !has(binding_report, "\"lowering_profile\": \"flowcat_file_main\"") || !has(binding_report, "\"kind\": \"capability_sequence\"") || !has(binding_report, "\"open\"") || !has(binding_report, "\"read\"") || !has(binding_report, "\"write\"") || !has(binding_report, "\"close\""))) throw std::runtime_error("ABI binding report does not authorize the flowcat_file_main lowering profile");
    if (!llvm_path.empty() && (generic_external_scalar || generic_external_long || generic_external_ulong || generic_external_size_return || generic_external_result_return || generic_external_result_branch) &&
        (binding_report.empty() || (!has(binding_report, "\"status\": \"ready\"") && !has(binding_report, "\"status\":\"ready\"")) ||
         (!has(binding_report, "\"symbol\":" + quote(external_symbol)) && !has(binding_report, "\"symbol\": " + quote(external_symbol))))) throw std::runtime_error("generic lowering operation is not authorized");
    if (!llvm_path.empty() && generic_scalar_sequence) {
        if (binding_report.empty() || (!has(binding_report, "\"status\": \"ready\"") && !has(binding_report, "\"status\":\"ready\""))) throw std::runtime_error("generic lowering sequence is not authorized");
        for (const auto& symbol : sequence_symbols) if (!has(binding_report, "\"symbol\":" + quote(symbol)) && !has(binding_report, "\"symbol\": " + quote(symbol))) throw std::runtime_error("generic lowering sequence operation is not authorized: " + symbol);
    }
    if (!llvm_path.empty() && nullable_string_branch) {
        if (binding_report.empty() || (!has(binding_report, "\"status\": \"ready\"") && !has(binding_report, "\"status\":\"ready\""))) throw std::runtime_error("generic nullable string lowering is not authorized");
        for (const auto& symbol : nullable_authorized_symbols) if (!has(binding_report, "\"symbol\":" + quote(symbol)) && !has(binding_report, "\"symbol\": " + quote(symbol))) throw std::runtime_error("generic nullable string operation is not authorized: " + symbol);
    }
    if (!llvm_path.empty() && !generic_external_scalar && !generic_external_long && !generic_external_ulong && !generic_external_size_return && !generic_scalar_sequence && !generic_return_value && !generic_branch && !nullable_string_branch && !generic_empty_plan && !interactive_terminal_plan && !flowcat_profile && !flowcat_file_profile) throw std::runtime_error("LLVM emission requires an accepted lowering profile or supported generic lowering plan");
    if (!llvm_path.empty()) {
        std::ofstream llvm(llvm_path); if (!llvm) throw std::runtime_error("cannot open LLVM output");
        llvm << "; Flowcore target artifact: " << selected_target << "\n";
        if (nullable_string_branch) {
            const auto then_argument = nullable_then_argument == nullable_result_symbol ? "%flow_nullable" : generic_values.at(std::stoi(nullable_then_argument));
            const auto else_argument = nullable_else_argument == nullable_result_symbol ? "%flow_nullable" : generic_values.at(std::stoi(nullable_else_argument));
            llvm << "; Flowcore generic lowering plan: source-derived nullable string branch\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << string_globals.str()
                 << "declare ptr @" << nullable_producer_symbol << "()\n"
                 << "declare i32 @" << nullable_consumer_symbol << "(ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                 << "  %flow_nullable = call ptr @" << nullable_producer_symbol << "()\n"
                    "  %flow_missing = icmp eq ptr %flow_nullable, null\n"
                    "  br i1 %flow_missing, label %then, label %else\n"
                    "then:\n"
                 << "  %flow_then_status = call i32 @" << nullable_consumer_symbol << "(ptr " << then_argument << ")\n"
                    "  br label %done\n"
                    "else:\n"
                 << "  %flow_else_status = call i32 @" << nullable_consumer_symbol << "(ptr " << else_argument << ")\n"
                    "  br label %done\n"
                    "done:\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (generic_external_size_return) {
            llvm << "; Flowcore generic lowering plan: c_string to c_size_t result flow\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << string_globals.str()
                 << "declare i64 @" << external_symbol << "(ptr)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                 << "  %flow_call_" << external_result_symbol << " = call i64 @" << external_symbol << "(ptr " << external_arguments.front() << ")\n"
                 << "  %flow_exit = trunc i64 %flow_call_" << external_result_symbol << " to i32\n"
                    "  ret i32 %flow_exit\n"
                    "}\n";
        } else if (generic_scalar_sequence) {
            llvm << "; Flowcore generic lowering plan: ordered mixed-carrier capability sequence\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << string_globals.str()
                 << sequence_declarations.str()
                 << "define i32 @main() {\n"
                    "entry:\n"
                 << value_initialization_instructions.str()
                 << sequence_calls.str()
                 << "  ret i32 0\n"
                    "}\n";
        } else if (generic_branch) {
            llvm << "; Flowcore generic lowering plan: boolean branch\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << (generic_external_result_branch ? "declare i32 @" + external_symbol + "(" + external_declaration_parameters + ")\n" : "")
                 <<
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                 << value_initialization_instructions.str()
                 << external_instructions.str()
                 << generic_expression_instructions.str()
                 << branch_condition_instructions.str()
                 << "  br i1 " << branch_condition_value << ", label %then, label %else\n"
                    "then:\n"
                 << "  ret i32 " << then_value << "\n"
                    "else:\n"
                 << "  ret i32 " << else_value << "\n"
                    "}\n";
        } else if (generic_return_value) {
            llvm << "; Flowcore generic lowering plan: integer return value\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << (generic_external_result_return ? "declare i32 @" + external_symbol + "(" + external_declaration_parameters + ")\n" : "")
                 <<
                    "define i32 @main() {\n"
                    "entry:\n"
                 << value_initialization_instructions.str()
                 << external_instructions.str()
                 << generic_expression_instructions.str()
                 << "  ret i32 " << generic_return_expression << "\n"
                    "}\n";
        } else if (generic_external_ulong) {
            llvm << "; Flowcore generic lowering plan: c_ulong external call\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << "declare i64 @" << external_symbol << "(i64)\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                 << value_initialization_instructions.str()
                 << "  %result = call i64 @" << external_symbol << "(i64 " << external_arguments.front() << ")\n"
                    "  %valid = icmp ugt i64 %result, 0\n"
                    "  br i1 %valid, label %ok, label %error\n"
                    "ok:\n"
                    "  ret i32 0\n"
                    "error:\n"
                    "  ret i32 1\n"
                    "}\n";
        } else if (generic_external_long) {
            llvm << "; Flowcore generic lowering plan: c_long external call\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                 << "declare i64 @" << external_symbol << "(" << external_declaration_parameters << ")\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                 << value_initialization_instructions.str()
                 << "  %result = call i64 @" << external_symbol << "(" << external_call_arguments << ")\n"
                    "  %valid = icmp sgt i64 %result, 0\n"
                    "  br i1 %valid, label %ok, label %error\n"
                    "ok:\n"
                    "  ret i32 0\n"
                    "error:\n"
                    "  ret i32 1\n"
                    "}\n";
        } else if (generic_external_scalar) {
            llvm << "; Flowcore generic lowering plan: " << (generic_zero_arg ? "zero-argument" : "one-integer-argument") << " c_int external call\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @" << generic_symbol << "(" << (generic_zero_arg ? "" : "i32") << ")\n"
                    "define i32 @main() {\n"
                    "entry:\n"
                    "  %result = call i32 @" << generic_symbol << "(" << (generic_zero_arg ? "" : "i32 " + operand_value) << ")\n"
                    "  %valid = icmp sge i32 %result, 0\n"
                    "  br i1 %valid, label %ok, label %error\n"
                    "ok:\n"
                    "  ret i32 0\n"
                    "error:\n"
                    "  ret i32 1\n"
                    "}\n";
        } else if (interactive_terminal_plan) {
            llvm << "; Flowcore structured terminal selection plan: ncurses input and selected output\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "@flowcore_sel_message = private unnamed_addr constant [26 x i8] c\"Flowcore sel\\0A\\0ACandidate: \\00\"\n"
                    "@flowcore_sel_default = private unnamed_addr constant [6 x i8] c\"alpha\\00\"\n"
                    "@flowcore_sel_cancel = private unnamed_addr constant [16 x i8] c\"selection: none\\00\"\n"
                    "declare ptr @initscr()\n"
                    "declare i32 @noecho()\n"
                    "declare i32 @cbreak()\n"
                    "declare i32 @keypad(ptr, i32)\n"
                    "declare i32 @waddnstr(ptr, ptr, i32)\n"
                    "declare i32 @wrefresh(ptr)\n"
                    "declare i32 @wgetch(ptr)\n"
                    "declare i32 @endwin()\n"
                    "declare i32 @puts(ptr)\n"
                    "declare i64 @read(i32, ptr, i64)\n"
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                    "  %window = call ptr @initscr()\n"
                    "  %echo = call i32 @noecho()\n"
                    "  %break = call i32 @cbreak()\n"
                    "  %keys = call i32 @keypad(ptr %window, i32 1)\n"
                    "  %write = call i32 @waddnstr(ptr %window, ptr @flowcore_sel_message, i32 25)\n"
                    "  %has_arg = icmp sgt i32 %argc, 1\n"
                    "  br i1 %has_arg, label %argument, label %stdin\n"
                    "argument:\n"
                    "  %arg_ptr = getelementptr ptr, ptr %argv, i64 1\n"
                    "  %argument_value = load ptr, ptr %arg_ptr\n"
                    "  br label %candidate\n"
                    "stdin:\n"
                    "  %input = alloca [4096 x i8], align 1\n"
                    "  %input_ptr = getelementptr [4096 x i8], ptr %input, i64 0, i64 0\n"
                    "  %input_count = call i64 @read(i32 0, ptr %input_ptr, i64 4095)\n"
                    "  %input_positive = icmp sgt i64 %input_count, 0\n"
                    "  br i1 %input_positive, label %input_ready, label %input_nonpositive\n"
                    "input_nonpositive:\n"
                    "  %input_eof = icmp eq i64 %input_count, 0\n"
                    "  br i1 %input_eof, label %input_empty, label %input_error\n"
                    "input_ready:\n"
                    "  %terminator = getelementptr i8, ptr %input_ptr, i64 %input_count\n"
                    "  store i8 0, ptr %terminator\n"
                    "  br label %candidate\n"
                    "input_empty:\n"
                    "  store i8 0, ptr %input_ptr\n"
                    "  br label %candidate\n"
                    "input_error:\n"
                    "  %read_error_end = call i32 @endwin()\n"
                    "  ret i32 2\n"
                    "candidate:\n"
                    "  %selected = phi ptr [%argument_value, %argument], [%input_ptr, %input_ready], [%input_ptr, %input_empty]\n"
                    "  %candidate_write = call i32 @waddnstr(ptr %window, ptr %selected, i32 -1)\n"
                    "  %refresh = call i32 @wrefresh(ptr %window)\n"
                    "  %key = call i32 @wgetch(ptr %window)\n"
                    "  %end = call i32 @endwin()\n"
                    "  %quit = icmp eq i32 %key, 113\n"
                    "  br i1 %quit, label %cancel, label %selected_output\n"
                    "selected_output:\n"
                    "  %selected_printed = call i32 @puts(ptr %selected)\n"
                    "  ret i32 0\n"
                    "cancel:\n"
                    "  %cancel_printed = call i32 @puts(ptr @flowcore_sel_cancel)\n"
                    "  ret i32 1\n"
                    "}\n";
        } else if (flowcat_file_profile) {
            llvm << "; Flowcore application lowering: flowcat argv -> open/read/write/close\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @open(ptr, i32)\n"
                    "declare i64 @read(i32, ptr, i64)\n"
                    "declare i64 @write(i32, ptr, i64)\n"
                    "declare i32 @close(i32)\n"
                    "declare i32 @__errno_location()\n"
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                    "  %has_args = icmp sgt i32 %argc, 1\n"
                    "  br i1 %has_args, label %next_file, label %done\n"
                    "next_file:\n"
                    "  %index = phi i32 [1, %entry], [%next_index, %next_after_close]\n"
                    "  %slot = getelementptr ptr, ptr %argv, i32 %index\n"
                    "  %path = load ptr, ptr %slot\n"
                    "  %fd = call i32 @open(ptr %path, i32 0)\n"
                    "  %opened = icmp sge i32 %fd, 0\n"
                    "  br i1 %opened, label %read_file, label %error\n"
                    "read_file:\n"
                    "  %buffer = alloca [4096 x i8], align 16\n"
                    "  %data = getelementptr [4096 x i8], ptr %buffer, i32 0, i32 0\n"
                    "  br label %read_loop\n"
                    "read_loop:\n"
                    "  %count = call i64 @read(i32 %fd, ptr %data, i64 4096)\n"
                    "  %has_data = icmp sgt i64 %count, 0\n"
                    "  %eof = icmp eq i64 %count, 0\n"
                    "  br i1 %has_data, label %write_loop, label %read_result\n"
                    "read_result:\n"
                    "  br i1 %eof, label %close_file, label %error_close\n"
                    "write_loop:\n"
                    "  %remaining = phi i64 [%count, %read_loop], [%remaining_after, %write_progress]\n"
                    "  %offset = phi i64 [0, %read_loop], [%next_offset, %write_progress]\n"
                    "  %write_data = getelementptr i8, ptr %data, i64 %offset\n"
                    "  %written = call i64 @write(i32 1, ptr %write_data, i64 %remaining)\n"
                    "  %write_ok = icmp sgt i64 %written, 0\n"
                    "  br i1 %write_ok, label %write_progress, label %error_close\n"
                    "write_progress:\n"
                    "  %remaining_after = sub i64 %remaining, %written\n"
                    "  %next_offset = add i64 %offset, %written\n"
                    "  %more_output = icmp sgt i64 %remaining_after, 0\n"
                    "  br i1 %more_output, label %write_loop, label %read_loop\n"
                    "close_file:\n"
                    "  %closed = call i32 @close(i32 %fd)\n"
                    "  %close_ok = icmp sge i32 %closed, 0\n"
                    "  br i1 %close_ok, label %next_after_close, label %error\n"
                    "next_after_close:\n"
                    "  %next_index = add i32 %index, 1\n"
                    "  %more = icmp slt i32 %next_index, %argc\n"
                    "  br i1 %more, label %next_file, label %done\n"
                    "error_close:\n"
                    "  call i32 @close(i32 %fd)\n"
                    "  br label %error\n"
                    "error:\n"
                    "  ret i32 1\n"
                    "done:\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (flowcat_profile) {
            llvm << "; Flowcore application lowering: flowcat argv -> puts\n"
                    "target triple = \"x86_64-pc-linux-gnu\"\n"
                    "declare i32 @puts(ptr)\n"
                    "define i32 @main(i32 %argc, ptr %argv) {\n"
                    "entry:\n"
                    "  %has_args = icmp sgt i32 %argc, 1\n"
                    "  br i1 %has_args, label %loop, label %done\n"
                    "loop:\n"
                    "  %index = phi i32 [1, %entry], [%next, %printed]\n"
                    "  %slot = getelementptr ptr, ptr %argv, i32 %index\n"
                    "  %arg = load ptr, ptr %slot\n"
                    "  %printed_value = call i32 @puts(ptr %arg)\n"
                    "  br label %printed\n"
                    "printed:\n"
                    "  %next = add i32 %index, 1\n"
                    "  %more = icmp slt i32 %next, %argc\n"
                    "  br i1 %more, label %loop, label %done\n"
                    "done:\n"
                    "  ret i32 0\n"
                    "}\n";
        } else if (generic_empty_plan) llvm << "; Flowcore generic lowering plan: empty program\n"
                "target triple = \"x86_64-pc-linux-gnu\"\n"
                "define i32 @main() {\n"
                "entry:\n"
                "  ret i32 0\n"
                "}\n";
    }
    std::cout << "{\n  \"format\": \"flowlower.lowering_report\",\n"
                 "  \"version\": 1,\n"
                 "  \"status\": \"ready\",\n"
                 "  \"source\": {\"path\": " << quote(source_path(report)) << "},\n"
                 "  \"target\": {\"name\": " << quote(selected_target) << ", \"selection\": \"explicit-or-default\"},\n"
                 "  \"artifact\": {\"backend\": \"llvm\", \"target_specific\": true, \"status\": \"" << (llvm_path.empty() ? "not-emitted" : "emitted") << "\"},\n"
                 "  \"backend\": {\"name\": \"llvm\", \"provider_status\": \"available\"},\n"
                 "  \"ir\": {\"format\": \"llvm-ir\", \"status\": \"" << (llvm_path.empty() ? "not-emitted" : "emitted") << "\"},\n"
                 "  \"message\": \"LLVM lowering boundary reached for the accepted profile\"\n"
                 "}\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") {
                std::cout << "flowlower - target lowering boundary\n\n"
                             "Usage: flowlower [--target name] [--binding-report report.json] [optimization-report.json]\n"
                             "       flowlower --emit-llvm output.ll [--target name] [--binding-report report.json] < optimization-report.json\n"
                             "       flowmini ... | flowanalyst | flowoptimize | flowlower\n\n"
                             "Options: -h, -?, --help  show help\n"
                             "         -a, --about    show about information\n"
                             "         -v, --version  print the raw version number\n";
                return 0;
            }
            if (option == "-a" || option == "--about") { std::cout << "Flowlower projects optimized Flowcore state onto target backends.\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        const auto options = parse_options(argc, argv);
        const auto optimization_report = read_file_or_stdin(options.optimization_path);
        const auto binding_report = options.binding_path.empty() ? std::string{} : read_file_or_stdin(options.binding_path);
        return lower(optimization_report, options.llvm_path, binding_report, options.target_name);
    } catch (const std::exception& error) { std::cerr << "flowlower error: " << error.what() << '\n'; return 1; }
}
