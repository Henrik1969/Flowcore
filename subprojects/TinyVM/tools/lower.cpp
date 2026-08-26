#include <flowcontracts/validate.hpp>
extern "C" {
#include <tinyvm/isa_v1.h>
}

#include <openssl/sha.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

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

int lower(const char* input_path, const char* output_path) {
    const auto input = parse(read(input_path));
    validate_backend_lowering_artifact(input);
    const auto& root = object(input);
    const auto& plan = required_object(root, "lowering_plan");
    const auto& operations = required_array(plan, "operations", "$.lowering_plan");
    if (!operations.empty()) {
        std::cout << serialize(Object{{"backend", "tinyvm"}, {"format", "flowtiny.lowering_result"},
                                     {"reason", "provider-free non-empty lowering is not admitted yet"},
                                     {"status", "unsupported"}, {"version", Integer{1}}}) << '\n';
        return 2;
    }

    TinyvmConstant constants[] = {{1, TINYVM_CARRIER_I32, 0}};
    InstrWord code[] = {{TV1_CONST, 0, 1, 0}, {TV1_RETURN, 0, 0, 0}};
    TinyvmProvenance provenance[2] = {};
    const auto canonical = serialize(input);
    const auto source = serialize(required(root, "source"));
    const auto plan_text = serialize(required(root, "lowering_plan"));
    const auto optimization = serialize(required(required_object(root, "provenance"), "optimization", "$.provenance"));
    const auto target = serialize(required(root, "target"));
    const auto source_id = identity("source-", source);
    const auto plan_id = identity("plan-", plan_text);
    for (std::size_t index = 0; index < 2; ++index) {
        provenance[index].instruction = index;
        provenance[index].operation = UINT64_MAX;
        provenance[index].block = 1;
        provenance[index].symbol = UINT64_MAX;
        provenance[index].line = 1;
        provenance[index].column = 1;
        copy(provenance[index].source, source_id);
        copy(provenance[index].derivation, plan_id);
    }

    TinyvmArtifactV2 artifact;
    tinyvm_artifact_v2_init(&artifact);
    artifact.isa_version = 1;
    artifact.data_words = 1;
    artifact.stack_words = 16;
    copy(artifact.artifact_id, identity("tinyvm-", canonical));
    copy(artifact.source_id, source_id);
    copy(artifact.target_policy_id, identity("target-", target));
    copy(artifact.lowering_plan_id, plan_id);
    copy(artifact.optimization_id, identity("opt-", optimization));
    artifact.code = code; artifact.code_count = 2;
    artifact.constants = constants; artifact.constant_count = 1;
    artifact.provenance = provenance; artifact.provenance_count = 2;
    char diagnostic[256];
    if (!tinyvm_artifact_v2_write(output_path, &artifact, diagnostic, sizeof diagnostic))
        throw std::runtime_error(std::string("cannot emit TinyVM artifact: ") + diagnostic);
    std::cout << serialize(Object{{"artifact_id", std::string(artifact.artifact_id)}, {"backend", "tinyvm"},
                                 {"format", "flowtiny.lowering_result"}, {"isa_version", Integer{1}},
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
