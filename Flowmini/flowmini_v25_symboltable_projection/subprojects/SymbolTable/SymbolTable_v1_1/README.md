# SymbolTable v1.1

Policy-free symbol table for language development, written in C++20.

This version keeps the core `SymbolTable` as factual storage and adds a separate
view/policy layer:

```text
SymbolTable      stores facts
ISymbolPolicy    interprets facts for a specific view
SymbolTreeView   exposes a filtered projection over the table
Semantic layer   decides what language-law means
```

## Design law

```text
The SymbolTable stores facts.
The Policy layer interprets facts.
The TreeView exposes a policy-filtered projection.
The Semantic Analyzer makes language decisions.
```

`private`, `public`, `deprecated`, `imported`, `generated`, `ABI tag`, and similar
properties are stored as facts/factoids. The raw table never rejects lookup.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j20
ctest --test-dir build --output-on-failure
```

## Targets

Depending on build options:

```text
SymbolTable::static
SymbolTable::shared
SymbolTable::headers
```

The generated library files are:

```text
libsymboltable.a
libsymboltable.so
```

## v1.1 additions

```text
ISymbolPolicy
ExposeAllPolicy
HidePrivatePolicy
SymbolTreeView
LookupDecision
policy-aware inspection
policy-filtered symbol lists
policy-filtered name resolution
```

`HidePrivatePolicy` is intentionally only a demonstration policy. It proves that
visibility can be interpreted outside the raw symbol table. Real language
frontends should provide their own policy classes.

## Example

```cpp
using namespace symboltable;

SymbolTable table;
auto global = table.globalScope();

auto klass = table.insertSymbol(global, "Box", SymbolKind::Class);
auto klassScope = table.createScope(ScopeKind::Class, global, klass, "Box");

auto secret = table.insertSymbol(klassScope, "secret", SymbolKind::Field);
table.symbol(secret).visibility = Visibility::Private;

HidePrivatePolicy policy;
SymbolTreeView view{table, policy, global};

auto visible = view.visibleSymbolsIn(klassScope);
auto inspected = view.inspectSymbolsIn(klassScope);
```

The raw table still sees everything:

```cpp
auto raw = table.findLocal(klassScope, "secret");
```

The tree view decides what the current viewpoint may see.

## Current tests

The test suite verifies:

```text
global scope creation
symbol insertion
local lookup
duplicate/overload preservation
upward scope resolution
symbol-introduced scopes
factoid attachment
ExposeAllPolicy view
HidePrivatePolicy view
raw table remains unmodified by policy filtering
debug dump
```
