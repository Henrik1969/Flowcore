#ifndef BITART_STAGE_H
#define BITART_STAGE_H

#include "flow_common.h"

#include <cstdint>
#include <string>
#include <vector>

struct BitArtHeader {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint32_t bitCount = 0;
};

class BitArtStage {
public:
    [[nodiscard]] flow::Envelope<flow::ByteBuffer> run(flow::Envelope<flow::ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string pack(
        const std::string& input,
        int width,
        char oneChar,
        char zeroChar,
        bool strict,
        bool ignoreWhitespace,
        std::vector<flow::Diagnostic>& diagnostics
    );

    [[nodiscard]] static std::string unpack(
        const std::string& input,
        char oneChar,
        char zeroChar,
        bool strict,
        std::vector<flow::Diagnostic>& diagnostics
    );

    [[nodiscard]] static std::string writeBart(
        std::uint16_t width,
        std::uint16_t height,
        const std::vector<bool>& bits
    );

    [[nodiscard]] static BitArtHeader readHeader(const std::string& input);

    static void appendU16Le(std::string& out, std::uint16_t value);
    static void appendU32Le(std::string& out, std::uint32_t value);

    [[nodiscard]] static std::uint16_t readU16Le(const std::string& input, std::size_t offset);
    [[nodiscard]] static std::uint32_t readU32Le(const std::string& input, std::size_t offset);
};

#endif // BITART_STAGE_H
