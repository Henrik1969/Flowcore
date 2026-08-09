#include "flowmini_payload.h"

namespace flowmini {

const char* payloadTypeName(const Payload& payload) {
    if (std::holds_alternative<UnitPayload>(payload)) {
        return "Unit";
    }

    if (std::holds_alternative<TextPayload>(payload)) {
        return "Text";
    }

    if (std::holds_alternative<IntPayload>(payload)) {
        return "Int";
    }

    return "Unknown";
}

} // namespace flowmini
