📘 FlowCore — Workpaper Specification (v0.1)
1. Overview

FlowCore is a contract-driven, dataflow-oriented programming language.

Programs are constructed as graphs of nodes, where:

nodes define behavior,
ports define interfaces,
connections define execution flow.

FlowCore enforces explicit semantics:

no hidden behavior,
no implicit control flow,
no ambiguous operator meaning.
2. Design Principles (Normative Intent)
2.1 Explicitness

All behavior MUST be visible as:

node definitions
port interactions
flow connections
2.2 Separation of Concerns

The language separates:

what exists (contracts)
what happens (implementation)
how it connects (composition)
2.3 No Hidden Semantics

There is no:

implicit execution order
hidden mutation
invisible side effects
3. Program Structure

A FlowCore program consists of three artifact types:

3.1 .fci — Interface (Contract Layer)

Defines:

types
node signatures
port definitions
effect declarations

Constraints:

MUST NOT contain execution logic
MUST NOT contain wiring
MUST NOT mutate state
3.2 .fc — Implementation (Behavior Layer)

Defines:

node behavior
transformation logic
internal wiring

Constraints:

MUST conform to .fci
MUST NOT violate declared effects
3.3 .fcp — Composition (Flow Layer)

Defines:

node instantiation
wiring (->)
system topology

Constraints:

MUST NOT define new behavior
MUST only connect existing nodes
4. Core Ontology
4.1 Nodes

A node is the fundamental computational unit.

A node:

has a type
has named ports
participates in flow

All values are nodes.

4.2 Ports

Ports define how nodes communicate.

Each port has:

direction: input | output
lane: data | event | ctrl | error
type: T

Example:

input data u32 value;
output data u32 result;
4.3 Connections

Connections define flow:

A.out -> B.in;

Properties:

directional
type-checked
lane-consistent
5. The Method Triad (Core Semantic Model)

All behavior is expressed using three method categories:

5.1 in — Ingress

Purpose: Admit or construct data.

Properties:
creates instances
validates input
normalizes external data
Restrictions:
MUST NOT mutate existing instances
MUST NOT emit external effects
Example:
in func parse(s:String) -> T
5.2 trans — Transformation

Purpose: Transform data deterministically.

Properties:
pure by default
deterministic
returns new values
Restrictions:
MUST NOT perform IO
MUST NOT emit events
MUST NOT mutate global state
Example:
trans func add(other:T) -> T
5.3 out — Egress

Purpose: Produce observable effects.

Properties:
emits to external systems
may produce events or side effects
Restrictions:
MUST be explicit
MUST declare effect capabilities
Example:
out func print() -> Void
6. Core Execution Model

All execution is reducible to:

input → in → trans → trans → ... → out

This applies universally:

Domain	Flow
CLI	parse → compute → print
GUI	event → update → render
IO	read → process → write
7. Assignment Semantics

Surface syntax:

a = expr;

Desugars to:

expr.out -> a.in;
Rule:
Assignment is syntactic sugar for flow
Flow is the only true execution model
8. Purity & Effects
8.1 Purity Rules
Method	Allowed Effects
in	construction only
trans	none (pure)
out	all declared effects
8.2 Compiler Enforcement

The compiler MUST:

reject side effects in trans
enforce declared capabilities
validate lane usage
9. Lanes (Communication Semantics)

Lanes represent categories of flow:

Lane	Meaning
data	value transfer
event	asynchronous trigger
ctrl	control flow
error	failure propagation
Rule:
Nodes MUST declare which lanes they use
Wiring MUST respect lane compatibility
10. Operators

Operators are layer-dependent semantics:

Layer	Meaning
.fci	type relations
.fc	value transformations
.fcp	flow connections
Rule:

An operator MUST NOT have ambiguous meaning across layers.

11. Metadata & Annotations

Annotations (@...) are:

declarative
non-behavioral by default

If an annotation affects behavior:

it MUST desugar into explicit nodes or wiring
12. External Modules

External components:

MUST provide .fci
MAY hide implementation
Rule:

Black bxes are allowed. Undefined contracts are not.

13. Error Handling

Errors propagate via the error lane.

Rules:

errors MUST be explicit
silent failure is illegal
unhandled errors MUST be surfaced
14. Determinism
trans MUST be deterministic
in MAY be non-deterministic (external input)
out MAY be non-deterministic (IO)
15. Design Constraints (Hard Rules)
Everything is a node
Everything flows through ports
No hidden behavior
No implicit control flow
No silent effects
Contracts are authoritative
16. Conceptual Summary

FlowCore models programs as explicit flows of data through well-defined transformations, constrained by contracts and enforced by a strict separation of construction, transformation, and effect.

17. Next Steps (Spec Evolution)

To move from workpaper → formal spec:

Formal grammar (BNF / PEG)
Type system definition
Lane typing rules (formal)
Execution model (VM or IR)
Module system details
Error type system
Standard library definition
Final distilled statement

If something happens, it must be a node.
If something moves, it must be a connection.
If something is promised, it must be declared.o
