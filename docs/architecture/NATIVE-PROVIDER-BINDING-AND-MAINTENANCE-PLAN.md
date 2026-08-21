# Native provider binding and substrate maintenance plan

**Status:** implementation plan
**Scope:** Flowcore bindings to libraries and toolchains supplied by the host
**Principle:** the substrate supplies implementations; Flowcore owns contracts,
policies, provenance, and composition.

## 1. Target model

Each capability has one canonical Flowcore namespace and may have several
providers:

```text
Flowcore contract
    -> provider adapter
        -> provider evidence
            -> policy authorization
                -> tested lowering/runtime path
```

Providers are alternatives behind one contract. A Flowcore-owned provider,
OpenBLAS, CUDA, or another implementation may coexist without creating
competing public meanings.

Discovery never authorizes use. Selection never authorizes execution. Binding
and runtime use are separate policy decisions.

## 2. Provider classes

### Bind existing providers

These are complex, specialized, security-sensitive, or hardware-specific:

```text
flowcore.gui       GTK, Qt
flowcore.display   Wayland, X11, OpenGL, Vulkan
flowcore.gpu       CUDA, NVML, video encoders, PTX JIT
flowcore.audio     PipeWire, PulseAudio, ALSA, JACK
flowcore.network   libcurl, GnuTLS
flowcore.crypto    OpenSSL/libcrypto, GnuTLS
flowcore.terminal  ncurses
flowcore.data      SQLite, JSON, XML, archive, compression
```

Flowcore defines the capability contract and safety policy. The provider
adapter translates to the external ABI and preserves provider/version evidence.

### Flowcore-owned providers

These express Flowcore’s own semantic model and should be implemented natively:

```text
flowcore.math
flowcore.parallel
flowcore.policy
flowcore.provenance
flowcore.contracts
flowcore.collections
flowcore.text
flowcore.execution
flowcore.optimization
```

Existing math and parallel libraries may still be optional providers behind the
same contracts. Ownership of semantics does not prohibit reuse of optimized
kernels.

### Internal substrate adapters

Some host facilities should remain implementation details rather than public
bindings:

```text
pthreads       worker/runtime substrate
dynamic loader provider lookup
platform clocks and basic OS scheduling
```

The public capability remains Flowcore-owned; the implementation may use the
host mechanism without exposing its ABI as the canonical contract.

### External toolchain providers

LLVM and Clang are compiler infrastructure, not ordinary runtime libraries:

```text
Flowlower -> LLVM IR -> LLVM -> target object/ELF
```

Clang is used for header/ABI inspection, probe compilation, and
interoperability. Their versions and target triples belong in toolchain
requirements and build provenance.

## 3. Binding workflow

Every new provider follows the same gates:

### Gate A — inventory

Run `scan-native-bindings.sh` and retain the generated inventory as discovery
evidence. Record library SONAME, exact path, architecture, headers, and
development package metadata.

### Gate B — selection

Create or update the explicit native-binding selection artifact. Assign:

* Flowcore namespace;
* capability identity;
* strategy (`bind-existing`, `implement-flowcore`,
  `internal-substrate-adapter`, or `external-toolchain`);
* contract path and version;
* provider identity, when applicable;
* rationale and scope.

The selection validator must reject providers absent from the inventory.

### Gate C — contract

Define the provider-neutral Flowcore contract before writing the adapter:

* types and calling convention;
* ownership and lifetime;
* memory effects;
* determinism and external effects;
* errors and cancellation;
* capability requirements;
* version and compatibility rules.

The contract is the stable truth. Headers are evidence about one provider.

### Gate D — ABI evidence

Collect provider-owned evidence using headers, compiler probes, symbol tables,
and runtime-safe probes where appropriate. Verify:

* symbol existence;
* exact parameter and return layout;
* width, alignment, signedness, and offsets;
* calling convention and ABI target;
* provider version and SONAME;
* required transitive libraries.

`dlsym` alone is addressability evidence, not signature evidence.

### Gate E — adapter

Implement a narrow adapter that translates the Flowcore contract to the
provider ABI. Keep provider-specific details inside the adapter namespace.
Do not leak GTK, CUDA, OpenSSL, or other provider types into portable Flowcore
contracts unless the contract explicitly represents that provider identity.

### Gate F — authorization

Add an explicit policy grant binding the capability to the provider contract,
signature evidence, ABI target, effect set, and contract version. Default is
fail-closed. Selection alone is never a grant.

### Gate G — tests and artifacts

Require, as applicable:

* manifest and schema validation;
* positive and hostile ABI tests;
* provider-unavailable tests;
* wrong-version and wrong-architecture tests;
* runtime behavior tests;
* fallback-provider tests;
* sanitizer and Valgrind coverage;
* exported capability and provenance artifacts;
* a complete lower-to-executable smoke test for executable providers.

## 4. Initial implementation order

### Tier 1 — complete the substrate boundary

1. libc/Linux file, process, memory, time, and basic networking contracts;
2. dynamic loading and provider discovery;
3. ncurses terminal display provider;
4. PipeWire audio provider with ALSA/PulseAudio alternatives;
5. libcurl and GnuTLS network providers;
6. SQLite, compression, JSON, and XML providers.

### Tier 2 — external toolchain and computation

1. LLVM lowering provider and target-triple requirements;
2. Clang header and ABI inspection provider;
3. Flowcore-owned scalar/vector/matrix contracts;
4. CPU-native math provider;
5. optional BLAS/OpenBLAS provider;
6. optional CUDA/cuBLAS provider;
7. runtime cost-based provider selection.

### Tier 3 — desktop and hardware projections

1. GTK provider;
2. Qt provider;
3. Wayland provider;
4. X11 compatibility provider;
5. OpenGL and Vulkan providers;
6. CUDA/NVML/video/encoder/PTX providers.

Each tier remains usable if later tiers are unavailable. No graphical or GPU
dependency may become a requirement for the base Flowcore toolchain.

## 5. Provider selection at runtime

The runtime chooses among authorized providers only. The decision inputs are:

```text
contract compatibility
provider availability
ABI/architecture match
policy and trust state
operation size and estimated cost
determinism/effect requirements
fallback availability
```

For example:

```text
flowcore.math.linear
    -> Flowcore CPU reference
    -> optimized CPU provider
    -> OpenBLAS
    -> CUDA/cuBLAS
```

The reference provider is retained for correctness comparison and fallback.
Provider choice is observable in provenance and must explain why a provider was
selected or rejected.

## 6. Substrate change detection

Maintenance is a recurring reconciliation process, not a one-time scan.

On inventory refresh, compare the new evidence with the last accepted
inventory by:

* SONAME and resolved path;
* architecture and target ABI;
* file digest and package ownership;
* provider version;
* exported symbols;
* header and development-package versions;
* transitive dependency graph.

Classify changes as:

```text
unchanged
compatible patch/minor change
new provider candidate
provider replacement
ABI-affecting change
provider removed
verification failure
```

Only compatible changes may remain automatically usable, and even those must
produce a recorded diagnostic. ABI-affecting, removed, or unverifiable
providers become unavailable until their binding gate is rerun.

## 7. Binding maintenance records

Each accepted provider binding should retain:

```text
Flowcore contract/version
provider name and SONAME
provider version
architecture and ABI target
header/package provenance
symbol and signature evidence
adapter revision
policy grant revision
test evidence
last verified timestamp
```

The record follows the project. A user-local catalogue may index it for
discovery, but the project artifact and provenance history remain authoritative.

## 8. Failure and recovery policy

If a provider changes incompatibly:

1. mark the provider binding degraded or unavailable;
2. preserve the last known-good binding record;
3. report the exact drift and failed evidence;
4. select an authorized fallback if policy permits;
5. otherwise stop the affected operation explicitly;
6. require a conscious repair, rollback, or re-authorization.

The system must never silently force a new ABI, ignore a failed signature
check, or replace a provider merely because a similarly named library exists.

## 9. Deliverables

The implementation sequence is:

1. stabilize inventory and selection schemas;
2. add provider-neutral contract metadata;
3. add reusable ABI-evidence records;
4. add one adapter template per provider class;
5. convert the current libc boundary to the full record format;
6. implement Tier 1 providers one capability at a time;
7. add drift comparison and diagnostics;
8. add provider fallback and runtime selection;
9. maintain reference providers and hostile regression fixtures;
10. publish supported-provider matrices per platform and release.

The result is additive: the host can evolve, Flowcore can improve its own
implementations, and both can coexist behind explicit contracts and policies.
