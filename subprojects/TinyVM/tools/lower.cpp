#include <flowcontracts/validate.hpp>
extern "C" {
#include <tinyvm/isa_v1.h>
}

#include <openssl/sha.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace flowcontracts;
using namespace flowcontracts::json;

std::string read(const char* path) {
    std::ostringstream input;
    if (std::strcmp(path, "-") == 0) input << std::cin.rdbuf();
    else { std::ifstream file(path); if (!file) throw std::runtime_error("cannot open backend lowering artifact"); input << file.rdbuf(); }
    return input.str();
}

std::string identity(std::string_view prefix, std::string_view meaning) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(meaning.data()), meaning.size(), digest);
    static constexpr char hex[] = "0123456789abcdef";
    std::string result(prefix);
    for (std::size_t index = 0; index < 16; ++index) { result.push_back(hex[digest[index] >> 4]); result.push_back(hex[digest[index] & 15]); }
    return result;
}

void copy(char output[64], const std::string& value) { std::snprintf(output, 64, "%s", value.c_str()); }

struct Unsupported : std::runtime_error { using std::runtime_error::runtime_error; };

class Compiler {
public:
    Compiler(const Object& root, std::string source, std::string derivation)
        : root_(root), source_(std::move(source)), derivation_(std::move(derivation)) {}

    void compile() {
        const auto& operations = required_array(required_object(root_, "lowering_plan"), "operations", "$.lowering_plan");
        scan_arguments(required(root_, "lowering_plan"));
        if (uses_arguments_) next_slot_ = required_argument_count_ + 1;
        for (const auto& value : operations) {
            const auto& operation = object(value, "$.lowering_plan.operations[]");
            const auto kind = string(required(operation, "kind", "$.lowering_plan.operations[]"), "$.lowering_plan.operations[].kind");
            if (kind == "call") continue;
            if (kind != "value_definition" && kind != "assignment" && kind != "return_value" && kind != "branch" && kind != "loop")
                throw Unsupported("operation kind '" + kind + "' is not admitted by the scalar slice");
            const auto block = optional(operation, "block_id") ? integer(*optional(operation, "block_id"), "$.operation.block_id") : 0;
            blocks_[block].push_back(&operation);
        }
        std::map<Integer, Integer> block_start;
        for (const auto& [block, items] : blocks_) for (const auto* operation : items) {
            const auto kind = string(required(*operation, "kind", "$.operation"), "$.operation.kind");
            if (kind == "loop") continue;
            const auto statement = integer(required(*operation, "statement_id", "$.operation"), "$.operation.statement_id");
            if (!block_start.contains(block) || statement < block_start[block]) block_start[block] = statement;
        }
        for (auto& [block, items] : blocks_) std::stable_sort(items.begin(), items.end(), [&](const auto* left, const auto* right) {
            auto order = [&](const Object* operation) {
                if (string(required(*operation, "kind", "$.operation"), "$.operation.kind") == "loop") {
                    const auto body = integer(required(*operation, "body_block_id", "$.operation"), "$.operation.body_block_id");
                    if (block_start.contains(body)) return block_start[body];
                }
                return integer(required(*operation, "statement_id", "$.operation"), "$.operation.statement_id");
            };
            const auto a = order(left), b = order(right);
            return a == b ? integer(required(*left, "id", "$.operation"), "$.operation.id") < integer(required(*right, "id", "$.operation"), "$.operation.id") : a < b;
        });
        if (required_argument_count_) emit_argument_guard();
        if (blocks_.empty()) emit_return_zero(); else compile_block(0);
        if (code.empty() || (code.back().opcode != TV1_RETURN && code.back().opcode != TV1_TRAP && code.back().opcode != TV1_HALT)) emit_return_zero();
    }

    std::vector<InstrWord> code;
    std::vector<TinyvmConstant> constants;
    std::vector<TinyvmString> strings;
    std::vector<TinyvmStorage> storage;
    std::vector<TinyvmProvenance> provenance;
    std::size_t slot_count() const { return next_slot_; }
    std::uint32_t isa_version() const { return uses_arguments_ ? 2 : 1; }

private:
    const Object& root_;
    std::string source_, derivation_;
    std::map<Integer, std::size_t> symbols_;
    std::map<Integer, std::vector<const Object*>> blocks_;
    std::set<Integer> active_blocks_;
    std::deque<std::string> string_data_;
    std::size_t next_slot_ = 0;
    std::size_t required_argument_count_ = 0;
    bool uses_arguments_ = false;
    std::uint64_t operation_ = UINT64_MAX, block_ = 1, symbol_ = UINT64_MAX;
    std::uint32_t line_ = 1;

    static std::uint32_t carrier(std::string_view type) {
        if (type == "bool") return TINYVM_CARRIER_I1;
        if (type == "c_int") return TINYVM_CARRIER_I32;
        if (type == "c_long" || type == "c_ulong" || type == "c_size_t") return TINYVM_CARRIER_I64;
        throw Unsupported("type carrier '" + std::string(type) + "' is not admitted by the scalar slice");
    }
    std::size_t slot() { return next_slot_++; }
    std::size_t symbol_slot(Integer identity) {
        const auto found = symbols_.find(identity);
        if (found != symbols_.end()) return found->second;
        return symbols_.emplace(identity, slot()).first->second;
    }
    void emit(std::int64_t opcode, std::int64_t a, std::int64_t b, std::int64_t pad) {
        code.push_back({opcode, a, b, pad});
        TinyvmProvenance item{};
        item.instruction = provenance.size(); item.operation = operation_; item.block = block_; item.symbol = symbol_;
        item.line = line_; item.column = 1; copy(item.source, source_); copy(item.derivation, derivation_);
        provenance.push_back(item);
    }
    std::size_t constant(std::uint32_t type, std::uint64_t bits) {
        for (const auto& item : constants) if (item.carrier == type && item.bits == bits) return item.id;
        const auto id = constants.size() + 1; constants.push_back({id, type, bits}); return id;
    }
    std::size_t literal(std::uint32_t type, std::uint64_t bits) {
        const auto result = slot(); emit(TV1_CONST, result, constant(type, bits), 0); return result;
    }
    std::size_t expression(const Value& value) {
        const auto& node = object(value, "$.expression");
        const auto kind = string(required(node, "kind", "$.expression"), "$.expression.kind");
        if (kind == "integer_literal") {
            const auto type = carrier(string(required(node, "type", "$.expression"), "$.expression.type"));
            const auto text = string(required(node, "value", "$.expression"), "$.expression.value");
            std::size_t consumed = 0; const auto signed_value = std::stoll(text, &consumed, 10);
            if (consumed != text.size()) throw Unsupported("non-canonical integer literal");
            if (type == TINYVM_CARRIER_I32 && (signed_value < INT32_MIN || signed_value > INT32_MAX)) throw Unsupported("i32 literal is out of range");
            const auto bits = type == TINYVM_CARRIER_I32 ? static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(signed_value))) : static_cast<std::uint64_t>(signed_value);
            return literal(type, bits);
        }
        if (kind == "bool_literal") {
            const auto text = string(required(node, "value", "$.expression"), "$.expression.value");
            if (text != "true" && text != "false") throw Unsupported("non-canonical boolean literal");
            return literal(TINYVM_CARRIER_I1, text == "true");
        }
        if (kind == "string_literal") {
            string_data_.push_back(string(required(node, "value", "$.expression"), "$.expression.value"));
            const auto id = string_data_.size();
            strings.push_back({id, reinterpret_cast<std::uint8_t*>(string_data_.back().data()), string_data_.back().size()});
            const auto result = slot(); emit(TV1_STRING_HANDLE, result, id, 0); return result;
        }
        if (kind == "writable_storage") {
            const auto& declaration = object(required(node, "storage", "$.expression"), "$.expression.storage");
            const auto bytes = integer(required(declaration, "bytes", "$.expression.storage"), "$.expression.storage.bytes");
            if (bytes <= 0) throw Unsupported("writable storage size is not positive");
            const auto id = storage.size() + 1; storage.push_back({id, static_cast<std::uint64_t>(bytes), 1, 1});
            const auto result = slot(); emit(TV1_STORAGE_HANDLE, result, id, 0); return result;
        }
        if (kind == "identifier") return symbol_slot(integer(required(node, "symbol_id", "$.expression"), "$.expression.symbol_id"));
        if (kind == "call" && optional(node, "intrinsic") && string(*optional(node, "intrinsic"), "$.expression.intrinsic") == "list_length") return 0;
        if (kind == "index" && optional(node, "intrinsic") && string(*optional(node, "intrinsic"), "$.expression.intrinsic") == "list_index") {
            const auto& index = object(required(node, "index", "$.expression"), "$.expression.index");
            const auto text = string(required(index, "value", "$.expression.index"), "$.expression.index.value");
            std::size_t consumed = 0; const auto value = std::stoull(text, &consumed, 10);
            if (consumed != text.size() || value + 1 > required_argument_count_) throw Unsupported("dynamic argument index is not admitted");
            return value + 1;
        }
        if (kind == "conversion") {
            const auto source = expression(required(node, "operand", "$.expression")); const auto result = slot();
            emit(TV1_CONVERT, result, source, carrier(string(required(node, "type", "$.expression"), "$.expression.type"))); return result;
        }
        if (kind == "unary") {
            const auto operation = string(required(node, "operator", "$.expression"), "$.expression.operator");
            const auto operand = expression(required(node, "operand", "$.expression"));
            if (operation == "+") return operand;
            if (operation != "-") throw Unsupported("unary operator '" + operation + "' is not admitted");
            const auto zero = literal(carrier(string(required(node, "type", "$.expression"), "$.expression.type")), 0), result = slot();
            emit(TV1_SUB, result, zero, operand); return result;
        }
        if (kind == "binary") {
            const auto left = expression(required(node, "left", "$.expression"));
            const auto right = expression(required(node, "right", "$.expression"));
            const auto operation = string(required(node, "operator", "$.expression"), "$.expression.operator");
            const std::map<std::string, std::int64_t> opcodes{{"+",TV1_ADD},{"-",TV1_SUB},{"*",TV1_MUL},{"/",TV1_SDIV},{"==",TV1_CMP_EQ},{"!=",TV1_CMP_NE},{"<",TV1_CMP_LT},{"<=",TV1_CMP_LE},{">",TV1_CMP_GT},{">=",TV1_CMP_GE}};
            const auto found = opcodes.find(operation); if (found == opcodes.end()) throw Unsupported("binary operator '" + operation + "' is not admitted");
            const auto result = slot(); emit(found->second, result, left, right); return result;
        }
        throw Unsupported("expression kind '" + kind + "' is not admitted by the scalar slice");
    }
    void set_provenance(const Object& operation) {
        operation_ = static_cast<std::uint64_t>(integer(required(operation, "id", "$.operation"), "$.operation.id")) + 1;
        block_ = optional(operation, "block_id") ? static_cast<std::uint64_t>(integer(*optional(operation, "block_id"), "$.operation.block_id")) + 1 : 1;
        symbol_ = optional(operation, "result_symbol_id") ? static_cast<std::uint64_t>(integer(*optional(operation, "result_symbol_id"), "$.operation.result_symbol_id")) + 1 : UINT64_MAX;
        line_ = optional(operation, "statement_id") ? static_cast<std::uint32_t>(integer(*optional(operation, "statement_id"), "$.operation.statement_id") + 1) : 1;
    }
    void compile_operation(const Object& operation) {
        set_provenance(operation);
        const auto kind = string(required(operation, "kind", "$.operation"), "$.operation.kind");
        const auto& operands = required_array(operation, "operands", "$.operation");
        if (operands.empty()) throw Unsupported("scalar operation has no operand");
        if (kind == "branch") {
            const auto value = expression(operands.front());
            const auto branch_index = code.size(); emit(TV1_BRANCH, value, 0, 0);
            const auto then_block = integer(required(operation, "then_block_id", "$.operation"), "$.operation.then_block_id");
            code[branch_index].b = static_cast<std::int64_t>(code.size());
            const bool then_terminal = compile_block(then_block);
            std::size_t then_jump = SIZE_MAX;
            if (!then_terminal) { set_provenance(operation); then_jump = code.size(); emit(TV1_JMP, 0, 0, 0); }
            if (const auto* otherwise = optional(operation, "else_block_id")) {
                code[branch_index].pad = static_cast<std::int64_t>(code.size());
                const bool else_terminal = compile_block(integer(*otherwise, "$.operation.else_block_id"));
                std::size_t else_jump = SIZE_MAX;
                if (!else_terminal) { set_provenance(operation); else_jump = code.size(); emit(TV1_JMP, 0, 0, 0); }
                const auto join = static_cast<std::int64_t>(code.size());
                if (then_jump != SIZE_MAX) code[then_jump].a = join;
                if (else_jump != SIZE_MAX) code[else_jump].a = join;
            } else {
                const auto join = static_cast<std::int64_t>(code.size());
                code[branch_index].pad = join;
                if (then_jump != SIZE_MAX) code[then_jump].a = join;
            }
            return;
        }
        if (kind == "loop") {
            const auto condition = code.size();
            const auto value = expression(operands.front());
            const auto branch_index = code.size(); emit(TV1_BRANCH, value, 0, 0);
            code[branch_index].b = static_cast<std::int64_t>(code.size());
            const auto body = integer(required(operation, "body_block_id", "$.operation"), "$.operation.body_block_id");
            const bool body_terminal = compile_block(body);
            if (!body_terminal) { set_provenance(operation); emit(TV1_JMP, condition, 0, 0); }
            code[branch_index].pad = static_cast<std::int64_t>(code.size());
            return;
        }
        const auto value = expression(operands.front());
        if (kind == "return_value") { emit(TV1_RETURN, value, 0, 0); return; }
        const auto identity = integer(required(operation, "result_symbol_id", "$.operation"), "$.operation.result_symbol_id");
        const auto destination = symbol_slot(identity);
        if (destination != value) emit(TV1_MOVE, destination, value, 0);
    }
    bool compile_block(Integer block) {
        if (!active_blocks_.insert(block).second) throw Unsupported("cyclic structured block ownership");
        const auto found = blocks_.find(block);
        if (found == blocks_.end()) throw Unsupported("referenced structured block is absent");
        bool terminal = false;
        for (const auto* operation : found->second) {
            if (terminal) break;
            compile_operation(*operation);
            terminal = string(required(*operation, "kind", "$.operation"), "$.operation.kind") == "return_value";
        }
        active_blocks_.erase(block);
        return terminal;
    }
    void emit_return_zero() {
        operation_ = UINT64_MAX; block_ = 1; symbol_ = UINT64_MAX; line_ = 1;
        const auto zero = literal(TINYVM_CARRIER_I32, 0); emit(TV1_RETURN, zero, 0, 0);
    }
    void scan_arguments(const Value& value) {
        if (const auto* node = std::get_if<Object>(&value)) {
            const auto* intrinsic = optional(*node, "intrinsic");
            if (intrinsic) {
                const auto name = string(*intrinsic, "$.intrinsic");
                if (name == "list_length") uses_arguments_ = true;
                if (name == "list_index") {
                    uses_arguments_ = true;
                    const auto& index = object(required(*node, "index"), "$.index");
                    const auto text = string(required(index, "value", "$.index"), "$.index.value");
                    std::size_t consumed = 0; const auto position = std::stoull(text, &consumed, 10);
                    if (consumed != text.size()) throw Unsupported("dynamic argument index is not admitted");
                    required_argument_count_ = std::max(required_argument_count_, static_cast<std::size_t>(position + 1));
                }
            }
            for (const auto& [key, child] : *node) { (void)key; scan_arguments(child); }
        } else if (const auto* items = std::get_if<Array>(&value)) for (const auto& child : *items) scan_arguments(child);
    }
    void emit_argument_guard() {
        operation_ = UINT64_MAX; block_ = 1; symbol_ = UINT64_MAX; line_ = 1;
        const auto required = literal(TINYVM_CARRIER_I32, required_argument_count_);
        const auto ready = slot(); emit(TV1_CMP_GE, ready, 0, required);
        const auto branch = code.size(); emit(TV1_BRANCH, ready, 0, 0);
        const auto failure = literal(TINYVM_CARRIER_I32, 64); emit(TV1_RETURN, failure, 0, 0);
        code[branch].b = static_cast<std::int64_t>(code.size());
        code[branch].pad = static_cast<std::int64_t>(branch + 1);
    }
};

int lower(const char* input_path, const char* output_path) {
    const auto input = parse(read(input_path));
    validate_backend_lowering_artifact(input);
    const auto& root = object(input);
    const auto canonical = serialize(input);
    const auto source = serialize(required(root, "source"));
    const auto plan_text = serialize(required(root, "lowering_plan"));
    const auto optimization = serialize(required(required_object(root, "provenance"), "optimization", "$.provenance"));
    const auto target = serialize(required(root, "target"));
    const auto source_id = identity("source-", source);
    const auto plan_id = identity("plan-", plan_text);
    Compiler compiler(root, source_id, plan_id);
    try { compiler.compile(); }
    catch (const Unsupported& unsupported) {
        std::cout << serialize(Object{{"backend", "tinyvm"}, {"format", "flowtiny.lowering_result"},
                                     {"reason", std::string(unsupported.what())}, {"status", "unsupported"},
                                     {"version", Integer{1}}}) << '\n';
        return 2;
    }

    TinyvmArtifactV2 artifact;
    tinyvm_artifact_v2_init(&artifact);
    artifact.isa_version = compiler.isa_version();
    artifact.data_words = compiler.slot_count();
    artifact.stack_words = 16;
    copy(artifact.artifact_id, identity("tinyvm-", canonical));
    copy(artifact.source_id, source_id);
    copy(artifact.target_policy_id, identity("target-", target));
    copy(artifact.lowering_plan_id, plan_id);
    copy(artifact.optimization_id, identity("opt-", optimization));
    artifact.code = compiler.code.data(); artifact.code_count = compiler.code.size();
    artifact.constants = compiler.constants.data(); artifact.constant_count = compiler.constants.size();
    artifact.strings = compiler.strings.data(); artifact.string_count = compiler.strings.size();
    artifact.storage = compiler.storage.data(); artifact.storage_count = compiler.storage.size();
    artifact.provenance = compiler.provenance.data(); artifact.provenance_count = compiler.provenance.size();
    char diagnostic[256];
    if (!tinyvm_artifact_v2_write(output_path, &artifact, diagnostic, sizeof diagnostic))
        throw std::runtime_error(std::string("cannot emit TinyVM artifact: ") + diagnostic);
    std::cout << serialize(Object{{"artifact_id", std::string(artifact.artifact_id)}, {"backend", "tinyvm"},
                                 {"format", "flowtiny.lowering_result"}, {"isa_version", Integer{compiler.isa_version()}},
                                 {"status", "emitted"}, {"version", Integer{1}}}) << '\n';
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) { std::cerr << "usage: flowtinylower INPUT.json OUTPUT.tvm\n"; return 2; }
        return lower(argv[1], argv[2]);
    } catch (const flowcontracts::json::Error& error) {
        std::cerr << "flowtinylower contract error: " << error.what() << '\n'; return 1;
    } catch (const std::exception& error) {
        std::cerr << "flowtinylower error: " << error.what() << '\n'; return 1;
    }
}
