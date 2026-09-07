# Flowmini versus JavaScript and TypeScript

| Area | Flowmini | JavaScript / TypeScript |
|---|---|---|
| Domain | governed provider/data-flow experiments; **EXPERIMENTAL** | web, servers, tooling, and event-driven applications |
| Types/runtime | small explicit declarations, native/provider runtime | JavaScript dynamic runtime; TypeScript erased static analysis over it |
| State/concurrency | placement and effects evolving; concurrency **NOT SUPPORTED** | mutable objects, event loop, promises/workers and platform APIs |
| Compiler/IR | AST/facts/Graph/lowering artifacts; parity gaps | mature parsers, JITs, bundlers, source maps, browser/Node runtimes |
| Interop/deployment/ecosystem | C ABI and small provider set | unmatched web APIs, package ecosystem, browsers and server deployment |

Flowmini gives stronger explicit provenance, provider identity, policy, fact
projection, state lineage, and evidence artifacts. JavaScript/TypeScript wins
for web UI, full-stack iteration, npm ecosystems, and event-driven integration.
Flowmini can be useful behind a provider where a small deterministic decision
or transformation must be audited. Calling a JS/TS implementation is often
cheaper than reproducing its platform bindings.

The weakness exposed is tooling and product reach: Flowmini has no browser
runtime, package ecosystem, source-map debugger, async model, or mature
standard library. TypeScript’s static checks also do not become runtime safety;
that is a useful reminder to state what Flowmini artifacts actually prove.

Sources: [MDN JavaScript guide](https://developer.mozilla.org/en-US/docs/Web/JavaScript)
and [TypeScript handbook](https://www.typescriptlang.org/docs/handbook/intro.html).
Exact runtime performance is **UNKNOWN** for Flowmini.
