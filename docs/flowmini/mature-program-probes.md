# Mature Flowmini program probes

These programs are verification probes. They are intentionally small enough to
remain deterministic and do not imply that the missing facilities are solved.

| Probe | Exercises | Evidence | Pressure exposed |
| --- | --- | --- | --- |
| `examples/apps/flowstats/flowstats.flow` | list construction/indexing, `length`, `while`, guards, constants, arithmetic, `std/math` | `flowmini_mature_programs`; output `5`, `1`, `9`, `25` | collection helpers cannot yet be abstracted over `T`; loop mutation is verbose |
| `examples/apps/flowconfig/flowconfig.flow` | enum declaration, `when`, guard, constant declaration, branch output | `flowmini_mature_programs`; production branch outputs `20` | configuration parsing and file/CLI APIs are not yet standard units |
| `examples/flowcat/flowcat.flow` | external C provider, file descriptor lifecycle, capability policy | `flowcat_flowcore_pipeline` | provider contracts expose effects, but resource cleanup is descriptive |

The attempted generic collection helper is recorded in
[`FUTURE_WORK_EVIDENCE.md`](../FUTURE_WORK_EVIDENCE.md): an imported
`length(list<T>)` declaration is rejected by the current callable type path.
The current probes therefore use concrete values and existing intrinsics until
generic substitution has a canonical representation.
