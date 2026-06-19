ChatGPT

Ok I am an language nerd and I just found some languages I absolutely know NOTHING .. nil, nada, zero ..about 

Wait what ? 

ok that presentation on flix was interseting .. but Unison ???


Ok Verse .. what have you been drinking ?? hahah ok there are completly midblowing ideas here 

Yes. Let us turn the pile into a formal working draft.

This is not final syntax yet. This is the tapestry layer: concepts, grammar shape, canonical lowering, and semantic laws.
Flowcore Language Draft v0.1 — Formal Tapestry
0. Prime Directive

Flowcore is a language for describing typed signal flow through explicit semantic machinery.

Wires are medium.
Signals are content.
Nodes act through ports.
Contracts define compatibility.
Effects mark contact with reality.
The graph is truth.
Syntax is sugar over the graph.

The language must always distinguish:

wire      ≠ signal
node      ≠ wire
port      ≠ signal
name      ≠ identity
effect    ≠ failure
failure   ≠ exception
schedule  ≠ handwritten thread choreography

1. Core ontology
1.1 Signal

A signal is one transported item.

A signal may be:

value
event
message
error
diagnostic
control token
stream element

A signal has a payload type and travels through a wire according to that wire’s protocol.

Example:

TemperatureSample(21.4, timestamp)
ParseError(line=12, column=7)
Bytes(...)

A signal is not the wire.
1.2 Wire

A wire is invariant communication plumbing.

Formal shape:

wire {
    identity
    contract
    payload type
    ABI
    protocol
    lane
    scope
}

Example:

wire bytes : BytesWire

This means:

There exists a wire named bytes in this scope.
It carries signals compatible with BytesWire.

It does not mean:

bytes is a mutable variable.

For a single protocol, the wire may carry one signal.

For a stream protocol, the wire may carry many signals.

The wire remains invariant. The signals may vary.
1.3 Port

A port is a typed connector on a node.

Ports are directional:

in
out
tap
control

A port declares what kind of wire it can connect to.

Example:

in.data bytes : BytesWire
out.data ast  : AstWire
out.error err : ParseErrorWire

The lane is part of the port’s semantic position.
1.4 Node

A node is an entity that acts on signals through ports.

Nodes do not produce wires.

Nodes may:

provide / produce signals
consume signals
transmute signals
monitor signals
delegate / route signals
collate / join signals
emit / sink signals

Canonical form:

input wires -> input ports -> node -> output ports -> output wires

Example:

node ParseConfig : transmuter {
    in.data    bytes : BytesWire
    out.data   ast   : AstWire
    out.error  err   : ParseErrorWire

    effects {}
}

1.5 Contract

A contract defines compatibility.

For wires and ports, the contract answers:

Can this plug fit that socket?

A wire contract may define:

payload type
ABI
protocol
lane
authority requirements
ordering rules
multiplicity

Example:

contract BytesWire : wire {
    payload  Bytes
    abi      bytes.v1
    protocol single
    lane     data
}

1.6 ABI

An ABI defines representation and mechanical compatibility.

Initial draft:

abi bytes.v1 {
    representation contiguous_u8
    ownership      borrowed | owned
    alignment      1
}

Later this can become richer.

For now, ABI exists so Flowcore can say:

These two ports speak the same mechanical language.

1.7 Lane

A lane is a category of flow.

Core lanes:

lane data
lane error
lane event
lane log
lane diagnostic
lane control
lane internal

Lanes avoid collapsing everything into one return channel.

A parser may emit:

data lane       -> Ast
error lane      -> ParseError
diagnostic lane -> Diagnostic
event lane      -> ParseCompleted

1.8 Effect

An effect is declared contact with external reality or controlled mutable state.

Initial effect set:

effect ReadFS
effect WriteFS
effect Network
effect Log
effect CurrentTime
effect Random
effect NoRollback
effect Unsafe

A pure node has:

effects {}

A filesystem reader:

effects { ReadFS }

An email sender:

effects { Network, NoRollback }

Effects belong o nodes or operations, not wires.
1.9 Failure

Failure is a flow outcome.

It is not automatically:

an exception
a Boolean false
an effect
a crash

Failure may be represented as:

error lane
Result<T, E>
failable expression
transaction abort
no signal

For prototype one, the safest representation is probably explicit error lanes or Result.

Verse-style failable expressions and rollback can come later.
1.10 Border

A border is a semantic region boundary.

Examples:

pure border
effect handler border
transaction border
commit border
unsafe border
authority border
scheduling policy border

Borders restrict what is allowed inside them.

Example rule:

NoRollback may not appear inside rollback-capable transaction regions.

1.11 Identity and moniker

Formal identity is not the same as human name.

A thing may have:

compiler identity
formal contextual name
local moniker

Example:

identity:
    #wire:ab91...

formal name:
    config.load.parser.out.ast

local moniker:
    ast

Rule:

Names may be shortened locally,
but public names must be canonical inside a semantic frame.

2. Program structure

A Flowcore program consists of declarations.

Top-level declarations:

package
module
import

type
abi
protocol
lane
effect

contract
node
flow
policy

Possible file separation later:

.fci  interface / contracts / ABI / public declarations
.fc   implementation / flows / node bodies

Not mandatory yet, but conceptually clean.
3. Lexical skeleton
3.1 Comments

Suggested:

// line comment

/*
   block comment
*/

Possibly documentation comments:

/// doc comment for next item

3.2 Identifiers

Local identifiers:

[a-zA-Z_][a-zA-Z0-9_]*

Formal paths:

identifier(.identifier)*

Example:

config.load.parser.out.ast

3.3 Blocks

Use braces for structural declarations:

node Parser {
    ...
}

Newlines separate statements.

Semicolons optional or only required for multiple statements on one line.
4. Declaration grammar, rough EBNF

This is not parser-final, but gives us structure.

program         := declaration*

declaration     := package_decl
                 | module_decl
                 | import_decl
                 | type_decl
                 | abi_decl
                 | protocol_decl
                 | lane_decl
                 | effect_decl
                 | contract_decl
                 | node_decl
                 | flow_decl
                 | policy_decl

package_decl    := "package" qualified_name
module_decl     := "module" qualified_name
import_decl     := "import" qualified_name

qualified_name  := identifier ("." identifier)*

block           := "{" declaration_or_stmt* "}"

5. Contracts
5.1 Wire contract

contract BytesWire : wire {
    payload  Bytes
    abi      bytes.v1
    protocol single
    lane     data
}

Formal grammar:

contract_decl :=
    "contract" identifier ":" contract_kind block

contract_kind :=
    "wire" | "node" | "port" | "protocol" | "abi"

A wire contract requires at minimum:

payload
abi
protocol
lane

5.2 Stream wire contract

contract TemperatureFeed : wire {
    payload  TemperatureSample
    abi      temperature.sample.v1
    protocol stream
    lane     data
}

This means:

The wire is stable.
Many TemperatureSample signals may pass through it.

6. Nodes
6.1 Node declaration

node ParseConfig : transmuter {
    in.data     bytes : BytesWire
    out.data    ast   : AstWire
    out.error   err   : ParseErrorWire
    out.log     diag  : DiagnosticWire

    effects {}
}

Node category examples:

provider
consumer
transmuter
monitor
delegate
collator
emitter
sink

Grammar:

node_decl :=
    "node" identifier (":" node_kind)? "{"
        port_decl*
        effect_block?
        node_body?
    "}"

node_kind :=
    "provider"
  | "consumer"
  | "transmuter"
  | "monitor"
  | "delegate"
  | "collator"
  | "emitter"
  | "sink"

port_decl :=
    port_direction "." lane_name identifier ":" contract_name

port_direction :=
    "in" | "out" | "tap" | "control"

6.2 Effects block

effects {
    ReadFS
    Log
}

Pure:

effects {}

Grammar:

effect_block :=
    "effects" "{" effect_name* "}"

7. Flows

A flow instantiates wires, nodes, and connections.

Canonical form is explicit wiring.
7.1 Explicit canonical wiring

flow ConfigPipeline {
    wire path  : PathWire
    wire bytes : BytesWire
    wire ast   : AstWire
    wire err   : ParseErrorWire

    node reader : ReadFile
    node parser : ParseConfig

    connect path                  -> reader.in.data.path
    connect reader.out.data.bytes -> bytes

    connect bytes                 -> parser.in.data.bytes
    connect parser.out.data.ast   -> ast
    connect parser.out.error.err  -> err
}

This is the core form.

Everything else lowers to this.
7.2 Grammar

flow_decl :=
    "flow" identifier flow_modifiers? "{"
        flow_item*
    "}"

flow_item :=
    wire_decl
  | node_instance_decl
  | connect_stmt
  | sugar_stmt
  | policy_stmt

wire_decl :=
    "wire" identifier ":" contract_name

node_instance_decl :=
    "node" identifier ":" node_contract_name

connect_stmt :=
    "connect" endpoint "->" endpoint

endpoint :=
    identifier
  | qualified_port_path

qualified_port_path :=
    identifier "." port_direction "." lane_name "." identifier

8. Surface sugar
8.1 Wire declarations may omit wire inside flow

Canonical:

wire bytes : BytesWire

Sugar:

bytes : BytesWire

Allowed only inside flow blocks.
8.2 Node-call sugar

Surface:

bytes <- ReadFile(path)
ast, err <- ParseConfig(bytes)

Meaning:

Instantiate node if needed.
Connect input wires to node input ports.
Connect node output ports to output wires.

Lowering:

node _n1 : ReadFile
connect path                 -> _n1.in.data.path
connect _n1.out.data.bytes   -> bytes

node _n2 : ParseConfig
connect bytes                -> _n2.in.data.bytes
connect _n2.out.data.ast     -> ast
connect _n2.out.error.err    -> err

Important:

<- is not assignment.
<- is wire-driving / port-connection sugar.

8.3 Pipeline sugar

Surface:

report <- path
    => ReadFile
    => ParseConfig
    => ValidateConfig

Lowers first to node-call sugar:

bytes  <- ReadFile(path)
ast    <- ParseConfig(bytes)
report <- ValidateConfig(ast)

Then to explicit wiring.

Pipeline sugar is allowed only when each stage has unambiguous default data ports.

Ambiguity is illegal.
9. Connection rules
9.1 Wire production rule

A wire may have exactly one producer connection in a single-value protocol.

Valid:

connect reader.out.data.bytes -> bytes

Invalid:

connect reader1.out.data.bytes -> bytes
connect reader2.out.data.bytes -> bytes

Unless the wire protocol explicitly allows multiple producers.
9.2 Many consumers allowed

Valid:

connect bytes -> parser.in.data.bytes
connect bytes -> checksum.in.data.bytes
connect bytes -> scanner.in.data.bytes

9.3 Contract compatibility

A connection is valid only if:

source payload fits target payload
source ABI fits target ABI
source protocol fits target protocol
source lane fits target lane
authority requirements are satisfied

If not:

compiler error

9.4 Adapter nodes

Bad direct connection:

connect jsonAst -> binaryEncoder.in.data.bytes

Correct:

bytes <- EncodeAst(jsonAst)
connect bytes -> binaryEncoder.in.data.bytes

10. Effects semantics
10.1 Effect inference

A flow’s effects are the union of its nodes’ effects.

Example:

node ReadFile {
    effects { ReadFS }
}

node ParseConfig {
    effects {}
}

flow LoadConfig {
    bytes <- ReadFile(path)
    ast   <- ParseConfig(bytes)
}

Flow inferred effects:

{ ReadFS }

10.2 Declared effect bounds

A flow may declare allowed effects:

flow LoadConfig effects { ReadFS } {
    bytes <- ReadFile(path)
    ast   <- ParseConfig(bytes)
}

Compiler rule:

inferred effects ⊆ declared effects

Invalid:

flow ValidateOnly effects {} {
    send <- SendEmail(message)
}

Because SendEmail has:

{ Network, NoRollback }

11. Purity

A pure node:

effects {}

A pure flow:

flow ValidateConfig effects {} {
    report <- Validate(ast)
}

Pure regions forbid effectful nodes.

Pure flows may be candidates for:

caching
parallelism
deterministic testing
compile-time evaluation
safe retry

Purity is semantic, not merely syntactic.
12. Failure model, first draft

Prototype one should use explicit error lanes.

Example:

node ParseConfig : transmuter {
    in.data    bytes : BytesWire
    out.data   ast   : AstWire
    out.error  err   : ParseErrorWire

    effects {}
}

Flow:

ast, parseErr <- ParseConfig(bytes)

Lowering maps outputs by lane.

Later syntax may support failable expressions:

attempt {
    ast ?<- ParseConfig(bytes)
    report <- Validate(ast)
}

But this belongs to a later phase.

First law:

Failure is expressed in the graph.

13. Transactions, future draft

Not prototype one, but reserve syntax space.

Possible:

transaction {
    receipt <- Debit(account, amount)
    ok      <- RequirePositive(account.balance)
    credit  <- Credit(target, amount)
}
commit {
    SendEmail(message)
}

Semantic law:

NoRollback effects forbidden before commit inside transaction.

Compiler check:

transaction body effects must exclude NoRollback

14. Scheduling policy

Default scheduling is derived from graph topology.

Example:

flow ScanFile {
    bytes <- ReadFile(path)

    ast     <- Parse(bytes)
    hash    <- Checksum(bytes)
    verdict <- MalwareScan(bytes)

    report <- Collate(ast, hash, verdict)
}

Graph says:

Parse, Checksum, and MalwareScan may run after bytes exists.
They do not depend on each other.
Collate waits for all three.

Advanced policy syntax:

flow ScanFile policy {
    MaxConcurrency(8)
    PreserveOrder(data)
    ResourceExclusive(FileSystem)
} {
    ...
}

Scheduling policies guide runtime decisions.

They should not expose raw threads/mutexes as ordinary source constructs.
15. Names, scopes, and monikers
15.1 Semantic frames

A semantic frame may be:

package
module
flow
node
contract
region

Inside a frame, names must be consistent.
15.2 Canonical name

Public declarations require one canonical exported name.

export node config.ParseConfig

15.3 Local moniker

Inside a flow:

use config.ParseConfig as Parse

Then:

ast <- Parse(bytes)

The moniker is local.

It does not rename the canonical node globally.
15.4 Alias discipline

Aliases are explicit:

alias config.ParseConfig as config.Parse

Aliases may be marked:

deprecated alias legacy.ParseOld as config.ParseConfig

No silent 200-names-for-tea.
16. Lowering pipeline

The compiler should process Flowcore in layers.

source text
    ↓
lexer
    ↓
parser
    ↓
surface AST
    ↓
name resolution
    ↓
sugar lowering
    ↓
canonical graph IR
    ↓
type / ABI / contract check
    ↓
effect / failure / border analysis
    ↓
scheduling analysis
    ↓
backend / interpreter / emitter

The key internal representation is the canonical graph.
16.1 Canonical graph IR

A flow lowers to something like:

Flow {
    Wires:
        path  : PathWire
        bytes : BytesWire
        ast   : AstWire

    Nodes:
        n1 : ReadFile
        n2 : ParseConfig

    Connections:
        path              -> n1.in.data.path
        n1.out.data.bytes -> bytes
        bytes             -> n2.in.data.bytes
        n2.out.data.ast   -> ast
}

Everything pretty becomes this.

The graph is truth.
17. Semantic laws

These are compiler laws.
Law 1 — Wire/signal separation

A wire is a medium.
A signal is transported content.
Nodes operate on signals through ports.

Law 2 — No implicit mutation

A wire declaration is not mutable variable declaration.

Law 3 — Producer rule

A single-value wire has at most one producer.

Law 4 — Consumer rule

A wire may have many consumers unless its protocol forbids fan-out.

Law 5 — Contract fit

Ports may connect only through compatible contracts.

Law 6 — Effects are explicit

Nodes that touch reality must declare effects.

Law 7 — Purity is enforceable

A pure node or flow may not contain effectful operations.

Law 8 — Failure is flow

Failure must be represented in the graph or in an explicit failure context.

Law 9 — NoRollback border

NoRollback effects may not occur inside rollback-capable speculative regions.

Law 10 — Names are scoped monikers

Local names do not define formal identity.
Public names require canonical discipline.

Law 11 — Scheduling follows topology

Execution order is derived from graph dependencies, effects, resources, and policies.

18. Minimal viable Flowcore syntax

This is probably the first useful shape:

package example.config

lane data
lane error

effect ReadFS

contract PathWire : wire {
    payload  Path
    abi      path.v1
    protocol single
    lane     data
}

contract BytesWire : wire {
    payload  Bytes
    abi      bytes.v1
    protocol single
    lane     data
}

contract AstWire : wire {
    payload  Ast
    abi      ast.v1
    protocol single
    lane     data
}

contract ParseErrorWire : wire {
    payload  ParseError
    abi      parse_error.v1
    protocol single
    lane     error
}

node ReadFile : provider {
    in.data    path  : PathWire
    out.data   bytes : BytesWire

    effects { ReadFS }
}

node ParseConfig : transmuter {
    in.data    bytes : BytesWire
    out.data   ast   : AstWire
    out.error  err   : ParseErrorWire

    effects {}
}

flow LoadConfig effects { ReadFS } {
    path  : PathWire
    bytes : BytesWire
    ast   : AstWire
    err   : ParseErrorWire

    bytes    <- ReadFile(path)
    ast, err <- ParseConfig(bytes)
}

Canonical lowering:

flow LoadConfig effects { ReadFS } {
    wire path  : PathWire
    wire bytes : BytesWire
    wire ast   : AstWire
    wire err   : ParseErrorWire

    node _read  : ReadFile
    node _parse : ParseConfig

    connect path                  -> _read.in.data.path
    connect _read.out.data.bytes  -> bytes

    connect bytes                 -> _parse.in.data.bytes
    connect _parse.out.data.ast   -> ast
    connect _parse.out.error.err  -> err
}

That is our first real tapestry.

It has:

contracts
wires
nodes
ports
lanes
effects
explicit lowering
graph truth

And most importantly, it obeys the constitution.

Exactly .. now we can go further down into the inner workings because the scaffolding is in place. We got the overall structural rules in place. Now we can start talking the actual computing and how we semantically express intent and functionalities in a concise way 

Yes. Now we move one layer inward:

outer layer:
    wires, ports, nodes, contracts, flows

inner layer:
    what a node actually computes
    how intent is expressed
    how payload values are transformed
    how signals are emitted

The key is to not confuse internal computation with graph plumbing.
New distinction

Inside a flow:

ast <- ParseConfig(bytes)

means:

connect node ports to wires

Inside a node:

let ast = parseConfig(bytes)

means:

compute a local immutable payload value

So we need two levels:

wire graph level:
    plumbing between nodes

node body level:
    local computation over signal payloads

Proposed core rule

Wires carry signals.
Signals contain payload values.
Node bodies compute payload values.
Ports emit or receive signals.

That gives us the clean split.
1. Local values inside nodes

Inside a node, let binds local immutable values.

let text = Utf8.decode(bytes)
let ast  = Config.parse(text)

This is not wire binding.
This is local computation.

Rule:

let creates a local fact.
It is not mutable.
It is not a wire.
It exists only inside the node activation.

So:

let x = 10
let y = x + 1

is fine.

But:

let x = 10
let x = 20

should probably be illegal inside the same scope.

If we want a changed value, name the next fact:

let rawPath      = input.path
let normalized   = Path.normalize(rawPath)
let canonical    = Path.canonicalize(normalized)

That keeps the single-assignment spirit without making ordinary calculation painful.
2. Port receive and emit must be explicit

A node receives signal payloads from input ports.

For a simple single-shot node, input names may be automatically available:

node ParseConfig : transmuter {
    in.data    bytes : BytesWire
    out.data   ast   : AstWire
    out.error  err   : ParseErrorWire

    effects {}

    impl {
        let text = Utf8.decode(bytes)
        let tree = Toml.parse(text)
        let cfg  = ConfigAst.fromToml(tree)

        emit ast <- cfg
    }
}

Here:

bytes

inside the body means:

payload received from in.data.bytes

And:

emit ast <- cfg

means:

emit signal with payload cfg through out.data.ast

That is good because emit makes the signal boundary visible.
3. Functions vs nodes

We probably want both.
Function

A function is local computation over payload values.

fn normalizePath(path : Path) -> Path {
    ...
}

Pure by default:

fn checksum(bytes : Bytes) -> Hash {
    ...
}

A function may possibly declare effects later, but the default should be pure.
Node

A node is graph-facing.

It has ports, lanes, wire contracts, effects, and an implementation.

node ReadFile : provider {
    in.data  path  : PathWire
    out.data bytes : BytesWire
    out.error err  : IOErrorWire

    effects { ReadFS }

    impl {
        let result = FS.read(path)

        match result {
            Ok(bytes)  -> emit bytes <- bytes
            Err(error) -> emit err   <- error
        }
    }
}

So:

fn   = payload transformer
node = plugboard component

This is a good separation.
4. Intent expression should be verb-oriented

Flowcore should not merely say:

x = f(y)

It should let us express domain intent cleanly.

But we do not want English soup either.

A reasonable style:

let normalized = Path.normalize(path)
let bytes      = File.read(normalized)
let ast        = Config.parse(bytes)
let report     = Config.validate(ast)

This keeps normal programmer readability while making semantic namespaces strong:

Path.normalize
File.read
Config.parse
Config.validate

The intent is in the verb-object naming.

A more Flowcore-ish spelling might become:

let normalized = path => Path.normalize
let bytes      = normalized => File.read
let ast        = bytes => Config.parse
let report     = ast => Config.validate

Inside node bodies, => can mean payload pipeline.

Inside flow blocks, => means graph pipeline.

Same visual idea, but different lowering context:

inside node:
    value pipeline

inside flow:
    wire/node pipeline

That is acceptable if the compiler grammar keeps the contexts separate.
5. Failure in node bodies

Prototype one can use Result and error lanes.

Example:

node ParseConfig : transmuter {
    in.data    bytes : BytesWire
    out.data   ast   : AstWire
    out.error  err   : ParseErrorWire

    effects {}

    impl {
        let parsed = Config.parse(bytes)

        match parsed {
            Ok(cfg)  -> emit ast <- cfg
            Err(e)   -> emit err <- e
        }
    }
}

Later we can add failable sugar:

impl {
    let text ?= Utf8.decode(bytes)
    let tree ?= Toml.parse(text)
    let cfg  ?= ConfigAst.fromToml(tree)

    emit ast <- cfg
}
fail e {
    emit err <- e
}

But that is phase two.

For now, explicit match is honest and clear.
6. Effects inside node implementations

If a node declares:

effects { ReadFS }

then its body may call operations requiring ReadFS.

impl {
    let bytes = FS.read(path)
    emit bytes <- bytes
}

If a node declares:

effects {}

then this is illegal:

impl {
    let bytes = FS.read(path)
    emit bytes <- bytes
}

Compiler:

ParseConfig declared effects {}
but body requires { ReadFS }

That gives us the Flix-style security benefit without needing full algebraic effects yet.
7. Node activation semantics

We need to define when a node runs.

For protocol single:

A node activation occurs when all required input ports receive their signal.

Example:

node Validate {
    in.data ast    : AstWire
    in.data policy : PolicyWire
    out.data report : ReportWire
}

This node fires when both ast and policy signals are available.

ast ───────┐
           ├──► Validate ───► report
policy ────┘

For protocol stream, we need a later policy:

per signal
per batch
sliding window
join by key
join by timestamp

Do not solve all stream semantics now. Just reserve the protocol slot.
8. Minimal implementation syntax

A node body could have these primitives:

let name = expression
emit port <- expression
match expression { ... }
if condition { ... } else { ... }
return expression          // only for fn, not node

A function:

fn Config.validate(ast : Ast, policy : Policy) -> ValidationReport {
    let rules  = Policy.rules(policy)
    let result = Rules.apply(rules, ast)

    return result
}

A node:

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}

    impl {
        let report = Config.validate(ast, policy)
        emit report <- report
    }
}

That is concise and structurally clean.
9. Canonical body IR

Internally, node bodies should lower to something boring:

NodeImpl ValidateConfig {
    Inputs:
        ast    : Ast
        policy : Policy

    Locals:
        report = Call(Config.validate, ast, policy)

    Emits:
        out.data.report <- report

    Effects:
        {}
}

Again:

pretty syntax lowers to boring truth

For flow blocks, truth is graph IR.
For node bodies, truth is SSA-like local computation plus explicit emits.
10. First actual computing model

This gives us a working computational core:

1. A flow graph connects wires and nodes.

2. A node activation receives payloads from input port signals.

3. The node body computes local immutable values.

4. The body may call pure functions or declared-effect operations.

5. The body emits payloads as signals through output ports.

6. The emitted signals travel over connected wires.

7. Scheduling comes from the graph.

That is the first real machine.
Proposed next law

Flowcore has two binding forms:

let
    binds a local immutable payload value inside computation

<-
    connects or drives flow-level plumbing / signal emission,
    depending on context

emit
    explicitly crosses from local payload computation to signal output

So:

let ast = parse(bytes)

means compute.

ast <- ParseConfig(bytes)

inside a flow means wire the graph.

emit ast <- astValue

inside a node means emit a signal.

That is sharp enough to build on.

Quote :"A node activation occurs when all required input ports receive their signal.

Example:

node Validate {
    in.data ast    : AstWire
    in.data policy : PolicyWire
    out.data report : ReportWire
}

This node fires when both ast and policy signals are available.

ast ───────┐
           ├──► Validate ───► report
policy ────┘"

I just know what my Phd.Cand.Computer.Science Cousin will say to this ;) 
Where have you specified what actually gets done ? 

Your cousin would be completely right. :D

What I described there is only the activation rule:

When is the node eligible to run?

It does not specify:

What does the node actually do when it runs?

So we need to split node semantics into two separate layers.
1. Node interface

This says what the node connects to:

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}
}

This only says:

ValidateConfig has two input ports.
ValidateConfig has one output port.
ValidateConfig is pure.
ValidateConfig may activate when ast and policy are present.

But yes: nothing yet says how report is computed.

So we need the second part.
2. Node implementation

The implementation says what happens during activation.

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}

    impl {
        let rules  = Policy.rules(policy)
        let result = Rules.apply(rules, ast)

        emit report <- result
    }
}

Now the node has actual behaviour:

Receive ast payload.
Receive policy payload.
Compute rules.
Apply rules to ast.
Emit result through out.data.report.

So the fuller rule becomes:

A node activation occurs when required input ports have matching signals.
The node body then executes over the received payloads.
The node may emit zero or more output signals through its output ports.

Formal shape

A node is not just a box with ports.

It is more like:

Node =
    interface
    + activation rule
    + implementation semantics
    + effects contract

Or:

Node N:
    inputs     : ports
    outputs    : ports
    effects    : effect set
    activation : readiness condition
    impl       : transition / computation

For the ValidateConfig example:

activation:
    ast signal available
    policy signal available

implementation:
    report_payload = Validate(ast.payload, policy.payload)

emission:
    emit report_payload on out.data.report

The node body is where actual computing lives

Inside impl, we are no longer wiring the graph.

We are computing payload values.

impl {
    let rules  = Policy.rules(policy)
    let result = Rules.apply(rules, ast)

    emit report <- result
}

Here:

let result = ...

is local computation.

But:

emit report <- result

crosses from local computation back into signal flow.

So we get three distinct syntactic worlds:

flow level:
    report <- ValidateConfig(ast, policy)
    means graph wiring sugar

node impl level:
    let result = Validate(ast, policy)
    means local payload computation

emit level:
    emit report <- result
    means produce output signal

That is the missing precision.
Abstract nodes versus implemented nodes

We should probably allow both.
Abstract node contract

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}
}

This says:

Something with this shape exists.

But it is not executable by itself.

Useful for interfaces, libraries, planning, distributed systems, FFI, or documentation.
Implemented node

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}

    impl {
        let reportValue = Config.validate(ast, policy)
        emit report <- reportValue
    }
}

This is executable.

Compiler rule:

A concrete flow may instantiate only implemented nodes,
unless the node is externally provided, imported, native, or deliberately abstract.

Alternative implementation sources

Not every node body must be written in Flowcore.

A node may be implemented by:

Flowcore impl block
pure function binding
native builtin
foreign/FFI symbol
external service
subflow

Examples:

node Checksum : transmuter {
    in.data  bytes : BytesWire
    out.data hash  : HashWire

    effects {}

    impl use fn Crypto.sha256
}

Or:

node ReadFile : provider {
    in.data    path  : PathWire
    out.data   bytes : BytesWire
    out.error  err   : IOErrorWire

    effects { ReadFS }

    impl native system.fs.read
}

Or node-as-subflow:

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    in.data  policy : PolicyWire
    out.data report : ReportWire

    effects {}

    impl flow {
        rules  <- ExtractRules(policy)
        report <- ApplyRules(ast, rules)
    }
}

That last one is important: a node can be a wrapper around a smaller internal graph.
More formal operational rule

For a single-shot node:

Given node N with required inputs I1...In
and outputs O1...Om:

When each required input port has a signal:
    collect payloads p1...pn
    execute impl_N(p1...pn)
    produce emissions e1...ek
    route each emission to an output port

So:

N(ast, policy) ⇓ { report := Validate(ast, policy) }

Or with effects:

N(ast, policy) ⇓ effects E, emissions S

For pure node:

E = {}

For ReadFile:

ReadFile(path) ⇓ effects { ReadFS }, emissions { bytes } or { err }

Important compiler checks

Once impl exists, the compiler can check:

All input port names used in impl exist.

All emitted ports exist.

Emitted payload type fits output wire contract.

All required effects of called operations are covered by node.effects.

A node with effects {} calls only pure functions/nodes.

All local let bindings are single-assignment within scope.

Every mandatory output is emitted on all successful paths, unless declared optional.

Failure/error paths are represented explicitly.

Example illegal node:

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    out.data report : ReportWire

    effects {}

    impl {
        let text = FS.read("/tmp/policy.json")
        emit report <- Config.validate(ast, text)
    }
}

Compiler:

ValidateConfig declares effects {}
but implementation requires { ReadFS }.

Another illegal node:

node ValidateConfig : transmuter {
    in.data  ast    : AstWire
    out.data report : ReportWire

    effects {}

    impl {
        let x = Config.validate(ast)
        emit banana <- x
    }
}

Compiler:

No output port named banana exists.

Better wording for the earlier sentence

The earlier sentence should be amended from:

This node fires when both ast and policy signals are available.

To:

This node becomes eligible for activation when both ast and policy signals are available. On activation, its implementation body determines what computation occurs and which output signals are emitted.

That is the version your cousin would not immediately shoot full of holes.
The new law

Activation is not computation.

Activation says when a node may run.
Implementation says what the node does.
Emission says what signals it produces.

That gives us the missing mechanical layer.


t
