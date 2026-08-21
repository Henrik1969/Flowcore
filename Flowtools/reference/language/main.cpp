#include <frankencore/language.hpp>

#include <cassert>

int main() {
    frankencore::contracts::LanguageMap map;
    map.id = "Danish";
    map.revision = "Danish.v1";
    map.parser = "surface.v1";
    map.parent = "canonical";
    map.monikers["ask"] = {"Spørg"};
    map.monikers["echo"] = {"skriv"};

    const auto canonical = frankencore::language::resolve_moniker(map, "Spørg");
    assert(canonical.resolved && canonical.canonical == "ask");
    const auto identity = frankencore::language::resolve_moniker(map, "ask");
    assert(identity.resolved && identity.canonical == "ask");
    const auto missing = frankencore::language::resolve_moniker(map, "Frage");
    assert(!missing.resolved);

    map.monikers["question"] = {"Spørg"};
    const auto collision = frankencore::language::resolve_moniker(map, "Spørg");
    assert(!collision.resolved);
    return 0;
}
