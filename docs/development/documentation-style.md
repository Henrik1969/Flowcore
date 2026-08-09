# Documentation Style

This repository contains experimental language and system-architecture work. The documentation must therefore serve two audiences at once:

```text
active project development
competent newcomer orientation

The goal is not to make the project look more finished than it is. The goal is to make the current state understandable.

Documentation usability rule

A competent newcomer should quickly be able to answer:

What is this?
What is active now?
What is experimental?
How do I build it?
How do I test it?
Where do I read next?
What claims are not yet supported?

If a document does not help answer one of those questions, it should either be moved deeper into the documentation tree or clearly marked as design notes, historical material, or future work.

Preferred order

Use this order when possible:

orientation
current status
working commands
project map
design philosophy
deep architecture

Do not put deep philosophy before the reader knows where they are.

Define project names early

Important names should be defined before they are used heavily.

Minimum definitions:

Flowcore:
    The broader language/system architecture direction.

Flowmini:
    The executable prototype language used to test Flowcore ideas.

Flowmini v0.24 explicit AST:
    The current active implementation stage focused on making AST structure explicit, observable, and regression-tested.

FrankenCore:
    The wider operating-environment and system-composition project context.
Separate current fact from future direction

Use clear labels:

Current:
    What exists and is expected to work now.

Experimental:
    What exists but is incomplete or unstable.

Future:
    Intended direction that is not implemented yet.

Historical:
    Preserved earlier work that explains development history but is not the active implementation.

Do not describe future design as if it already works.

Prefer runnable commands

When documenting workflow, include commands that can be copied and run.

Example:

cd Flowmini/flowmini_v24_explicit_ast
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite

If a command depends on prior setup, say so.

Avoid hidden context

Avoid writing documentation that only makes sense after a private conversation.

Bad:

Now we do the bridge thing from before.

Better:

This stage connects the previously visible TokenTree layer to the explicit AST layer.
Tone

The tone should be plain, direct, and honest.

Good documentation says:

This works.
This is partial.
This is not implemented yet.
This is the next intended step.
Review test

Before committing public-facing documentation, check whether a competent outsider could answer:

Where am I?
What should I run?
What should I read next?
What should I not assume?

If not, improve the document.
