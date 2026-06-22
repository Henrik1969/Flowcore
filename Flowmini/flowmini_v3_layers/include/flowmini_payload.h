#ifndef FLOWMINI_PAYLOAD_H
#define FLOWMINI_PAYLOAD_H

#include "flow_common.h"

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace flowmini {

struct UnitPayload {};

struct TextPayload {
    std::string value;
};

struct IntPayload {
    std::int64_t value = 0;
};

struct BoolPayload {
    bool value = false;
};

struct Value;
using Record = std::map<std::string, Value>;
using List = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::monostate, bool, std::int64_t, std::string, Record, List>;
    Storage data;

    Value() = default;
    Value(bool value) : data(value) {}
    Value(std::int64_t value) : data(value) {}
    Value(int value) : data(static_cast<std::int64_t>(value)) {}
    Value(std::string value) : data(std::move(value)) {}
    Value(const char* value) : data(std::string{value}) {}
    Value(Record value) : data(std::move(value)) {}
    Value(List value) : data(std::move(value)) {}
};

struct RecordPayload {
    Record values;
};

using Payload = std::variant<UnitPayload, TextPayload, IntPayload, BoolPayload, RecordPayload>;
using MiniEnvelope = flow::Envelope<Payload>;

[[nodiscard]] const char* payloadTypeName(const Payload& payload);
[[nodiscard]] const char* valueTypeName(const Value& value);

[[nodiscard]] bool hasPath(const RecordPayload& payload, const std::string& path);
[[nodiscard]] const Value& getPathValue(const RecordPayload& payload, const std::string& path, const std::string& stage);
Value& getPathValueMutable(RecordPayload& payload, const std::string& path, const std::string& stage);
void setPathValue(RecordPayload& payload, const std::string& path, Value value, const std::string& stage);

[[nodiscard]] std::int64_t getPathInt(const RecordPayload& payload, const std::string& path, const std::string& stage);
[[nodiscard]] bool getPathBool(const RecordPayload& payload, const std::string& path, const std::string& stage);
void setPathInt(RecordPayload& payload, const std::string& path, std::int64_t value, const std::string& stage);
void setPathBool(RecordPayload& payload, const std::string& path, bool value, const std::string& stage);

// Throws flow::DiagnosticError if the envelope carries the wrong payload type.
template <typename T>
T& requirePayload(MiniEnvelope& env, const std::string& stage) {
    if (auto* value = std::get_if<T>(&env.payload)) {
        return *value;
    }

    throw flow::DiagnosticError{stage, "payload contract mismatch"};
}

template <typename T>
const T& requirePayloadConst(const MiniEnvelope& env, const std::string& stage) {
    if (const auto* value = std::get_if<T>(&env.payload)) {
        return *value;
    }

    throw flow::DiagnosticError{stage, "payload contract mismatch"};
}

} // namespace flowmini

#endif // FLOWMINI_PAYLOAD_H
