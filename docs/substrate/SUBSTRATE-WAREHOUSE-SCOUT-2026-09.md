# Substrate warehouse scout — 2026-09-08

This is a read-only inventory of the reusable substrate already available to
Flowcore. It records local observations and conservative interpretations; it
does not authorize a provider, generate a binding, or change the host.

Evidence labels used throughout are **OBSERVED** (confirmed from local files or
tool output), **INFERRED** (a bounded architectural interpretation),
**UNKNOWN** (evidence is insufficient), and **NOT PRESENT** (the local tool or
artifact was absent). Raw inventories remain outside Git; the compact structured
records are in [`substrate-warehouse-inventory.json`](substrate-warehouse-inventory.json).

## Executive findings

There is already a substantial warehouse. The Flowcore checkout contains
versioned contract, frontend, analysis, binding, lowering, parallel, terminal,
package-projection, kernel, inspection, and language components. The surrounding
development realm adds mature C++ bricks for configuration, environment,
arguments, text, AST structure, AI providers, and symbol facts. The host has
15,416 C headers, 8,532 shared objects, 1,472 static archives, 294 CMake package
files, and 321 pkg-config packages (**OBSERVED**, counts from the 2026-09-08
scan). LLVM/Clang, Qt 5/6, OpenBLAS, OpenCL, SQLite, zlib, curl, OpenSSL,
libxml2, libarchive, Wayland, PipeWire runtime, PulseAudio, and ncurses are
present.

The strongest immediate conclusion is that discovery is not the scarce part.
The high-value missing bridge is semantic carrier coverage: buffer-plus-length,
opaque resource handles, and verified aggregates appear repeatedly. Callbacks,
variadic functions, C++ objects/templates, and managed-runtime objects are
larger steps and should not be added speculatively. Existing Flowbind already
has a useful scalar/string/pointer subset and a Clang-backed generator; it should
grow from measured provider experiments rather than from a universal ABI.

The warehouse also falsifies a “rewrite it in Flow” instinct. `TextLib`,
`ConfigResolve`, `AiLib`, LLVM/Clang, Qt, zlib, SQLite, curl and the Linux
substrate already solve difficult problems. The appropriate Flow architecture is
to give their useful behavior explicit capability contracts and let policy pick
providers.

## 1. Internal Flow/FrankenCore warehouse

The realm-level [`BRICK_INVENTORY.md`](../../../BRICK_INVENTORY.md) was used as
the starting cross-project map and checked against local READMEs/CMake files.
The most reusable internal bricks are:

| Brick | Observed interface/artifact | Capability potential | Provider suitability |
| --- | --- | --- | --- |
| ConfigResolve 1.1.0 | C++20, C ABI, CMake and pkg-config metadata | layered configuration, precedence, schema, provenance, scoped views | capability brick; use as configuration authority |
| EnvVar 0.3.8 | C++20 library wrapping `getenv`/`setenv`/`environ`, snapshots and views | process environment source and controlled mutation | environment provider/source adapter |
| ArgsLib 0.1.0 | C++20 parser, positional/options/defaults/diagnostics, CMake package | invocation ingestion | easy provider/source adapter |
| TextLib 0.1.0 | byte text, edits, ranges, history, transactions, CMake package | text substrate for editors/parsers/projections | pure data brick; encoding remains a higher policy |
| AstLib 0.1.0-dev | lossless token/group tree over externally owned tokens | structural source representation | structural component; grammar meaning remains with consumer |
| AiLib 0.2.0 | capability registry, policy gate, mock/local providers, embeddings/vector store | governed AI capability | reference provider/capability architecture |
| SymbolTable v1.1 | static/shared/header targets, factual symbols plus views/policies | compiler symbol facts | compiler substrate, not a host provider |
| TokenTree 0.1 | C ABI, shared/static, borrowed text and explicit status | lossless structural grouping | compiler structural component |
| Flowcontracts | header-only strict artifact parser/validators | durable authority boundaries | shared contract authority |
| Flowterminal 1.0.0 | C shared provider with terminal-session resource | terminal capability | platform provider with cleanup boundary |
| Flowpkgprojector | C transactional symlink-tree projector | package projection/rollback | system provider; mutation policy stays above it |
| FlowLFS | sealed Linux substrate and provider environment | target/deployment substrate | target provider, not an ordinary library |
| firetest 0.1.2 | host-tool probes and evidence reports | capability inventory/conformance | evidence consumer, never provider authority |
| VerbLayer | early verb-command projection over Unix tools | intent projection | projection layer; stable contract not established |
| SystemLocal | ckb-next/ckbctl device tools | hardware/device control | OS-local provider; effects and policy require review |
| chatarchive 0.7.0 | private Python archive/workflow application | archive and memory workflow | application until reusable contracts are extracted |
| VectorLib | empty/incubating directory | planned vector/matrix/geometry | **NOT PRESENT** as a usable brick; semantic card required first |

The internal inventory shows useful composition paths already exist:
`ArgsLib + EnvVar -> ConfigResolve`, `TextLib + AstLib -> language tooling`,
and `AiLib -> provider registry/policy/provenance`. `FrankenCore` itself is an
integration/reference area containing small neutral Contracts, Language,
Packages, Policy, Provenance, Requirements and Runtime libraries; it is not a
single replacement runtime.

## 2. Native C warehouse

### Header/interface culture

The system has 15,416 C headers (**OBSERVED**). Representative public headers
show a stable pattern:

* scalar typedefs, enums, macros and status codes are common;
* opaque handles are expressed as forward-declared structs (`sqlite3`, archive,
  parser and TLS contexts);
* buffer APIs pair pointers with explicit sizes;
* lifecycle is conventionally expressed as `init/open/create` and
  `end/close/free/destroy` functions;
* callbacks and function pointers are common in SQLite, libxml2, libarchive,
  TLS and media APIs;
* ownership, thread-safety and error behavior are mostly prose in headers and
  manuals, not discoverable from the signature alone.

Examples: `zlib.h` documents stream state and default-allocator thread safety;
`sqlite3.h` documents opaque connections, callbacks, error codes and concurrent
use modes; `archive.h` exposes opaque archives and open/read/write/close
callbacks; OpenSSL headers expose many opaque context types and callback hooks.
These are **OBSERVED** interface patterns, not Flow permissions.

### Discovery and linking

`pkg-config` 1.8.1 lists 321 packages. Selected records include zlib 1.3,
SQLite 3.45.1, libcurl 8.5.0, OpenSSL 3.0.13, libxml2 2.9.14, libarchive 3.7.2,
libffi 3.4.6, libpng 1.6.43, libjpeg 2.1.5, liblzma 5.4.5, Wayland 1.23.1,
ncurses 6.4.20240113, OpenBLAS 0.3.26, Qt6 6.4.2, GLib 2.80.0 and PulseAudio
16.1. A `.pc` file supplies include directories, link flags, dependencies and
version; it does not supply Flow effects, ownership or concurrency authority.

`ldconfig -p` confirms representative SONAMEs such as `libz.so.1`,
`libsqlite3.so.0`, `libcurl.so.4`, `libssl.so.3`, `libxml2.so.2`,
`libarchive.so.13`, `libwayland-client.so.0`, `libpipewire-0.3.so.0`,
`libasound.so.2` and `libncurses.so.6`. Shared objects expose architecture,
SONAME, dependencies and exported symbols through ELF tools. `readelf` on the
selected objects showed x86-64 ELF and expected dependency graphs; `nm` showed
versioned and named symbols. **UNKNOWN from binaries:** parameter types,
ownership, effects, resource aliasing, failure policy and thread safety.

## 3. Native C++ warehouse

C++ is present through g++/clang++, LLVM 13/14/17/18 headers and CMake exports,
Qt5/Qt6, OpenBLAS and the project’s own C++ bricks. Boost, Eigen, fmt, spdlog
and OpenCV headers were not found in the sampled standard include paths
(**NOT PRESENT in this local sample**), so no claims are made about those
libraries. The installed CMake surface
contains 294 package/config/target files (**OBSERVED**), including LLVM/Clang,
Qt5/Qt6, OpenBLAS, PulseAudio and Catch2. CMake imported targets can express
target names, include directories, transitive link dependencies, compile
definitions and configuration/version constraints; this is richer discovery
evidence than a bare library filename, but it remains discovery rather than
semantic authority.

The C++ interface culture differs from C in material ways:

| Concern | C | C++ |
| --- | --- | --- |
| API discovery | headers, `.pc`, SONAME/symbols | headers, namespaces, CMake targets, sometimes `.pc` |
| Type identity | typedef/struct/enum names and ABI | namespaces, classes, templates, overload sets |
| ABI stability | comparatively stable C calling convention | compiler/stdlib/configuration-sensitive; Qt and LLVM publish their own policies |
| Generic programming | macros/conventions | templates/header-only instantiation and inline code |
| Resource ownership | documented create/free conventions | RAII, smart pointers, parent/child ownership and exceptions |
| Error handling | status/errno/null/out-parameters | status plus exceptions/error objects/contracts |
| Version metadata | `.pc`, macros, SONAME | CMake package/version files plus headers/ABI policy |
| Binding difficulty | scalar/opaque C APIs are tractable | C adapter is easy; native classes/templates are hard/very hard |
| Flow provider suitability | strong stable ABI candidate | use C ABI/adapters unless a dedicated runtime bridge is justified |

The practical rule is therefore `Flow -> stable C-compatible adapter/provider ->
C++ implementation` for Boost/Qt/LLVM-style systems. Direct binding of arbitrary
C++ templates or classes would turn compiler configuration into Flow semantics.

## 4. Other locally available ecosystems

### Rust

Rust 1.91.1 and Cargo 1.91.1 are installed. The local registry contains 817
`Cargo.toml` packages (**OBSERVED**), including serde, tokio, rayon, anyhow,
thiserror, clap and bindgen. Cargo manifests/locks expose package identity,
versions, features and dependency graphs more richly than ELF, while ownership,
traits and error types are encoded in Rust source/types. A Rust-native API has no
stable general ABI; an exported C ABI or explicit provider contract is the safe
Flow boundary (**INFERRED** from Rust’s ABI model and current Flow policy).

### Python

Python 3.12.3 is installed with 179 distribution records. Examples include
requests 2.31.0, cryptography 41.0.7, lxml 5.2.1, Pillow 10.2.0, NumPy 1.26.4
and pytest 7.4.4. `dist-info`/`egg-info` supplies versions, licenses and
dependencies; imports and optional native extension modules are discoverable.
Dynamic object protocols and runtime behavior are not a stable native contract.
Python is best treated as a managed provider/runtime bridge, not as a C ABI
library (**INFERRED**).

### JVM

OpenJDK 21.0.12 is installed with 16 local JARs, including ANTLR runtimes,
ICU4J, PDFBox and JSON-P. `MANIFEST.MF` records implementation/specification
versions and package exports where supplied. A JVM provider is a runtime bridge
with class/module/dependency metadata, not a native library binding.

### Go, .NET, Zig, Mojo and Kotlin

Go, dotnet, zig, mojo and kotlinc were not present (**NOT PRESENT**). Their
ecosystems were not inspected and no claims are made about them.

## 5. Interface-shape taxonomy

The warehouse repeatedly uses these shapes:

1. pure scalar function;
2. scalar plus status/error code;
3. string/`const char*` view;
4. buffer plus length;
5. opaque handle with create/use/release;
6. mutable stream/state struct;
7. aggregate struct with provider-owned layout;
8. callback/function pointer;
9. iterator/event loop;
10. file descriptor or process-global state;
11. C++ class/RAII object;
12. template/header-only API;
13. managed runtime object;
14. asynchronous operation/future.

The most reusable first carriers are scalar, string, buffer-plus-length and
opaque-handle contracts. Callbacks, aggregate ABI, unions, variadics, C++
objects and managed objects need separate evidence and should remain explicit
unsupported or provider-specific until a real application requires them.

## 6. ABI/carrier pressure map

| Carrier | Local frequency | Current Flow/Flowbind position | Binding pressure |
| --- | --- | --- | --- |
| integer/float/enum | high | scalar subset is strong; float coverage narrower | low for ordinary C math |
| string | high | `c_string`, bounded provider-local NUL view | low/moderate; encoding/lifetime remain explicit |
| buffer + length | very high | safe `list<int>` materialization exists for filesystem; general ABI carrier is narrow | **high** |
| opaque handle | very high | metadata and selected calls exist; no general lifetime proof | **high** |
| struct/aggregate | high | layout manifests exist; aggregate calls still blocked | **high** |
| callback | common in SQLite/curl/XML/UI/media | not generally admitted | high, but expensive |
| union/variadic/object/template | common in selected APIs/C++ | not general | defer until concrete need |

This is a qualitative frequency/pressure map based on sampled headers and
current Flowbind coverage, not a scientific census.

## 7. Resource, effect, error and concurrency patterns

### Resource contracts

`open/close`, `create/free`, `init/end`, `connect/disconnect` and
`retain/release` dominate opaque APIs. SQLite (`sqlite3_open`/`sqlite3_close`),
curl (`curl_easy_init`/`curl_easy_cleanup`), zlib stream init/end, archive
handles and Wayland proxies are representative. The signatures identify a
likely lifecycle, but the provider contract must own identity, cleanup,
nullability, aliasing and concurrency. Automatic lifetime proof is **UNKNOWN**
for the current Flow binding surface.

### Error models

The warehouse uses `errno`, negative returns, null pointers, integer status
codes/enums, out-parameters, callback error channels, and OpenSSL-style error
queues. Adapters may normalize these into explicit Flow outcomes, but the
foreign convention must remain provenance evidence. A symbol name or return
type alone does not establish failure semantics.

### Concurrency contracts

Some headers document thread safety (zlib’s default allocator case and SQLite
thread modes), while GUI, terminal, media and event-loop APIs impose
thread/dispatch restrictions. For most libraries the local survey could not
prove a general concurrency contract. Flowparallel should therefore classify
provider concurrency as **UNKNOWN** unless a reviewed provider manifest states
otherwise.

## 8. Candidate provider shortlist

The shortlist is in the machine-readable inventory with versions, discovery,
interface shape, resource/error/concurrency notes, current Flowbind coverage and
qualitative action. The highest-value near-term candidates are:

* **BIND SOON:** libm, because pure scalar ABI is simple and directly extends
  the existing `std.math` evidence.
* **USE EXISTING PROVIDER:** libc for filesystem/process primitives, ncurses
  through the existing terminal/provider surface, libcurl through a provider
  rather than a new HTTP stack, Qt/LLVM through dedicated C++/runtime adapters.
* **INVESTIGATE:** zlib, SQLite, libarchive and OpenBLAS, because they exercise
  buffer/state/opaque-handle carriers and expose the next real contracts.
* **DEFER:** OpenSSL, libxml2, libffi, Wayland, PipeWire/PulseAudio/ALSA,
  OpenCL and image codecs until callbacks, aggregate/resource and provider
  concurrency evidence justify the cost.

The recommendation is deliberately many-to-many: one library can satisfy many
capabilities, and one capability can have several providers.

## 9. Standard-library implications

Real substrate suggests these future semantic layers:

* `std.config` over ConfigResolve/EnvVar/ArgsLib;
* `std.text` and `std.bytes` over TextLib and safe byte carriers;
* `std.fs` and `std.process` over libc/system providers;
* `std.compress` over zlib/libdeflate/libarchive;
* `std.db` over SQLite/provider contracts;
* `std.net`/`std.http` over curl or socket providers;
* `std.crypto` over OpenSSL or another reviewed provider;
* `std.ui`/`std.terminal` over Flowterminal/ncurses/Qt/Wayland;
* `std.math` over libm/OpenBLAS/LLVM intrinsics.

These are semantic API candidates, not implementation tasks in this scouting
checkpoint. A standard API should own behavior while providers own mechanics.

## 10. Discovery and tooling implications

`pkg-config`, CMake package exports, Cargo manifests/locks, Python distribution
metadata and JAR manifests are useful discovery inputs. They can provide
identity, version, include/link/runtime dependencies, features and target
configuration. They cannot prove Flow capability semantics, ownership, effects,
resource aliasing, failure behavior or concurrency. A future provider-discovery
tool can ingest them and emit a candidate inventory; explicit binding contracts,
policy and evidence must remain the authority.

Flowbind’s current Clang-backed `tools/flowbind-gen` already covers a valuable
first C subset: scalar carriers, enums, function declarations, opaque handles
and explicit resource metadata. It marks unsupported declarations as partial.
The next measured additions should be chosen from a real zlib/SQLite/buffer
experiment rather than from a universal C parser or automatic C++ binder.

## 11. Architecture risks

* treating library identity as capability identity;
* treating an exported symbol or `pkg-config` record as a complete contract;
* assuming a C++ ABI is stable across compilers/configurations;
* inventing ownership from a pointer signature;
* assuming thread safety because an API is read-only by name;
* automatically binding enormous callback/template-heavy APIs;
* duplicating ConfigResolve/ArgsLib/TextLib/AiLib instead of adapting them;
* allowing discovery metadata to authorize execution;
* collapsing managed-runtime providers into native ABI assumptions.

## 12. Bare-metal/provider replacement

Today’s userland providers can later be replaced by a custom Flow-native,
kernel, userspace, bare-metal, remote or test provider if they satisfy the same
capability contract. Target policy selects the provider and deployment envelope;
the semantic API, effect/resource declarations and provenance remain stable.
This is an architectural compatibility property, not a claim that any current
provider runs bare metal.

## 13. Recommended next scouting/build steps

1. **BIND SOON:** run one libm experiment through the existing Flowbind
   generator and std.math contract; verify linked/dynamic parity and pure-effect
   evidence.
2. **INVESTIGATE:** run a small zlib or SQLite experiment to decide whether the
   next highest-value carrier is buffer-plus-length or opaque handle lifecycle.
3. **COMPARE DISCOVERY:** build a disposable inventory adapter that consumes one
   `.pc` file and one CMake imported target, then emits candidate metadata while
   keeping authorization separate.
4. **PRESERVE INTERNAL BRICKS:** perform focused clean-build audits of ArgsLib,
   EnvVar, TextLib and ConfigResolve before proposing Flow adapters; current
   realm evidence is useful but several claims were not rerun in this checkout.
5. **DEFER:** do not start callbacks, C++ template binding, GUI/runtime bridges,
   or broad aggregate layout work until a real application produces a P0/P1
   grievance.

## Final answers

**How much useful substrate do we already possess before writing anything new?**

Quite a lot: a complete contract-driven Flowcore chain, a tested Flowmini
language/runtime, provider/binding/lowering tools, a terminal and package
substrate, and a surrounding realm with mature configuration, environment,
arguments, text, AST, symbol and AI bricks. The host adds a broad, versioned C
and C++ warehouse plus Rust, Python and JVM ecosystems. The limiting factor is
semantic binding coverage and evidence, not absence of machinery.

**What is the smallest set of contracts, bindings and carriers that would make
the largest portion usable from Flowmini?**

1. Keep the existing versioned capability/provider contract and add discovery
   inputs from pkg-config/CMake without treating them as authority.
2. Finish the safe **buffer + length / byte collection** carrier.
3. Strengthen the descriptive **opaque handle create/use/release** contract
   with identity, cleanup and conservative concurrency facts.
4. Add verified **aggregate/struct layout** calls only where a concrete provider
   needs them.
5. Retain scalar, string, enum and status/error carriers as the stable C ABI
   base; route C++/Rust/Python/JVM through explicit adapters or runtime
   providers.

That set is smaller and safer than a universal FFI. It unlocks libc, libm,
compression, database, networking and many system APIs while leaving callbacks,
managed objects and C++ templates behind deliberate provider boundaries.
