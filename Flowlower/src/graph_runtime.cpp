#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
thread_local const char* activation = nullptr;
thread_local std::uint64_t active_operation = 0;
thread_local int failure_code = 0;
bool tracing() {
    const auto* value = std::getenv("FLOWCORE_GRAPH_TRACE");
    return value && std::strcmp(value, "1") == 0;
}
void record(const char* value) { if (value) { std::fputs(value, stderr); std::fputc('\n', stderr); } }
}
extern "C" void flow_graph_enter(const char* value) {
    activation = value; active_operation = 0; failure_code = 0;
    if (tracing()) record(value);
}
extern "C" void flow_graph_operation(std::uint64_t operation) { active_operation = operation; }
extern "C" void flow_graph_event(const char* value) { if (tracing()) record(value); }
extern "C" void flow_graph_drop(const char* value) { record(value); }
extern "C" [[noreturn]] void flow_graph_fail(std::uint64_t operation, const char* reason) {
    // Both strings are compiler-serialized constants; no payload or raw pointer
    // is interpolated into the diagnostic. Failure never publishes an output.
    std::fprintf(stderr, "{\"format\":\"flowcore.graph_failure\",\"version\":1,\"operation_id\":%llu,\"reason\":\"%s\",\"code\":%d,\"activation\":%s}\n",
        static_cast<unsigned long long>(operation), reason, failure_code, activation ? activation : "null");
    std::exit(70);
}

extern "C" int flow_graph_raise(int code) {
    failure_code = code;
    flow_graph_fail(active_operation, "source_failure");
}
