#include <frankencore/policy.hpp>

#include <cassert>

int main() {
    using namespace frankencore::policy;
    const auto decision = resolve_with_configresolve(
        {Fact{"frankencore.policy.outcome", "allowed", FactSource::runtime,
              "test"}},
        "frankencore.policy.outcome");
    assert(decision.resolved);
    assert(decision.outcome == frankencore::contracts::PolicyOutcome::allowed);
    return 0;
}
