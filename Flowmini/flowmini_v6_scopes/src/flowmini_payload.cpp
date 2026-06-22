#include "flowmini_payload.h"

#include <sstream>

namespace flowmini {

const char* payloadTypeName(const Payload& payload) {
    if (std::holds_alternative<UnitPayload>(payload)) { return "Unit"; }
    if (std::holds_alternative<TextPayload>(payload)) { return "Text"; }
    if (std::holds_alternative<IntPayload>(payload)) { return "Int"; }
    if (std::holds_alternative<BoolPayload>(payload)) { return "Bool"; }
    if (std::holds_alternative<RecordPayload>(payload)) { return "Record"; }
    return "unknown";
}

const char* valueTypeName(const Value& value) {
    if (std::holds_alternative<std::monostate>(value.data)) { return "Unit"; }
    if (std::holds_alternative<bool>(value.data)) { return "Bool"; }
    if (std::holds_alternative<std::int64_t>(value.data)) { return "Int"; }
    if (std::holds_alternative<std::string>(value.data)) { return "Text"; }
    if (std::holds_alternative<Record>(value.data)) { return "Record"; }
    if (std::holds_alternative<List>(value.data)) { return "List"; }
    return "unknown";
}

namespace {

[[nodiscard]] bool isSimplePath(const std::string& path) {
    return !path.empty() && path.find('.') == std::string::npos &&
           path.find('[') == std::string::npos && path.find(']') == std::string::npos;
}

void requireSimplePath(const std::string& path, const std::string& stage) {
    if (!isSimplePath(path)) {
        throw flow::DiagnosticError{stage, "only simple top-level paths are supported in flowmini v2: " + path};
    }
}

} // namespace

bool hasPath(const RecordPayload& payload, const std::string& path) {
    if (!isSimplePath(path)) {
        return false;
    }
    return payload.values.find(path) != payload.values.end();
}

const Value& getPathValue(const RecordPayload& payload, const std::string& path, const std::string& stage) {
    requireSimplePath(path, stage);
    const auto it = payload.values.find(path);
    if (it == payload.values.end()) {
        throw flow::DiagnosticError{stage, "missing record path: " + path};
    }
    return it->second;
}

Value& getPathValueMutable(RecordPayload& payload, const std::string& path, const std::string& stage) {
    requireSimplePath(path, stage);
    const auto it = payload.values.find(path);
    if (it == payload.values.end()) {
        throw flow::DiagnosticError{stage, "missing record path: " + path};
    }
    return it->second;
}

void setPathValue(RecordPayload& payload, const std::string& path, Value value, const std::string& stage) {
    requireSimplePath(path, stage);
    payload.values[path] = std::move(value);
}

std::int64_t getPathInt(const RecordPayload& payload, const std::string& path, const std::string& stage) {
    const Value& value = getPathValue(payload, path, stage);
    if (const auto* typed = std::get_if<std::int64_t>(&value.data)) {
        return *typed;
    }
    throw flow::DiagnosticError{stage, "record path '" + path + "' expected Int, got " + valueTypeName(value)};
}

bool getPathBool(const RecordPayload& payload, const std::string& path, const std::string& stage) {
    const Value& value = getPathValue(payload, path, stage);
    if (const auto* typed = std::get_if<bool>(&value.data)) {
        return *typed;
    }
    throw flow::DiagnosticError{stage, "record path '" + path + "' expected Bool, got " + valueTypeName(value)};
}


const List& getPathList(const RecordPayload& payload, const std::string& path, const std::string& stage) {
    const Value& value = getPathValue(payload, path, stage);
    if (const auto* typed = std::get_if<List>(&value.data)) {
        return *typed;
    }
    throw flow::DiagnosticError{stage, "record path '" + path + "' expected List, got " + valueTypeName(value)};
}

List& getPathListMutable(RecordPayload& payload, const std::string& path, const std::string& stage) {
    Value& value = getPathValueMutable(payload, path, stage);
    if (auto* typed = std::get_if<List>(&value.data)) {
        return *typed;
    }
    throw flow::DiagnosticError{stage, "record path '" + path + "' expected List, got " + valueTypeName(value)};
}

void setPathList(RecordPayload& payload, const std::string& path, List value, const std::string& stage) {
    setPathValue(payload, path, Value{std::move(value)}, stage);
}

void setPathInt(RecordPayload& payload, const std::string& path, std::int64_t value, const std::string& stage) {
    setPathValue(payload, path, Value{value}, stage);
}

void setPathBool(RecordPayload& payload, const std::string& path, bool value, const std::string& stage) {
    setPathValue(payload, path, Value{value}, stage);
}

} // namespace flowmini
