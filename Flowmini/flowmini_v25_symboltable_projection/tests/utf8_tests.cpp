#include "flowmini_utf8.h"

#include <cassert>
#include <string>

using flowmini::decodeUtf8;

int main() {
    const auto valid = decodeUtf8("A\xC3\xA6\xF0\x9F\x8C\x8D");
    assert(valid.valid());
    assert(valid.scalars.size() == 3);
    assert(valid.scalars[0].value == 0x41);
    assert(valid.scalars[0].byte_offset == 0);
    assert(valid.scalars[1].value == 0xe6);
    assert(valid.scalars[1].byte_offset == 1);
    assert(valid.scalars[1].byte_length == 2);
    assert(valid.scalars[2].value == 0x1f30d);
    assert(valid.scalars[2].byte_offset == 3);
    assert(valid.scalars[2].byte_length == 4);

    const auto malformed = decodeUtf8(
        std::string("\xC0\xE2\x82\xAC\xF0\x90\x80", 7)
    );
    assert(!malformed.valid());
    assert(malformed.diagnostics.size() == 4);
    assert(malformed.diagnostics[0].code == "invalid-leading-byte");
    assert(malformed.diagnostics[1].code == "truncated-sequence");

    const auto surrogate = decodeUtf8(std::string("\xED\xA0\x80", 3));
    assert(!surrogate.valid());
    assert(surrogate.diagnostics.size() == 3);
    assert(surrogate.diagnostics[0].code == "surrogate-scalar");
}
