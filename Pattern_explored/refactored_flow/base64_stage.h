#ifndef BASE64_STAGE_H
#define BASE64_STAGE_H

#include "flow_common.h"

#include <cstdint>
#include <string>
#include <vector>

class Base64Stage {
public:
    [[nodiscard]] flow::Envelope<flow::ByteBuffer> run(flow::Envelope<flow::ByteBuffer> env) const;

private:
    [[nodiscard]] static std::string encode(const std::string& input, int wrap);
    [[nodiscard]] static std::string decode(
        const std::string& input,
        bool strict,
        bool ignoreWhitespace,
        std::vector<flow::Diagnostic>& diagnostics
    );

    [[nodiscard]] static int decodeChar(unsigned char c);
};

#endif // BASE64_STAGE_H
