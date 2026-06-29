#pragma once

#include <cstdint>
#include <string>

namespace symboltable {

struct SourceLocation final {
    std::string file;
    std::uint32_t line {0};
    std::uint32_t column {0};
};

} // namespace symboltable
