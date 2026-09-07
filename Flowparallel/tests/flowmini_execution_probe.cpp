#include "flowparallel/cpu_execution.hpp"
#include <flowcontracts/json.hpp>

#include <dlfcn.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Json = flowcontracts::json::Value;
using Object = flowcontracts::json::Object;
using Array = flowcontracts::json::Array;
const Json* field(const Json& value, std::string_view name) {
    const auto* object = std::get_if<Object>(&value);
    if (!object) return nullptr;
    const auto it = object->find(std::string(name));
    return it == object->end() ? nullptr : &it->second;
}
const Json* field(const Json* value, std::string_view name) { return value ? field(*value, name) : nullptr; }
std::string text(const Json* value) { return value && std::holds_alternative<std::string>(*value) ? std::get<std::string>(*value) : std::string{}; }
int number(const Json* value, std::string_view path) { return static_cast<int>(flowcontracts::json::integer(*value, path)); }
const Array& array(const Json* value, std::string_view path) { return flowcontracts::json::array(*value, path); }
Json read_file(const std::string& path) { std::ifstream input(path); if (!input) throw std::runtime_error("cannot open execution plan"); std::ostringstream text; text << input.rdbuf(); return flowcontracts::json::parse(text.str()); }
struct Call { std::string name; int expression = -1; int argument = 0; };
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: flowparallel_flowmini_probe plan.json lowered.so");
        const auto root = read_file(argv[1]);
        const auto candidates = array(field(root, "parallel_candidates"), "parallel_candidates");
        if (candidates.size() < 2) throw std::runtime_error("plan has fewer than two approved candidates");
        std::vector<int> candidate_expressions;
        for (std::size_t index = 0; index < 2; ++index)
            candidate_expressions.push_back(number(field(candidates[index], "call_expression"), "candidate.call_expression"));
        const auto* lowering_value = field(root, "lowering_plan");
        if (!lowering_value) throw std::runtime_error("plan has no lowering plan");
        std::map<int, int> values;
        std::vector<Call> calls;
        for (const auto& item : array(field(*lowering_value, "operations"), "lowering_plan.operations")) {
            const auto kind = text(field(item, "kind"));
            if (kind == "value_definition") {
                const auto* result = field(item, "result_symbol_id");
                const auto operands = array(field(item, "operands"), "operation.operands");
                if (result && !operands.empty() && text(field(operands.front(), "kind")) == "integer_literal")
                    values[number(result, "result_symbol_id")] = std::stoi(text(field(operands.front(), "value")));
            }
            if (kind != "call") continue;
            const int expression = number(field(item, "expression_id"), "operation.expression_id");
            if (std::find(candidate_expressions.begin(), candidate_expressions.end(), expression) == candidate_expressions.end()) continue;
            const auto operands = array(field(item, "operands"), "call.operands");
            if (operands.size() != 1) throw std::runtime_error("probe requires one scalar operand per candidate");
            int argument = 0;
            if (text(field(operands.front(), "kind")) == "integer_literal") argument = std::stoi(text(field(operands.front(), "value")));
            else {
                const int argument_symbol = number(field(operands.front(), "symbol_id"), "call.operand.symbol_id");
                if (!values.count(argument_symbol)) throw std::runtime_error("candidate input is not a deterministic integer value");
                argument = values.at(argument_symbol);
            }
            calls.push_back({text(field(item, "callee")), expression, argument});
        }
        if (calls.size() != 2) throw std::runtime_error("could not reconstruct two candidate calls from lowering plan");
        void* library = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
        if (!library) throw std::runtime_error(std::string("cannot load lowered Flowmini artifact: ") + dlerror());
        std::vector<int> serial_values(2), parallel_values(2);
        auto make_tasks = [&](std::vector<int>& output) {
            std::vector<flowparallel::cpu::Task> tasks;
            for (std::size_t index = 0; index < calls.size(); ++index) {
                using Function = int (*)(int);
                dlerror();
                auto function = reinterpret_cast<Function>(dlsym(library, calls[index].name.c_str()));
                const auto error = dlerror();
                if (error || !function) throw std::runtime_error("lowered candidate symbol is unavailable");
                tasks.push_back({[&, index, function] { output[index] = function(calls[index].argument); }});
            }
            return tasks;
        };
        const auto serial = flowparallel::cpu::execute_independent(make_tasks(serial_values), 1);
        const auto parallel = flowparallel::cpu::execute_independent(make_tasks(parallel_values), 2);
        dlclose(library);
        const bool matching = serial.status == "ok" && parallel.status == "ok" && serial_values == parallel_values;
        std::cout << "{\"format\":\"flowparallel.flowmini_execution\",\"version\":1,\"status\":\"" << (matching ? "ok" : "error")
                  << "\",\"source\":{\"path\":\"" << text(field(field(root, "source"), "path")) << "\"},\"provider_paths\":[\"cpu.serial\",\"cpu.threadpool\"],\"candidate_count\":2,\"serial_results\":[" << serial_values[0] << "," << serial_values[1] << "],\"parallel_results\":[" << parallel_values[0] << "," << parallel_values[1] << "],\"matching_observable_results\":" << (matching ? "true" : "false") << "}\n";
        return matching ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << "flowparallel_flowmini_probe error: " << error.what() << '\n'; return 2; }
}
