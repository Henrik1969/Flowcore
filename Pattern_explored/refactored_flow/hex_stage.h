#ifndef HEX_STAGE_H
#define HEX_STAGE_H

#include "flow_common.h"

#include <cstdint>
#include <string>
#include <vector>

class HexStage {
public:
    [[nodiscard]] flow::Envelope<flow::ByteBuffer> run(flow::Envelope<flow::ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string encode(
        const std::string& input,
        int width,
        bool uppercase
    );

    [[nodiscard]] static std::string decode(
        const std::string& input,
        bool strict,
        bool ignoreWhitespace,
        std::vector<flow::Diagnostic>& diagnostics
    );

    [[nodiscard]] static int hexValue(unsigned char c);
};

#endif // HEX_STAGE_H
