# Research: THROW cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_throw()` in `src/cf8.cpp:16587` throws `"Function THROW is not implemented"`.
- Compiler: `THROW` is in the zero-arg not-implemented function list (`src/compiler.cpp:1748`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5721`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:16587` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1748`, symbol at `src/compiler.cpp:5721` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*throw*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:835` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_THROW.md` |

## What THROW does at the low C level

Throw a CFML exception of `type` (default `Custom`) with `message`/`detail`/`errorcode`/`extendedinfo`. Throwing a struct with `type`/`message` keys is also supported when `object` is given. The function-form of `<cfthrow>`.

- Arg 1: `message` (string, optional).
- Arg 2: `type` (string, optional, default Custom).
- Arg 3: `detail` (string, optional).
- Arg 4: `errorcode` (string, optional).
- Arg 5: `extendedinfo` (string, optional).
- Arg 6: `object` (any, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `message` (string). passed by value.
- Arg 2: `type` (string). passed by value.
- Arg 3: `detail` (string). passed by value.
- Arg 4: `errorcode` (string). passed by value.
- Arg 5: `extendedinfo` (string). passed by value.
- Arg 6: `object` (any). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_throw(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

An exception object model with type/message/detail that can be caught by `cftry`/`catch` (cftry exists). Unit tests + tracker updates.

## Ease of implementation

Moderate. Construct an exception variant and throw it; the catch machinery already exists.
