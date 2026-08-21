# Frankencore Clock reference object

This is the first small reference implementation for the Frankencore
constitutional model.

```text
Linux clock_gettime/clock_getres
        -> Linux adapter/provider
        -> Clock semantic object
        -> capability and policy facts
        -> JSON/CLI projection
```

It demonstrates:

- semantic type: `Clock`;
- stable identity within the report: `clock:monotonic` or `clock:realtime`;
- typed properties and resolution;
- discoverable `frankencore.clock.read` capability version 1;
- explicit provider/backend distinction;
- an explicit policy decision;
- structured error output for syscall failure;
- a CLI projection independent of future GUI or Flowcore projections.

This is not yet a general Frankencore runtime or authority system. The
`reference-default` policy is intentionally narrow and only demonstrates the
separation between capability and policy.
