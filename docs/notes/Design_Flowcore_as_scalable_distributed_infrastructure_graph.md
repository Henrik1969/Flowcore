Future design note: Flowcore as scalable distributed infrastructure graph

Flowcore's graph model has a larger implication: if the program is fundamentally a graph of nodes connected by contracted wires, then physical placement does not have to be part of the algorithmic meaning.

The core semantic form remains:

node -> contracted wire -> node

Whether both nodes run in the same process, on different cores, in different local processes, on different machines, across a department subnet, or through a WAN/SSH tunnel is not fundamentally a change to the graph. It is a deployment/runtime decision, provided the wire contract can still be honored.

This suggests a long-term path where Flowcore can scale from local computation into enterprise-level distributed infrastructure description.

Scale ladder

Possible execution layers:

1. In-process execution
   - direct calls
   - in-memory queues
   - move/reference transfer
   - cheap and mostly trusted

2. Same process, multiple cores
   - scheduler-managed parallelism
   - worker pools
   - shared runtime memory
   - effect/resource conflict analysis

3. Same machine, multiple processes
   - pipes
   - Unix sockets
   - shared memory
   - coproc-like worker processes
   - lifecycle and framing become explicit

4. Local server / department LAN
   - TCP/RPC/message bus
   - service discovery
   - authentication
   - retries
   - latency and backpressure handling

5. WAN / SSH / remote secure transport
   - encryption
   - identity
   - trust model
   - partial failure
   - timeout and reconnect semantics

6. Highly remote or low-trust networks
   - signed payloads
   - strict capabilities
   - certificate/trust network
   - sandboxing
   - replay protection
   - audit trails
   - degraded operation models

The graph idea can remain stable across all of these, but the wire contracts must become stronger as the distance and trust boundary increases.

Important principle
Placement is policy.
Connection is semantics.
The contract decides whether a transport is legal.

The algorithm should be able to say:

Parse.out => Analyze.in
Analyze.out => Render.in

without caring whether Analyze runs locally, in another process, on a server in the department, or on a remote secured runtime.

But the runtime and deployment policy must care deeply.

A deployment policy may say:

Parse    -> local process
Analyze  -> gpu-server-03 over LAN
Render   -> local UI sink

or:

Parse    -> worker pool, local cores
Analyze  -> secured remote Flowcore runtime
Render   -> audit/log/archive sink

The semantic graph remains the same, but the wire lowering changes.

Wires are abstract, not magical

A Flowcore wire may lower to many different transport mechanisms:

direct function call
in-memory queue
pipe
Unix socket
shared memory
TCP socket
RPC
message bus
SSH tunnel
file stream
device channel
remote Flowcore runtime protocol

But it may only do so if the transport satisfies the declared contract.

A local in-process wire may only need:

Envelope<T> passed by move/ref/value

A remote wire may require:

Envelope<T>
  -> serialize / marshal
  -> frame
  -> sign / authenticate
  -> encrypt
  -> transmit
  -> verify
  -> deserialize
  -> deliver to port

This means distributed Flowcore requires explicit syntax and semantics for things that local Flowcore can treat cheaply.

Required future contract dimensions

For distributed or enterprise-scale Flowcore, a wire/port contract may need to specify:

payload type
payload encoding
serialization/marshalling format
ABI/protocol version
framing
ordering guarantees
delivery guarantees
retry behavior
timeout behavior
idempotency requirements
replay protection
backpressure behavior
error lane behavior
diagnostics and observability
lifecycle ownership
capability/security boundary
authentication method
authorization rules
trust/certificate/key requirements
resource limits
sandboxing expectations
placement constraints
latency expectations
failure-domain behavior

Without these, a remote wire would be pretending to be a local function call, which is dangerous.

Securit implications

Once Flowcore crosses process, machine, or trust boundaries, security becomes part of the real semantics.

Questions the system must be able to answer:

Who is this node?
Who owns this runtime?
Is this node allowed to consume this payload?
Is this node allowed to call this port?
Is this node allowed to produce effects?
Can this node write to disk/network/UI/database?
What contract version is being spoken?
Is the payload authenticated?
Is the transport encrypted?
Can this message be replayed?
Can this node lie, stall, fork, or disappear?
What happens if it does?
Who audits the result?

This implies a future security layer involving:

node identity
runtime identity
keys
certificates or trust networks
capability tokens
authorization policy
encrypted transport
key rotation
sandboxing
resource quotas
audit logging
diagnostic trails

This is not v1 complexity, but it should be acknowledged as the eventual cost of making Flowcore global/enterprise-capable.

Infrastructure inside one program

At the advanced end, Flowcore could describe an entire distributed infrastructure inside one program:

subgraph ImagePipeline policy {
    placement: department.gpu_pool
    transport: encrypted
    trust: signed_nodes_only
    max_workers: 12
    timeout: 30s
    output_order: stable
} {
    ingest   => decode
    decode   => classify
    classify => archive
    classify => alert_if_match
}

This would mean the Flowcore source is no longer only algorithmic code. It becomes:

program
+ deployment graph
+ security policy
+ transport policy
+ lifecycle policy
+ observability model
+ failure semantics

That is powerful, but it has a cost.

Verbosity is the price of truth

Distributed systems are not simple. Either the language makes the semantics explicit, or the system hides the mess until runtime failure.

Flowcore should preserve the rule:

Simple local programs should stay simple.
Enterprise/global graphs must be explicit.

A small local parser should not need NASA-moon-subnet syntax.

But if someone wants to describe a secure multi-server infrastructure, then Flowcore should eventually have the vocabulary to express exactly what is happening.

Relationship to earlier principles

This fits the existing Flowcore direction:

nodes are active work-performing components
wires are contracted communication media
ports are explicit endpoints
effects are declared contact with external reality
scheduler owns execution
placement is runtime/deployment policy
contracts decide compatibility
failure is flow
diagnostics travel with the envelope

Distributed Flowcore is therefore not a different idea. It is the same graph model extended upward through more demanding transport and trust layers.

Tentative governing principles
1. The graph defines semantic relationship.
2. The scheduler/runtime owns execution.
3. Placement is policy, not algorithmic meaning.
4. A wire is abstract, but not magical.
5. A transport must satisfy the wire contract.
6. Security boundaries must be explicit.
7. Failure, timeout, retry, and backpressure are part of the contract.
8. Local simplicity must not be sacrificed for future enterprise power.
9. Enterprise/global graphs must pay the cost of explicitness.
10. Distribution must be explicit in contracts, but not necessarily explicit in the algorithm.
Long-term conclusion

Flowcore could eventually become capable of describing a whole infrastructure as one inspectable graph: local nodes, process nodes, server nodes, subnet nodes, WAN nodes, secured remote runtimes, policy-governed placement, and contracted communication between them.

This is a powerful advanced direction, not v1 core complexity.

The short version:

The same graph can scale from one process to global infrastructure,
but every wider boundary demands stronger contracts.y
