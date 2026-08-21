#include <frankencore/requirements.hpp>

#include <cassert>

int main() {
    using namespace frankencore::requirements;
    assert(validate_version("2.8.3").valid);
    assert(!validate_version("v2").valid);
    assert(satisfies("2.8.3", ">=2 <3").valid);
    assert(satisfies("2.8.3", ">=2 <3").matches);
    assert(satisfies("3.0", ">=2 <3").valid);
    assert(!satisfies("3.0", ">=2 <3").matches);
    assert(!satisfies("2.8", "latest").valid);
    return 0;
}
