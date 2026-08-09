#pragma once

#include <cstdint>
#include <functional>

namespace symboltable {

struct SymbolId final {
    std::uint64_t value {0};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(SymbolId lhs, SymbolId rhs) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(SymbolId lhs, SymbolId rhs) noexcept = default;
};

struct ScopeId final {
    std::uint64_t value {0};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(ScopeId lhs, ScopeId rhs) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(ScopeId lhs, ScopeId rhs) noexcept = default;
};

} // namespace symboltable

namespace std {

template <>
struct hash<symboltable::SymbolId> final {
    [[nodiscard]] std::size_t operator()(const symboltable::SymbolId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value);
    }
};

template <>
struct hash<symboltable::ScopeId> final {
    [[nodiscard]] std::size_t operator()(const symboltable::ScopeId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value);
    }
};

} // namespace std
