---
title: Named Targets and Main Entrypoints
status: binding-direction-active-design
kind: language-design
---

# Named Targets and Main Entrypoints

Authority: the one-root-`main` rule inherited from v0.24 remains supported for
anonymous root programs. The neutral source universe and named-target
projection model are implemented through Flowanalyst. Artifact selection and
target-specific lowering remain staged work.

## Current v0.25 rule

Active Flowmini v0.25 supports the inherited root form and named target
declarations. A root program may contain one root `main`, or named targets with
target-local declarations and entrypoints.

Multiple anonymous root `main` blocks are currently an error.

```flow
program example

main {
    ...
}

main {
    ...
}
```

This remains invalid as two anonymous root entrypoints in v0.25.

Named targets are the explicit structural expansion of that rule.

## Target implementation status

Flowcore/Flowmini preserves multiple named targets from the same source base.
Flowanalyst enforces exactly one `main` procedure per named target and emits
target status in its semantic report. Flowlower does not yet select a target or
emit separate artifacts for the named-target case.

The planned model is:

```text
program
    shared semantic universe

target
    named buildable/runnable product

main
    entrypoint inside a target
```

A source universe owns shared declarations.

A target owns target-local declarations, target-local prerequisites, and at
most one main block until target completeness is checked. A selected runnable
target must have exactly one main block.

“Program” and “library” are artifact roles, not mutually exclusive source
universe kinds. The same source universe may produce a library target, an
executable target, a test target, or several of them.
## Candidate syntax

The following spelling is provisional:

```flow
program toolset

fn shared_log(message : string) -> void {
    ...
}

target cli {
    fn parse_args() -> Args {
        ...
    }

    main {
        args : Args(parse_args())
        shared_log("cli running")
    }
}

target daemon {
    prerequisite capability "network.server"

    fn setup_server() -> Server {
        ...
    }

    main {
        server : Server(setup_server())
        shared_log("daemon running")
    }
}

target test_runner {
    fn make_fake_job() -> Job {
        ...
    }

    main {
        job : Job(make_fake_job())
        shared_log("test runner running")
    }
}
```

## Semantic model

The intended scope structure is:

```text
program scope
    shared declarations
    shared types
    shared records
    shared functions
    shared prerequisites, if allowed by policy

target scope
    target-specific declarations
    target-specific prerequisites
    exactly one main block
```

The main block is not a free-floating global concept in the future target model.

Instead:

```text
main is an entrypoint inside a target.
target is a buildable or runnable product.
program is the shared declaration universe.
```

## Artifact and loading model

The target declaration is structural input to later artifact planning. It does
not encode CPU instructions, register choices, calling conventions, or memory
placement in the AST.

The planned artifact is layered:

```text
manifest
dispatcher
symbol/version lookup
API and ABI contracts
dependency metadata
lazy implementation sections
```

An artifact is materialized from the selected target’s dependency closure. It
may contain multiple compatible implementations of one capability, provided
their API, ABI, state, and dependency contracts are explicit. The dispatcher
selects an implementation by capability identity, version, ABI, and policy.
Unused implementation sections need not be loaded.

This is a post-AST artifact concern. The frontend records declarations,
targets, names, source locations, and written contracts; semantic analysis and
artifact planning establish compatibility and dependency closure later.

## Build and lowering model

A build or lowering tool may later select one or more targets:

```bash
flowmini build --target cli
flowmini build --target daemon
flowmini build --all-targets
```

Each selected target may lower to a separate library, executable, service, test
runner, tool, graph projection, language binding, or runtime surface. One
source universe may produce both a library and an executable without changing
the meaning of the shared declarations.
## Visual patchbay interpretation

Named targets fit the Flowcore visual patchbay model.

A program can be understood as a shared graph universe.

Each target is a named projection of that universe.

```text
program graph
    shared nodes and declarations

target cli
    CLI-specific graph projection

target daemon
    daemon-specific graph projection

target test_runner
    test-specific graph projection
```

Shared declarations may appear in multiple target projections.

Target-local declarations only appear inside their target projection.

This supports the long-term goal of representing a whole system from one governed source base rather than only representing one executable at a time.
## Design law

A Flowcore source universe is a shared semantic universe.

A target is a named projection of that universe into a buildable or runnable product.

A main block is the entrypoint of a target.

For active v0.25, root programs support one anonymous main block, while named
targets may each own one main block.

Future versions will add explicit target selection and require a selected
target when a source universe contains more than one runnable target.

Why not allow multiple anonymous main blocks?

Multiple anonymous root main blocks are ambiguous:

```flow
program bad_example

main {
    ...
}

main {
    ...
}
```

The compiler or build tool would have to guess which main represents the executable entrypoint.

Named targets avoid that ambiguity:

```flow
program good_example

target cli {
    main {
        ...
    }
}

target daemon {
    main {
        ...
    }
}
```

This keeps the language precise while allowing the source base to describe a larger system.
## Implementation notes for later

This feature should not be implemented as a quick relaxation of the current “one main” rule.

Instead, it should be implemented as a real scoped construct.

Likely AST concepts:

```text
TargetDecl
    name
    declarations
    optional main block
    prerequisites
    location
```

Implemented/current semantic checks:

```text
root program may have either:
    one root main block
or:
    one or more named targets

each target must have exactly one main block

target names must be unique within the program

target-local declarations must obey normal scoping rules

target-aware build/lowering must select an explicit target when more than one target exists
```

Possible future lowering behavior:

```text
program + target cli
    -> executable/tool named cli

program + target daemon
    -> service/daemon artifact

program + target test_runner
    -> test executable or test graph
```

## Deferred questions

- Can a program mix a root main and named target blocks?

- Should there be an implicit default target for single-main programs?

- Can targets import or depend on other targets?

- Are target-local prerequisites inherited from program scope?

- Should target outputs be executable binaries, graph packages, services,
  scripts, or all of these depending on backend?

- Should `entry` exist as a lighter-weight concept, or is
  `target { main { ... } }` sufficient?

- How should this appear in the visual graph/patchbay UI?

## Current implementation status

The current v0.25 implementation is strict about anonymous roots:

```text
one anonymous root main only
```

But preserve the future canon:

```text
multiple runnable products are modeled as named targets, not as multiple
anonymous root main blocks. The semantic boundary is implemented; target-aware
artifact selection is the next lowering boundary.
```
