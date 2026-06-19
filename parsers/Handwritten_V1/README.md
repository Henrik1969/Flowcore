# LangLab handwritten parser 0.1-A

A deliberately inspectable handwritten parser for the first LangLab parser milestone.

## Contract

```text
stdin:
    LangLab token stream as JSON Lines

stdout:
    rich AST document as JSON

stderr:
    parser diagnostics

exit status:
    0 success
    non-zero syntax or token-stream failure
```

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run the included smoke test

```bash
./tests/smoke.sh
```

## Feed a lexer into the parser

```bash
flowlex < program.flow | ./build/flowparse > program.ast.json
```

or:

```bash
tablelex --rules config/flowcore.rules < program.flow \
    | ./build/flowparse \
    > program.ast.json
```

## Implemented syntax subset

```text
declarations:
    § x:int = 10

assignments:
    x = y + 20

multiline expressions:
    § total:int =
        price
        + freight
        + tax

expression pipelines:
    event
        => handlers[event.kind]()
        => accepted

projection, indexing, calls, unary and binary operators

flow blocks and graph wiring:
    flow Demo {
        src.out -> sink.in
    }

if / elseif / else
while
until
return
basic and generic type expressions
callable type expressions
```

## Deliberately deferred

```text
node declarations
record constructors
match
switch
guard
for and forall
transaction syntax
pattern destructuring
semantic checks
desugaring
```

The parser is intentionally incomplete but structurally serious. It proves the contract:

```text
token stream on stdin
    => handwritten parser
    => inspectable AST JSON on stdout
```
