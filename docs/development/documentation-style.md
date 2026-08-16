# Documentation style

This repository contains experimental language and system-architecture work.
Documentation must serve active development and competent newcomer orientation
without making the project look more finished than it is.

## Usability rule

A competent newcomer should quickly be able to answer:

- What is this?
- What is active now?
- What is experimental or historical?
- How do I build and test it?
- Where do I read next?
- Which claims are not supported yet?

If a document does not help answer those questions, place it deeper in the
documentation tree or label it explicitly as design, historical evidence, or
future work.

## Preferred order

Use this order when practical:

1. orientation;
2. current status;
3. working commands;
4. project map;
5. design philosophy;
6. deep architecture.

Do not put deep philosophy before the reader knows where they are.

## Define project names early

- Flowcore is the broader language and system-architecture direction.
- Flowmini is the executable prototype language used to test Flowcore ideas.
- Flowmini v0.25 SymbolTable projection maturation is the active stage. It
  hardens factual structural origins and an independent-consumer boundary
  before semantic analysis.
- Flowmini v0.24 explicit AST is the closed raw frontend/export checkpoint
  inherited by v0.25.
- FrankenCore is the constitutional system and capability layer above the
  living substrate.

## Separate fact from direction

Use clear authority labels:

- **Current:** observed behavior expected to work now.
- **Experimental:** implemented but incomplete or unstable.
- **Future:** intended direction not yet implemented.
- **Historical:** preserved evidence that is not active implementation.
- **Binding:** approved law or boundary an implementation must preserve.
- **Provisional:** a mechanism or spelling that remains open to revision.

Do not describe future design as implemented fact. Do not rewrite historical
gate counts to match a later version.

## Prefer runnable commands

Document prerequisite setup and use commands that can be copied:

```bash
cd Flowmini/flowmini_v25_symboltable_projection
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_symbol_projection_tests
cmake --build cmake-build-debug --target flowmini_frontend_bundle_tests
cmake --build cmake-build-debug --target flowmini_suite
```

## Avoid hidden context

Avoid text that only makes sense after a private conversation.

Weak:

> Now we do the bridge thing from before.

Better:

> This stage connects the visible TokenTree layer to the explicit AST layer.

## Tone and review

Use plain, direct language:

```text
This works.
This is partial.
This is not implemented yet.
This is the next intended step.
```

Before committing public documentation, verify that a competent outsider can
identify where they are, what to run, what to read next, and what not to assume.
