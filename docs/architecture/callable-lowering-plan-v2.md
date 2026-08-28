# Callable lowering plan v2

`flowcore.lowering_plan` version 2 adds the first compiler-construction closure
facility: complete ordinary callable-function authority.

The `functions` catalog records stable function symbol identity, name, scope,
body block, return type, ordered parameter symbol/type pairs, definition versus
declaration availability, and explicit entry candidacy. Multiple named-target
entries remain distinct until `flowprepare` selects a source target. Every operation carries `function_symbol_id`; ordinary call
operations name a catalogued `callee_symbol_id` and retain their typed operand
trees. A call expression used directly by another expression is preserved as a
typed call operand, rather than flattened to `unsupported`.

This file is complete after Flowanalyst exits. Flowparallel and Flowoptimize
preserve it structurally, and `flowprepare` captures it without consulting the
frontend. Flowcontracts rejects duplicate functions/parameters, missing or
multiple entries, unknown operation owners and unknown ordinary callees.

Version 1 remains readable for captured programs that have no callable catalog.
Version 2 is the required authority for implementing functions in LLVM and
TinyVM; backends may specialize non-recursive calls, but may not rediscover
function ownership from source names or lexical accidents.

The first LLVM consumer emits each v2 definition as a separate LLVM function,
binds parameters through their stable symbol identities and connects ordinary
call results to the captured expression identity. The shared scalar classifier
is called independently by tokenizer-shaped and headless-document-shaped probes
and returns the same class through an `llvm-host` target-policy build.

The first TinyVM consumer deterministically specializes the same non-recursive
call graph while lowering: arguments move into typed parameter-identity slots,
each function return writes a dedicated result slot and jumps to the call
continuation, and the surrounding expression consumes that result by captured
expression identity. No host stack address or source lookup is serialized.

During the staged consumer migration, Flowanalyst emits version 1 by default
and emits the complete callable boundary with
`--lowering-plan-version 2`. This is an explicit artifact-version request, not
source-name dispatch. The default changes only after both backend consumers
pass the callable parity corpus, preserving existing captured pipelines while
the new file contract is independently testable.
