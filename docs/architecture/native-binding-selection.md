# Native binding selection

The host scanner produces facts about what is present. It does not decide what
Flowcore should use. Selection is a separate, reviewable project artifact.

```text
host inventory
    -> explicit selection
        -> namespace assignment
            -> bind an existing provider OR implement Flowcore-owned behavior
                -> contract and policy review
                    -> binding/lowering implementation
```

Each selection has a canonical namespace, a capability identity, a contract,
and an explicit strategy:

* `bind-existing` uses a provider recorded exactly in the inventory;
* `implement-flowcore` deliberately owns the implementation and has no host
  provider reference.

Neither choice authorizes execution, loads a library, or generates a binding.
Those remain later policy and implementation stages. This prevents discovery
from silently becoming authority and lets us choose portability, correctness,
or substrate compatibility capability by capability.

The machine-readable schema and example are:

* `docs/architecture/schemas/flowcore-native-binding-selection-v1.json`
* `docs/architecture/schemas/flowcore-native-binding-selection-v1.example.json`

Validate a selection against a fresh inventory with:

```sh
tools/scan-native-bindings.sh --output native-binding-inventory.json
tools/validate-native-binding-selection.sh \
  native-binding-inventory.json \
  docs/architecture/schemas/flowcore-native-binding-selection-v1.example.json
```

The validator checks the inventory contract, selection shape, unique IDs,
namespace syntax, and exact provider presence. Its result remains
`authorization: not-performed`, `execution: not-performed`, and
`binding_generation: not-performed`.
