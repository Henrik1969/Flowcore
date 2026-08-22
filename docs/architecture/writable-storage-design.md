# Writable storage design boundary

## Current compatibility behavior

The lowering-plan v1 projection currently interprets a positive
`c_pointer(N)` initializer as a call-lifetime, read/write byte allocation. This
is a migration aid for native APIs such as `read`, `uname`, and
`clock_gettime`; it is not established permanent Flow language semantics.
Allocation size, access, and lifetime are explicit in the lowering plan, and
binding rejects zero-sized or malformed descriptors.

`sel` now initializes its compatibility allocation through the authorized
`memset` ABI operation before reading into it. Positive, EOF, and error behavior
is expressed by Flow branches. The backend does not infer zero-initialization
or read-result policy from `c_pointer`.

## Candidate permanent representations

### Dedicated buffer carrier

Add a carrier such as `buffer<byte, N>` with length, initialized extent,
mutability, and bounded indexing. ABI conversions to borrowed `c_pointer` and
`c_string` would be explicit and checked. This gives the strongest type and
bounds model, but expands carrier compatibility and generic/type syntax.

### Storage declaration

Add a declaration form such as `storage input : byte[4096]`. Its address,
capacity, initialization state, and lifetime would be language facts rather
than encoded in an ABI carrier initializer. This keeps allocation separate from
FFI types, but requires new AST, symbol, semantic-plan, and control-flow rules.

### Explicit allocation operation

Represent allocation as an effectful provider or language operation returning
a typed storage handle, paired with explicit release for non-lexical lifetime.
This naturally models heap and provider-owned storage, but makes simple stack
buffers depend on effect and cleanup policy and still needs a bounded view type.

## Required semantic choice

Henrik must choose whether writable storage is primarily a value/type
(`buffer`), a declaration/lifetime construct (`storage`), or an effectful
operation (`allocate`). The choice determines ownership, initialization,
bounds, pointer/string conversion, cleanup, and whether fixed local storage can
remain non-effectful.

The smallest recommended permanent direction is a dedicated bounded buffer
carrier with lexical storage by default and explicit ABI borrow conversions.
Until that public-language decision is made, positive `c_pointer(N)` remains a
documented compatibility representation only and must not acquire additional
implicit semantics.
