# Flowmini versus Erlang and Elixir

| Area | Flowmini | Erlang / Elixir |
|---|---|---|
| Domain | accountable computation/provider graphs; **EXPERIMENTAL** | fault-tolerant concurrent and distributed systems |
| Runtime/concurrency | no language concurrency or supervision contract | BEAM processes, message passing, supervision and distribution |
| Types/state/errors | declared subset, placement, provenance; many semantics incomplete | dynamic runtime (with optional specs/types), immutable data, pattern matching, tagged failures |
| Compiler/tooling | small split pipeline, incomplete backend parity | mature BEAM compiler, releases, tracing and operational tooling |
| Deployment/ecosystem | small and target work **PLANNED** | production distributed deployments and libraries |

Flowmini’s facts, capabilities, providers, policy, provenance, state lineage,
projections, and evidence can describe a governed boundary, but they do not
provide BEAM supervision, mailbox semantics, distribution, hot upgrade, or
failure isolation. Erlang/Elixir are the stronger choice for resilient
services. Flowmini could specify a narrow policy or transformation invoked by a
BEAM provider.

The comparison directly falsifies any near-term claim that Flowmini is a
concurrent/distributed systems language. Supervision and failure semantics are
substantial runtime architecture; “add actors” fails the cost test without a
measured Flowcore need.

Sources: [Erlang/OTP System Principles](https://www.erlang.org/doc/system_principles/users_guide.html)
and [Elixir introduction](https://hexdocs.pm/elixir/introduction.html). Exact
Flowmini scheduler and failure behavior are **NOT SUPPORTED/UNKNOWN**.
