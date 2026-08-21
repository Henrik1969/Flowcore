# Native binding generation

`tools/generate-flow-bindings.sh` is the additive generation brick between
provider selection and the Flow toolchain. It does not infer C signatures from
ELF symbols: symbol tables prove names and addresses, not parameter types,
ownership, effects, or lifetimes. Those facts therefore remain explicit in a
reviewable `flowcore.native_binding_spec` JSON artifact.

The generator verifies the provider path and every selected exported symbol,
then emits three artifacts:

```text
explicit ABI spec
    -> generated namespace.flow
    -> generated capability policy
    -> generated provider/evidence manifest
```

Example:

```sh
tools/generate-flow-bindings.sh \
  --spec docs/architecture/schemas/flowcore-native-binding-spec-v1.example.json \
  --flow-output /tmp/generated-kernel.flow \
  --policy-output /tmp/generated-kernel.policy \
  --manifest-output /tmp/generated-kernel.manifest.json
```

The generated `.flow` file can be imported by Flowmini immediately. The
manifest records the specification and provider hashes, exported-symbol
verification, and the emitted authorization facts. Generation never loads a
function for execution and never modifies the C++ pipeline. A later contract
change is handled by regenerating artifacts and rerunning the normal Flowmini
→ Flowanalyst → Flowbind → Flowparallel → Flowoptimize → Flowlower gates.

Provider changes require regeneration and review. A changed provider hash is
evidence of substrate drift; it is not an automatic authorization to accept a
new ABI.
