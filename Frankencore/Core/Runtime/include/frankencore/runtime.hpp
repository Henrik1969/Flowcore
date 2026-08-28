#pragma once

#include <cstdint>
#include <string>

namespace frankencore::runtime {

struct CpuFacts { std::uint64_t logical_processors = 0; };
struct MemoryFacts { std::uint64_t total_bytes = 0; std::uint64_t available_bytes = 0; };
struct CudaFacts {
    std::string status = "unknown";
    std::string driver;
    std::uint64_t device_count = 0;
    std::string diagnostic;
};
struct Capabilities {
    std::string format = "frankencore.runtime_capabilities";
    int version = 1;
    CpuFacts cpu;
    MemoryFacts memory;
    CudaFacts cuda;
};

// Read-only discovery. It does not enable providers, allocate workers, or
// resolve policy. Those decisions belong to a later runtime planner.
Capabilities discover();

// JSON is an inspectable projection; the C++ structure remains canonical.
std::string to_json(const Capabilities& capabilities);

} // namespace frankencore::runtime
