#ifndef FLOWMINI_PAYLOAD_H
#define FLOWMINI_PAYLOAD_H

#include "flow_common.h"

#include <cstdint>
#include <string>
#include <variant>

namespace flowmini {

struct UnitPayload {};

struct TextPayload {
    std::string value;
};

struct IntPayload {
    std::int64_t value = 0;
};

using Payload = std::variant<UnitPayload, TextPayload, IntPayload>;
using MiniEnvelope = flow::Envelope<Payload>;

[[nodiscard]] const char* payloadTypeName(const Payload& payload);

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
