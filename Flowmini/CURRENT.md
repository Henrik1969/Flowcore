# Current Flowmini Version

Current checkout status:

```text
Flowmini v0.30 canonical variant backend completion slice
implementation directory: flowmini_v25_symboltable_projection
```

The directory name is historical. Flowmini remains experimental and unfinished.
The structural frontend and normal runtime are separate paths; see the
[Programmer’s Guide](../docs/language/flowmini-programmers-manual.md).

## Verified status

```text
root CTest:                         PASS (93/93)
AST golden tests:                   PASS (28)
symbol projection tests:            PASS (14)
Flowmini focused CTest:             PASS (14/14)
categorized flowmini_suite:         91/140 (49 known parser/ABI/profile gaps)
variant payload lowering:           IMPLEMENTED/TESTED (LLVM i32 carrier; enum payload)
compile-time constants:            IMPLEMENTED/TESTED (bounded deterministic expressions)
canonical guard lowering:           IMPLEMENTED/TESTED (LLVM backend)
std.math v0:                        IMPLEMENTED/TESTED (pure integer helpers)
Flowbind C generator:               EXPERIMENTAL/TESTED (Clang C subset, partial artifacts)
mature program probes:              IMPLEMENTED/TESTED (flowstats, flowconfig)
variant carrier experiment:         IMPLEMENTED/TESTED (target-neutral identity; one-slot LLVM carrier)
multi-field/record/nested variants: NOT SUPPORTED — BACKEND LIMITATION (explicit lowering diagnostic)
TinyVM variant payload lowering:    NOT SUPPORTED — BACKEND LIMITATION
ASan/UBSan CTest:                   PASS (93/93; leak checks disabled)
```

The structural chain publishes TokenTree/AST, symbol and fact projections for
Flowanalyst, Flowbind, Flowoptimize, and Flowlower. Integer and enum match
lowering is tested. Variant payload labels survive the AST and semantic
artifacts. A target-neutral carrier now preserves variant type/member/discriminant
and payload type identity; LLVM lowers one i32 payload slot and extracts it in
match arms. Larger payload layouts and TinyVM remain explicit backend limits.
Runtime probes cover a broader compatibility parser,
including constants, guards, records, collections, enums, and variants; that
does not establish artifact-chain parity.

## Build and test

```bash
cmake -S . -B /tmp/flowcore-build -G Ninja
cmake --build /tmp/flowcore-build
ctest --test-dir /tmp/flowcore-build --output-on-failure

cmake -S Flowmini/flowmini_v25_symboltable_projection -B /tmp/flowmini-build -G Ninja
cmake --build /tmp/flowmini-build
ctest --test-dir /tmp/flowmini-build --output-on-failure
```

The categorized suite is intentionally reported separately because its
expected-pass inventory exposes unfinished parser, ABI, and profile contracts.

## Architecture law

```text
TokenTree remembers what the source looked like.
AST states what the source means.
Projections expose facts without becoming authority.
```

The transformation/revision boundary is documented in
`docs/architecture/compiler-transformation-revision-model.md`. Future
self-hosting work must preserve C++ artifacts as comparison evidence and move
one independently tested language boundary at a time.

## Known incomplete areas

```text
canonical parser/semantic convergence
target selection and multitarget artifact emission
generic collections and broader standard library
text/io/fs/time/net APIs and generated binding declarations
multi-field variant carrier layouts and TinyVM parity
ownership, resource, effect, and concurrency semantics
unsafe/opaque source regions intentionally excluded; provider paperwork remains bounded work
optimizer breadth, debugging/tooling, and self-hosting
```

See [`docs/FUTURE_WORK_EVIDENCE.md`](../docs/FUTURE_WORK_EVIDENCE.md) and the
[language-comparison index](../docs/language-comparisons/README.md).
