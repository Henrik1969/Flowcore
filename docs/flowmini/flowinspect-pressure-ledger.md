# flowinspect application-pressure ledger

This ledger records what the first useful artifact-inspection application
actually demonstrated. A pressure item is evidence only after an application
boundary was exercised; roadmap hypotheses are kept separate from observed
blockers.

## Artifact inventory

| Format | Version | Producer | Consumer/authority | Required evidence used by flowinspect |
| --- | --- | --- | --- | --- |
| `flowmini.frontend_bundle` | 2 | Flowmini structural frontend | Flowanalyst and tooling | format/version, source path, AST pools, symbols, scopes |
| `flowanalyst.semantic_report` | 1 | Flowanalyst | Flowparallel, Flowoptimize, inspection tools | header, status, source, facts, regions, external operations, candidate/rejection evidence, diagnostics |
| `flowparallel.execution_plan` | 1 | Flowparallel | runtime/provider planners, inspection tools | header, status, source, candidates/rejections, dependencies, fallback and provenance |
| `flowoptimize.optimization_report` | 1 | Flowoptimize | review/tooling | header, source, lowering operations, transforms, projections, targets |
| `flowcore.lowering_plan` | 1/2 | lowering pipeline | backend/provider tooling | header, source, operations, functions |

The shared `flowcontracts` definitions and validators are the authority for
shape and version checks. `flowinspect` is a projection over those typed
values.

## Pressure entries

### FI-P001 — Existing contracts are the right JSON boundary

* **Category / severity:** JSON, TOOLING, P1.
* **Application requirement:** read and summarize real versioned artifacts
  without inventing semantic facts.
* **Attempt:** parse captured Flowanalyst and Flowparallel JSON, including
  malformed input and unsupported versions.
* **Observed behavior:** the repository already provides a strict JSON parser,
  typed artifact validators, duplicate-key refusal, and structured error paths
  in `flowcontracts`.
* **Workaround and cost:** writing a second parser or a Flowmini JSON runtime
  would duplicate authority and add substantial maintenance cost.
* **Options:** duplicate parsing; add a new language facility; consume the
  existing contract/provider.
* **Decision:** consume `flowcontracts` directly. No language change was
  justified. This is a solved boundary, not a Flowmini grievance.
* **Gate:** `flowinspect_cli`, malformed-input tests, and real artifact
  inspection.

### FI-P002 — Version and malformed-artifact handling must be explicit

* **Category / severity:** DIAGNOSTICS, TOOLING, P1.
* **Application requirement:** never guess when an artifact contract is absent,
  malformed, or newer than the inspector understands.
* **Observed behavior:** shared validators reject malformed structure, while a
  consumer still needs a clear format/version dispatch and stable exit status.
* **Workaround and cost:** accepting similar field names would manufacture
  authority and make failures difficult to automate.
* **Decision:** implement explicit format/version dispatch, structured parser
  errors, and exit codes 2 (malformed) and 3 (unsupported).
* **Gate:** unknown format, version 7, malformed JSON, missing fields, wrong
  field types, and deterministic output tests.

### FI-P003 — A concise projection is useful before universal schema coverage

* **Category / severity:** TOOLING, P2.
* **Application requirement:** quickly answer “what did analysis prove?” while
  debugging current compiler pipelines.
* **Observed behavior:** the first real semantic report exposes useful counts
  (symbols, regions, external operations, candidates, rejections) and reason
  groups; full schema rendering is much larger than the inspection task.
* **Workaround and cost:** dumping raw JSON is authoritative but slow to read;
  rendering every field would create a brittle second UI schema.
* **Decision:** keep a small stable summary and add fields only when a real
  inspection request needs them. Raw artifacts remain available for detail.
* **Gate:** real semantic-report and execution-plan captures, byte-identical
  repeat output.

### FI-P004 — Flowmini source was not the proportional implementation host

* **Category / severity:** LANGUAGE, STDLIB, P2 (pressure observed, no blocker).
* **Application requirement:** consume files, parse JSON, model typed outcomes,
  and render diagnostics.
* **Observed behavior:** the existing C++ `flowcontracts` provider already
  performs strict parsing and artifact validation. Building the first durable
  inspector in Flowmini would have required a new JSON surface, broader text
  and collection carriers, and more module plumbing before the tool could be
  useful.
* **Workaround and cost:** the tool is implemented in C++ and linked to the
  existing contract library; this keeps semantics in the established provider
  boundary and avoids a speculative language expansion.
* **Decision:** retain the C++ consumer for v0. Record Flowmini file/text/JSON
  and cross-function Result use as future application pressure only; no remedy
  was silently laundered into the language.
* **Gate:** revisit only when a real Flowmini application requires these
  operations and the provider boundary is insufficient.

### FI-P005 — Directory aggregation is a deferred request, not demonstrated pain

* **Category / severity:** COLLECTIONS, FLOWPARALLEL, P3.
* **Application requirement:** inspect a directory and aggregate many artifacts.
* **Observed behavior:** v0 intentionally accepts one file or stdin. No current
  acceptance program required directory enumeration.
* **Decision:** defer directory mode. If requested, first measure the need for
  enumeration, deterministic ordering, aggregate errors, and optional safe
  parallel inspection before adding APIs.

### FI-P006 — Broader artifact families need demand-led projections

* **Category / severity:** TOOLING, P3.
* **Application requirement:** inspect optimization, lowering, and frontend
  artifacts as naturally as semantic reports.
* **Observed behavior:** v0 validates and summarizes all five current families,
  but detailed field views are intentionally shallow for optimization/lowering.
* **Decision:** keep support additive and contract-driven. Expand a projection
  only when compiler work produces a concrete debugging question.

## Current pressure summary

* **Solved by existing architecture:** strict JSON, versioned dispatch,
  provenance-preserving validation, and provider-side file input.
* **Implemented in this checkpoint:** one-file deterministic inspection for
  five artifact families, explicit malformed/unsupported handling, and real
  Flowanalyst/Flowparallel dogfood.
* **Deferred:** a Flowmini-native inspector, directory mode, broad schema UI,
  generic collection aggregation, and parallel multi-file inspection.
* **Top evidence-led next pressures:**
  1. improve module/package and file/text surfaces only if a Flowmini-native
     tool is accepted as the next application;
  2. add demand-driven artifact projections for lowering/optimization review;
  3. collect a real directory workload before expanding collections or
     Flowparallel scheduling.
