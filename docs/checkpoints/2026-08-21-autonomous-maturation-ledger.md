# Flowcore autonomous maturation ledger

## Completed in this continuation

- Baseline audited: current v25 imports already preserve explicit aliases such
  as `curses`, `libc`, and `linux` in AST/SymbolTable projections; the existing
  compatibility path for unaliased legacy imports remains to be tightened.
- Generated binding policies now carry exact declared parameter and return
  carriers. Flowbind accepts the shorter four-field policy as an explicit
  legacy wildcard and binds generated grants exactly.
- Added a hostile generated-signature-policy test; it is rejected.
- Flowbind now parses semantic reports and ABI manifests as structured JSON,
  rejects duplicate JSON keys, and compares aggregate size, alignment, field
  order, names, types, and offsets against semantic facts.
- Runtime graph connections now preserve destination ports and stable per-build
  wire identities in envelope routing metadata.
- Added deterministic graph-routing coverage for destination-port delivery and
  wire identity traces.
- Added the first `flow_less` application slice. `pager.fake` is a deterministic
  provider with policy-controlled page size and navigation; `pager.render` is a
  provider-neutral plain projection. Invalid page sizes fail closed.
- Added `pager.ncurses` as a separate dynamically loaded provider using the same
  page-record contract. Its pseudo-terminal test verifies `q` handling and the
  final plain projection without linking Flowmini directly to ncurses.
- Flowanalyst now exports `abi_type_contracts`, preserving provider-declared ABI
  carrier representation, ownership, access, lifetime, nullability, and
  opacity facts. The ncurses pipeline asserts the typed opaque-window carrier.
- `flow_less` now reads a real temporary text file through `pager.file.ncurses`,
  renders its first page in a pseudo-terminal, and rejects a missing path.
- Flowanalyst now emits an additive `flowcore.lowering_plan` v1 containing
  report-local call operations and exact provider/signature facts.
- Flowbind validates every external operation in that plan against semantic
  requirements before authorizing the binding.
- Flowparallel and Flowoptimize preserve the structured lowering plan.
- Added the first profile-free native proof: `profile_free_getpid` uses an
  arbitrary program name and generic zero-argument `c_int` lowering to emit and
  execute an ELF binary without a matching source-name profile.
- Lowering-plan operations now export typed operand descriptors for integer
  literals and identifiers. A second arbitrary profile-free proof generates a
  `getpgid(c_int)` binding, preserves its operand through Flowparallel and
  Flowoptimize, and executes the resulting ELF without a compiler change.
- Flowparallel and Flowoptimize no longer enumerate known profile names to
  preserve a report's lowering profile; they mechanically carry the declared
  value and structured lowering plan forward.
- Migrated the existing `getpid` native example to the generic lowering path:
  its source name is now arbitrary, the analyst emits no special profile, and
  the dedicated getpid lowerer dispatch was removed. Existing corpus and native
  execution checks still pass.
- Added a namespace adversarial gate with three independent providers exposing
  the same function name. The unqualified call is rejected with provenance-aware
  ambiguity diagnostics, while three explicit aliases compile to three distinct
  qualified operations.
- Added generic profile-free `return_value` lowering for typed integer literals.
  The plan carries the value through both intermediate stages and Flowlower
  emits an executable ELF whose exit status is 42.
- Extended generic return lowering to nested integer binary expressions. A
  profile-free `40 + 2` program now produces an LLVM `add` and executes with the
  expected result.
- Added generic local value flow: `let` initializers become
  `value_definition` operations with symbol identity, and return expressions
  can consume those values through the plan. The profile-free local-value ELF
  test executes successfully.
- Added generic boolean branch operations with explicit then/else block IDs.
  A profile-free conditional-return program now lowers to LLVM branches and
  executes the selected return path.
- Extended branch conditions to source-derived integer comparisons. Local SSA
  values can now feed an `icmp` predicate in a profile-free conditional ELF.
- External `c_int` call results now retain their result-symbol identity and can
  feed later generic return operations. LLVM emission requires a ready binding
  report containing the authorized symbol; missing and mismatched reports are
  rejected.
- Migrated the getuid native example to this result-placement path. Its program
  name is arbitrary, its lowering profile is `none`, and the handwritten
  getuid profile branches were removed from analysis, binding, and lowering.
- Generic external results can now feed subsequent integer expressions. The
  arbitrary getppid acceptance program emits a call, adds one to its result,
  and returns the derived SSA value.
- Migrated the existing getppid native example to the generic result path and
  removed its source-name dispatch from all three compiler stages.
- External result symbols can now drive generic comparison branches. An
  arbitrary getppid program emits the authorized call and `icmp`, takes the
  source-defined true branch, and exits 42.
- Migrated the remaining zero-argument `c_int` identity examples (`getgid`,
  `geteuid`, `getegid`, and `getpgrp`) to the generic plan and removed their
  analyst, binder, and LLVM profile branches. Their previously implicit result
  behavior is now expressed by explicit Flow return statements.
- Migrated the one-argument `getpgid` and `getsid` examples to the same generic
  path. Generic external operands can now consume initialized local symbols,
  with initializer, call, and result-dependent expression instructions emitted
  in dependency order.
- Generic `c_int` result calls now accept an arbitrary non-empty sequence of
  `c_int` literal or initialized-local operands, derive the LLVM declaration
  and call argument lists from the exact plan signature, and reject mismatched
  carrier lists. `getpriority(c_int,c_int)` is migrated to this path with its
  native return behavior expressed in Flow source.
- The generated `gettid` acceptance fixture now retains `lowering_profile:
  none`; its binding is generated at test time, exactly authorized, lowered by
  the generic zero-argument `c_int` machinery, linked, and executed without a
  compiler profile branch.
- Added generic profile-free `c_long` external-call lowering with source-derived
  `c_int` arguments, exact binding authorization, positive-result validation,
  and structured refusal outside the supported carrier shape. The generated
  `sysconf` fixture now uses that path and has no source-name profile branches.
- Added profile-free `c_ulong` argument/result lowering with carrier-derived
  64-bit local initialization and unsigned result validation. The generated
  `getauxval` fixture now compiles and executes without source-name dispatch.
- Replaced the generated system-information profile with generic ordered scalar
  capability-sequence lowering. An arbitrarily named program now carries five
  zero-argument `c_int`/`c_long` calls through the plan, requires authorization
  for every native symbol, and emits and executes without a handwritten LLVM
  block or source-name selection.
- Added generic unary integer operand propagation and migrated the libc `abs`
  example off its source-name profile. Its `-42` initializer, result placement,
  and explicit return now drive the emitted LLVM and native exit status 42.
- Added generic ASCII string-literal storage, `c_string` argument flow, and
  `c_size_t` result-to-return lowering. The libc `strlen` example now returns 8
  from explicit Flow source with no source-name selection or handwritten LLVM.
- Added generic nullable `c_string` result flow and branch-local string-call
  lowering. The generated getlogin/puts proof now has an arbitrary program
  name, expresses its null fallback as a Flow `if`, retains
  `lowering_profile: none`, and has no analyst, binder, or LLVM profile branch.
- Generalized ordered capability sequences across `c_string`, `c_int`,
  `c_long`, `c_ulong`, `c_size_t`, and pointer LLVM carriers. The libc
  integration fixture now emits its source-defined `strlen`, `abs`, and `puts`
  sequence generically and no longer has compiler profile dispatch.
- Admitted single mixed-carrier calls on the same generic path and migrated
  `rmdir(c_string)`. Its emitted call now uses the Flow source literal instead
  of the legacy handwritten emitter's null pointer.
- Migrated the remaining scalar-only kernel fixtures (`fork`, `socket`,
  `listen`, and `unshare`) to generic single-operation lowering. Their exact
  `c_int` operands now come from Flow initializers and their source-name profile
  branches are removed.
- Migrated `sethostname` and all remaining explicit-null `c_pointer` kernel
  fixtures to generic mixed-carrier lowering. `c_pointer(0)` is represented as
  LLVM `null`; nonzero integer-to-pointer conversion remains unsupported rather
  than guessed. The corresponding source-name profile table and handwritten
  emitters are removed.
- Migrated `openat`, `lseek`, and `unlinkat` to generic source-derived calls and
  removed their analyst, binder, and LLVM profiles. A trial migration of all
  remaining kernel profiles correctly exposed that `clock_gettime` cannot write
  through the source-declared null pointer; the five buffer-writing profiles
  were retained pending explicit storage semantics rather than hiding an
  application-specific allocation in generic lowering.
- Migrated the safe explicit-null probes `getrandom`, invalid-descriptor `read`,
  and invalid-descriptor `write` to generic lowering. Only `clock_gettime` and
  `uname` remain among the kernel compatibility profiles because successful
  execution requires sized writable storage absent from their current Flow
  declarations.
- Added source-derived writable-storage descriptors for positive `c_pointer(N)`
  initializers. The lowering plan records exact byte count, read/write access,
  and call lifetime; Flowbind rejects malformed or zero-sized descriptors.
- Migrated `clock_gettime` and `uname` to generic mixed-carrier lowering using
  explicit 16-byte and 390-byte source allocations. Their analyst, binder, and
  handwritten LLVM profile branches are removed, and both native ELFs execute.
- Replaced the imported short-name toggle with a permanent ambiguity set. One
  provider retains the diagnosed compatibility alias; two, three, and four
  colliding providers remain ambiguous, while four qualified calls remain
  stable and distinct.
- Repaired the transitional `sel` native emitter's input handling. Flow source
  declares 4096 bytes of writable storage, the read reserves one byte for the
  terminator, and LLVM distinguishes positive reads, EOF, and errors before
  using the result as an offset. The error path restores ncurses and exits 2.
- Added ABI type cleanup capability facts and exact provider contract identity
  to lowering-plan operations. Ncurses pointer results now retain external
  ownership, opaque access, external lifetime, nullability, and `endwin`
  cleanup identity through the middle stages.
- Flowbind rejects mutated resource facts and resource acquisition without the
  declared cleanup operation. The standalone ncurses example now lowers as a
  generic ordered mixed-carrier sequence; its source-name profile and
  handwritten LLVM emitter are removed.
- Removed the empty-program lowering profile. A valid version-1 generic plan
  with zero operations now emits a minimal native `main`, including independent
  explicitly selected target artifacts.
- Added stable runtime signal identities distinct from wire identities. Each
  output activation creates one signal, fan-out deliveries retain it across
  distinct wires, and traces preserve full source/destination port identity.
- Expanded graph coverage for deterministic fan-out order, multiple destination
  ports, unconnected-output diagnostics, and invalid-port contract rejection.
- Removed the `sel` source-unit/profile selector from Flowanalyst and Flowlower.
  Its current terminal slice is now selected from a profile-free structured
  plan containing the required external operations, and every operation is
  checked against the ready binding report. An arbitrarily renamed copy follows
  the same path, while a binding with a mutated `wgetch` identity is rejected.
- Added a generic `length(list<string>)` lowering operand for parameterized
  entry points. A profile-free arbitrary program now branches on its actual
  native argument count and produces distinct tested exit statuses with and
  without an application argument.
- Added generic checked `list<string>` indexing for parameterized entry points.
  The plan retains the source parameter and index expression; native lowering
  guards argc before loading argv. An arbitrary profile-free program prints its
  source-selected argument, while the missing-argument path exits 64 without
  dereferencing argv out of bounds.
- Made the transitional profile-free `sel` backend contingent on source-derived
  entry-argument and key-selection branches. The plan must contain an argv
  length definition, checked argv indexing, a branch consuming the length
  symbol, and an equality branch for the quit key; the capability set alone is
  no longer sufficient to select the terminal emitter.
- Removed the final flowcat application/source-name profiles. Flow source now
  declares argv traversal, bounded writable storage, open/read/write/close
  calls, error branches, index mutation, and returns. Flowanalyst publishes
  generic `loop` and `assignment` operations and resolves placement results
  through parent scopes; Flowlower selects the transitional file-copy emitter
  from those structured facts and exact authorized capabilities.
- Added explicit ABI carrier conversion operations for typed initializers.
  Flowcat now converts the `c_long` read result to its declared `c_size_t`
  write count in the plan, and Flowbind verifies every external operand count
  and carrier type against the exact provider signature.
- Added reusable profile-free integer loop and mutation lowering. A previously
  unknown program initializes local `c_int` values, evaluates its structured
  loop comparison on every iteration, applies a source assignment through
  mutable storage, and returns the final value without any source-name or
  capability selector.
- Removed the last source-unit exception from Flowanalyst capability discovery
  and the obsolete `sel_main`/`abi_kernel_getpid_main` branches from Flowbind.
  Requirements are now derived only from actual calls, binding reports describe
  the versioned plan as generic, and Flowlower public wording no longer calls
  accepted plans profiles.
- Removed the transitional terminal capability-set recognizer and fixed LLVM
  emitter. Flowlower now parses the optimization plan and binding report as
  typed JSON, authorizes every external operation by its exact provider and ABI
  tuple, and emits values, calls, nested branches, conversions, cleanup, and
  returns in source statement/block order.
- Made `sel` read behavior source-derived. Flow source explicitly initializes
  its compatibility buffer through authorized `memset`, branches on negative,
  zero, and positive read results, performs `endwin` cleanup before returning 2
  on failure, writes only a positive byte count, and returns the selected or
  cancelled status from its key branch.
- Added adversarial terminal proofs for different behavior under the same
  capability set, changed source operation order, an unused policy grant, and
  renamed source/program identity. No compiler path is selected by those facts.
- Documented the temporary `c_pointer(N)` writable-allocation interpretation
  and the explicit public-language choice among bounded buffer, storage
  declaration, and allocation-operation designs.
- Extended typed structured lowering with nested loops, assignments, dynamic
  argv indices, integer promotion/conversion, loop-carried external results,
  cleanup branches, early returns, and reachability validation. Removing a
  controlling loop now rejects the plan instead of silently dropping child
  blocks.
- Removed the transitional file-copy capability recognizer and handwritten LLVM
  emitter. `flowcat` now uses only generic typed-plan machinery and an explicit
  Flow `sendfile` loop; short transfers advance the kernel-managed input offset
  and continue without invented pointer arithmetic.
- Added `sendfile` to the provider contract and exact policy boundary without
  adding any compiler dispatch. The arbitrary renamed copy and a two-megabyte
  multi-iteration native transfer pass with the already-built toolchain.
- Added typed call-site effect contracts to every external lowering operation.
  They retain the declared external effect, declared certainty, determinism,
  and one exact argument-resource record per ABI parameter with memory effect,
  ownership, access, lifetime, nullability, and opacity.
- Flowbind now verifies those effect and argument-resource facts against the
  provider declaration and exported ABI type contract. Adversarial mutations
  of `sendfile` determinism and pointer memory access are rejected.
- Replaced ncurses' generic `c_pointer` handle with a distinct
  `ncurses_window` ABI carrier. Its external ownership, opaque access, external
  lifetime, nullability, and `endwin` cleanup identity now remain distinct from
  ordinary pointer authority at acquisition and every window-consuming call.
- Flowparallel and Flowoptimize now preserve ABI type contracts alongside the
  lowering plan. Flowbind and typed Flowlower accept provider-declared pointer
  carriers from their exact `repr` contract instead of adding the ncurses type
  name to compiler dispatch. Hostile lifetime and cleanup mutations are
  rejected, and both the standalone ncurses ELF and `sel` remain executable.
- Flowbind now validates acquired-resource cleanup path-sensitively across
  structured branches. Every reachable exit after `initscr` must execute
  exactly one contract-matched `endwin`; cleanup before acquisition, repeated
  acquisition, double cleanup, and resource actions in loops without an
  explicit lifetime proof are rejected. Adversarial `sel` plans cover an early
  error exit with missing cleanup and an ordinary path with duplicate cleanup.
- Removed Flowlower's 750-line substring-based compatibility parser and all
  fallback LLVM emitters. A typed JSON driver now validates optimization report
  identity, version, status and target selection, while the structured-plan
  emitter handles empty plans, scalar calls, nullable pointers, values,
  branches, checked argv indexing, loops, assignments, cleanup and returns.
  Missing loop bodies and controlling blocks are rejected instead of silently
  emitting altered behavior.
- Removed the transitional `lowering_profile` field from Flowanalyst, Flowbind,
  Flowparallel and Flowoptimize. Tests now assert the versioned lowering-plan
  contract and source-derived operations directly; no required compiler stage
  contains or consumes profile vocabulary.
- Atom contracts now distinguish required inputs, optional activation inputs
  and terminal nodes. Graph validation rejects an unconnected required sink
  input, accepts either optional port of the routing probe, and rejects a
  terminal contract that exposes outputs. Runtime node failures retain the
  receiving node/input plus wire and signal identity in diagnostics before the
  original failure propagates.
- `flow_less` no longer uses the bundled fake pager implementation. Its Flow
  source explicitly connects `pager.input.fake => pager.navigate =>
  pager.render => halt.record`; the input provider emits raw lines, commands
  and page size, while the provider-neutral navigation node owns page-state
  transitions. Source-selected command order changes observable output and an
  unknown command fails with navigation-node wire/signal provenance.

## Evidence

- Focused binding checkpoint: `flowbind_provider`, `flowcore_stdlib_boundary`,
  and `native_binding_generation` passed.
- Focused graph checkpoint: `flowcore_graph_routing` passed.
- Current complete checkpoint: **54/54 CTest tests passed** after the namespace
  ambiguity gate; the profile-free comparison-branch lowering gate is also
  green and its native ELF returned the expected status 42.
- Focused identity migration checkpoint: `profile_free_generic_lowering` and
  `flowlower_pipeline` passed, including native observable-result checks for all
  six newly migrated identity examples.
- Focused multi-operand checkpoint: `profile_free_generic_lowering`,
  `flowbind_provider`, and `flowlower_pipeline` passed, including the native
  `getpriority` result assertion.
- Focused generated-binding checkpoint: `native_binding_generation`,
  `flowbind_provider`, and `flowlower_pipeline` passed.
- Focused `c_long` checkpoint: `native_binding_generation` and
  `flowlower_pipeline` passed, including compilation and execution of the
  generated `sysconf` ELF.
- Focused `c_ulong` checkpoint: `native_binding_generation`,
  `profile_free_generic_lowering`, and `flowlower_pipeline` passed.
- Focused ncurses checkpoint: `ncurses_flow_pipeline`, `flow_less_pager`, and
  `flow_less_ncurses_pager` passed.
- Focused ordered-sequence checkpoint: `native_binding_generation`,
  `profile_free_generic_lowering`, `flowbind_provider`, and
  `flowlower_pipeline` passed. The complete canonical suite then passed
  **54/54** tests.
- Focused unary migration checkpoint: `flowlower_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, both pass-corpus gates, and
  `profile_free_generic_lowering` passed. The complete suite again passed
  **54/54** tests.
- Focused string/size migration checkpoint: the same six focused gates passed,
  followed by the complete canonical suite at **54/54**.
- Focused nullable-string checkpoint: `native_binding_generation`,
  `profile_free_generic_lowering`, `flowbind_provider`, and
  `flowlower_pipeline` passed, including hostile signature-policy rejection and
  execution of the generated ELF. The complete canonical build and suite then
  passed **54/54**.
- Focused mixed-carrier checkpoint: `flowlower_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, `native_binding_generation`,
  `profile_free_generic_lowering`, and both pass-corpus gates passed. Native
  stdout remained exactly `Flowcore libc bindings`; the complete canonical
  build and suite then passed **54/54**.
- Focused rmdir checkpoint: `flowlower_pipeline`, `flowanalyst_pipeline`,
  `flowbind_provider`, `profile_free_generic_lowering`, and both pass-corpus
  gates passed. The native ELF executed, the LLVM call referenced the
  source-derived string global, and an adversarial assertion rejected the old
  null-pointer shape. The canonical build and suite passed **54/54**.
- Focused scalar-kernel checkpoint: `flowlower_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, `profile_free_generic_lowering`,
  and both pass-corpus gates passed, including all four native ELFs. The
  canonical build and suite passed **54/54**.
- Focused pointer-carrier checkpoint: `flowlower_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, `profile_free_generic_lowering`,
  and both pass-corpus gates passed, including native execution of all migrated
  fixtures. The canonical build and suite passed **54/54**.
- Focused final scalar/string kernel checkpoint: `flowlower_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, `profile_free_generic_lowering`,
  and both pass-corpus gates passed. Native `openat`, `lseek`, and `unlinkat`
  ELFs executed. The canonical build and suite passed **54/54**. The attempted
  generic `clock_gettime(c_int,c_pointer(0))` execution produced a segmentation
  fault, confirming that typed writable storage must be represented before its
  compatibility profile can be removed.
- Focused null-probe checkpoint: the same six focused gates passed, including
  native execution of `getrandom`, `read`, and `write`; the canonical build and
  suite passed **54/54**.
- Focused writable-storage checkpoint: `flowcore_pass_corpus`,
  `profile_free_generic_lowering`, `flowanalyst_pipeline`, `flowbind_provider`,
  and `flowlower_pipeline` passed, including native execution of
  `clock_gettime` and `uname` and rejection of a zero-byte descriptor. The
  canonical build and suite passed **54/54**.
- Focused namespace checkpoint: `namespace_ambiguity`, `flowanalyst_pipeline`,
  and both `flowcore_pass_corpus` registrations passed with one-through-four
  provider coverage.
- Focused `sel` read-safety checkpoint: `sel_tui_pipeline`,
  `flowlower_pipeline`, and both `flowcore_pass_corpus` registrations passed.
  The TUI gate covers positive input, EOF, and closed-stdin error behavior.
- Focused resource checkpoint: `ncurses_flow_pipeline`, `sel_tui_pipeline`,
  `flowbind_provider`, `flowlower_pipeline`, and both pass-corpus registrations
  passed. Adversarial cases reject invented cleanup identity and missing
  cleanup, and the profile-free ncurses ELF runs in a pseudo-terminal.
- Focused empty-plan checkpoint: `flowanalyst_pipeline`,
  `profile_free_generic_lowering`, `flowlower_pipeline`, and both pass-corpus
  registrations passed, including two separately attributed native targets.
- Focused graph-law checkpoint: `flowcore_graph_routing` and both pass-corpus
  registrations passed with wire/signal provenance and fan-out assertions.
- Focused profile-free terminal checkpoint: `sel_tui_pipeline`,
  `flowanalyst_pipeline`, `flowbind_provider`, `flowlower_pipeline`, and both
  pass-corpus registrations passed. The gate includes renamed-source and
  hostile-binding cases plus positive input, EOF, and read-error execution.
- Focused entry-argument checkpoint: `profile_free_generic_lowering` and
  `flowlower_pipeline` passed, including native execution of both argc branch
  outcomes.
- Focused checked-argv checkpoint: `profile_free_generic_lowering` passed with
  native selected-argument output and missing-argument refusal.
- Focused structured-terminal checkpoint: `sel_tui_pipeline` passed with an
  arbitrarily renamed source, explicit argument and quit-key branches,
  branch-removal refusal, hostile-binding refusal, native positive input, EOF,
  and closed-stdin error behavior. The canonical build and suite then passed
  **54/54** tests.
- Focused profile-free file-copy checkpoint: `flowanalyst_pipeline`,
  `flowbind_provider`, `flowcore_stdlib_boundary`, `flowcat_flowcore_pipeline`,
  and `flowlower_pipeline` passed. An arbitrarily renamed source emitted the
  same native path, removing loop operations was rejected, two files produced
  the expected output, and a missing file returned 1. The canonical build and
  suite passed **54/54** tests.
- Focused operand-contract checkpoint: `flowanalyst_pipeline`,
  `flowbind_provider`, and `flowlower_pipeline` passed. The semantic gate
  asserts the explicit `c_long` to `c_size_t` conversion and an adversarial
  write operation with a mutated operand carrier is rejected.
- Focused generic-loop checkpoint: `profile_free_generic_lowering`,
  `flowlower_pipeline`, `flowanalyst_pipeline`, and both pass-corpus
  registrations passed. The native arbitrary counter loop exited 4, its LLVM
  contains the reusable loop back-edge, and deleting the mutation operation
  caused structured refusal.
- Independent sanitizer checkpoint: configured `/tmp/flowcore-reusable-chain-sanitize`
  with `-fsanitize=address,undefined -fno-omit-frame-pointer`, built the complete
  tree, and ran `ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=detect_leaks=0 ctest
  --test-dir /tmp/flowcore-reusable-chain-sanitize --output-on-failure`.
  **54/54** tests passed, including malformed-input fuzzing, native linking,
  profile-free execution, terminal/resource, graph, and kernel gates.
- Focused generic-terminal checkpoint: `sel_tui_pipeline`,
  `flowlower_pipeline`, `native_binding_generation`, and
  `profile_free_generic_lowering` passed. The native pseudo-terminal gate
  covered argument input, positive stdin input, EOF, and closed-stdin failure;
  adversarial variants proved literal and operation-order sensitivity and
  unused-capability independence.
- Canonical post-terminal checkpoint: the complete build succeeded and
  **54/54** CTest tests passed in 19.35 seconds.
- Focused reusable file-loop checkpoint: `flowanalyst_pipeline`,
  `flowbind_provider`, `flowcore_stdlib_boundary`,
  `flowcat_flowcore_pipeline`, `flowlower_pipeline`,
  `profile_free_generic_lowering`, and `sel_tui_pipeline` passed. Native tests
  copied two small files and a two-megabyte file exactly, rejected a missing
  file, retained cleanup/error returns, and rejected a plan with removed loops.
- Canonical post-file-emitter checkpoint: the complete build succeeded and
  **54/54** CTest tests passed in 17.96 seconds.
- Focused typed-effect checkpoint: `flowanalyst_pipeline`,
  `flowbind_provider`, `sel_tui_pipeline`, and `flowlower_pipeline` passed,
  including hostile effect and argument-memory mutations.
- Canonical typed-effect checkpoint: the complete build succeeded and
  **54/54** CTest tests passed in 19.43 seconds.
- Focused typed-window checkpoint: `ncurses_flow_pipeline`, `sel_tui_pipeline`,
  `flowbind_provider`, `flowlower_pipeline`, `flowanalyst_pipeline`, and both
  pass-corpus registrations passed. The canonical build and suite then passed
  **54/54** tests in 19.58 seconds.
- Focused path-sensitive resource checkpoint: `flowbind_provider`,
  `ncurses_flow_pipeline`, and `sel_tui_pipeline` passed, including missing
  branch-cleanup and double-cleanup rejection. The complete canonical build and
  suite then passed **54/54** tests in 17.75 seconds.
- Focused typed-only lowering checkpoint: `flowlower_pipeline`,
  `profile_free_generic_lowering`, `native_binding_generation`,
  `ncurses_flow_pipeline`, `sel_tui_pipeline`, `flowcat_flowcore_pipeline`, and
  both pass-corpus registrations passed. The complete canonical suite passed
  **54/54** tests in 18.93 seconds after all legacy Flowlower emitters were
  removed.
- Profile-field removal checkpoint: the complete canonical build succeeded and
  **54/54** CTest tests passed in 18.46 seconds with no `lowering_profile`
  identifier remaining outside historical mission/ledger documentation.
- Focused connectivity/failure checkpoint: `flowcore_graph_routing`, both
  pass-corpus registrations, `flow_less_pager`, and
  `flow_less_ncurses_pager` passed. The complete canonical build and suite then
  passed **54/54** tests in 17.95 seconds.
- Focused Flow-owned navigation checkpoint: `flow_less_pager`,
  `flow_less_ncurses_pager`, `flowcore_graph_routing`, and both pass-corpus
  registrations passed. The complete canonical build and suite then passed
  **54/54** tests in 17.48 seconds.

## Remaining work

- The unqualified single-provider import alias remains explicitly transitional;
  selective-opening syntax is not yet part of the language.
- Split the ncurses compatibility provider's file/input and terminal behavior
  around the same provider-neutral `pager.navigate` stage used by the fake
  provider.
- Reconcile documentation and test counts after the implementation stabilizes.

## Exact next action

Next action: refactor the ncurses `flow_less` slice into explicit source input,
provider-neutral navigation and terminal projection stages without changing
its pseudo-terminal behavior.
