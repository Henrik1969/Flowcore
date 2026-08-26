# Stage 0 host-feature inventory

**Seed:** current C/C++ Flowcore checkout on `v25-symboltable-projection`  
**Purpose:** declare the host privileges that a later Flow-written compiler must
replace through public file boundaries.

## Rebuild seed

The root superbuild requires CMake 3.22 or newer, a C11-capable compiler, a
C++20-capable compiler, the host C/C++ standard libraries, POSIX shell test
tools, `jq`, and LLVM/Clang for executable LLVM parity tests. TinyVM's optimized
engine additionally uses the documented GCC/Clang labels-as-values extension;
the portable switch engine does not. OpenSSL Crypto, threads, `dlopen`/`dlsym`,
and selected system libraries/providers are discovered by the build or invoked
by governed integration tests.

`tools/capture-stage0-seed.sh` emits a versioned, canonical snapshot of the
actual checkout, tool versions, tracked-source digest, and declared external
provider surfaces. It is evidence, not an input that changes compiler meaning.

## Component inventory

| Stage 0 component | Material C++ facilities in current source | Flow closure owner |
|---|---|---|
| Flowmini lexer | byte/string views, vectors, character classification, exact offsets, diagnostic exceptions | text/bytes; spans; outcome diagnostics |
| Flowmini parser and AST builder | recursive descent, recursive/tagged AST values, optional nodes, owned trees, vectors/maps, recovery diagnostics | variants; recursive/arena data; option/outcome; bounded recursion |
| Symbol projection | maps/sets, stable integer identities, scopes, deterministic traversal, provenance maps | collections; stable identity; module/scope semantics |
| Frontend bundle writer | deterministic JSON strings, arrays/objects, escaping, file/stdin I/O | canonical serialization; streams; Unicode validation |
| Flowanalyst | structural JSON consumer, symbol/type maps, semantic facts, diagnostics, Graph/lowering-plan construction | artifact readers; type facts; graph construction |
| Flowbind | filesystem/provider manifests, maps/sets, ABI tuples, exact capability matching, dynamic-library evidence | packages; paths; ABI records; capabilities |
| Flowparallel | graph/matrix collections, dependency sets, provider decisions and fallback evidence | graph algorithms; resource/effect policies |
| Flowoptimize | typed artifact consumption, deterministic transforms, projections and provider decisions | algorithms; canonical artifact transform |
| flowprepare/flowtarget | filesystem paths, canonical JSON, target lookup, policy admission | paths; artifact I/O; target-policy client |
| LLVM lowerer | tagged JSON values, maps/sets, string assembly, files, optional emission, structured failure | IR model; deterministic writer; outcome |
| TinyVM lowerer | maps/vectors/sets, recursive expression lowering, typed slots, binary artifact writer | callable IR; aggregates; deterministic binary serialization |
| Flowcontracts | recursive JSON variant, parser/serializer, typed validators, sets, error paths | core variants/collections; canonical serialization; diagnostics |

## Classification of remaining closure

- **Language semantics:** complete callable-function IR, recursive/tagged data,
  ownership/observation and cleanup, interfaces/generics, outcome propagation,
  module visibility, concurrency/effect edges.
- **Standard library:** UTF-8/text, bytes and buffers, generic collections and
  algorithms, stable hashing, canonical JSON/binary I/O, paths and diagnostics.
- **Provider capabilities:** filesystem/process/environment, dynamic libraries,
  clocks, threading, crypto, terminal/UI and target toolchains.
- **Tool support:** package manifests/locks, separate compilation, incremental
  build graph, golden/mutation/fuzz harnesses and bootstrap comparison.
- **Application code:** tokenizer/parser policies, document tree, editing,
  undo/redo, layout and format codecs belong above those general facilities.

## First shared closure slice

The smallest useful compiler/document slice is a callable pure classifier:

1. a tokenizer classifies an input scalar as whitespace, digit, identifier
   start/continuation, punctuation or invalid;
2. a headless document model classifies the same scalar for word/line boundary
   handling;
3. both call an ordinary Flow function and return a stable enum-like integer;
4. the captured lowering artifact must contain complete callable-function
   structure; LLVM and TinyVM must execute it equivalently.

This deliberately starts with Unicode-scalar-shaped integers rather than
claiming UTF-8 decoding. The following separately owned slice adds validated
byte/text decoding. The current blocker is earlier: backend-neutral lowering
records `call` operations but does not preserve a complete function catalog,
parameter bindings, entry root, or call-return structure. TinyVM's current
failure on `fn_demo.flow` is retained as the opening evidence.
