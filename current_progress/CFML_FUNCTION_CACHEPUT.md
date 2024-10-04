# Research: CACHEPUT cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_cacheput()` in `src/cf8.cpp:11935` throws `"Function CACHEPUT is not implemented"`.
- Compiler: `CACHEPUT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1732`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5259`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11935` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1732`, symbol at `src/compiler.cpp:5259` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*cacheput*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:275` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Cache*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CACHEPUT.md` |

## What CACHEPUT does at the low C level

Store `value` under `id` in the region with optional lifetime `timeSpan` and idle `idleTime` (CF timespans).

- Arg 1: `id` (string, required).
- Arg 2: `value` (any, required).
- Arg 3: `timeSpan` (timespan, optional).
- Arg 4: `idleTime` (timespan, optional).
- Arg 5: `region` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `id` (string). passed by value.
- Arg 2: `value` (any). passed by value.
- Arg 3: `timeSpan` (timespan). passed by value.
- Arg 4: `idleTime` (timespan). passed by value.
- Arg 5: `region` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_cacheput(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Cache backend + CF timespan type support. Unit tests + tracker updates.

## Ease of implementation

Moderate. TTL/idle expiry logic.
