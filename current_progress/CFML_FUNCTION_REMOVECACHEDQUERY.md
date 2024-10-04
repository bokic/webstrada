# Research: REMOVECACHEDQUERY cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_removecachedquery()` in `src/cf8.cpp:15674` throws `"Function REMOVECACHEDQUERY is not implemented"`.
- Compiler: `REMOVECACHEDQUERY` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5598`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15674` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5598` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*removecachedquery*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:688` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Cache*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REMOVECACHEDQUERY.md` |

## What REMOVECACHEDQUERY does at the low C level

Remove the cached query object with `queryName` (the `cachedwithin`/`cachedafter` cache key) from the region.

- Arg 1: `queryName` (string, required).
- Arg 2: `region` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `queryName` (string). passed by value.
- Arg 2: `region` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_removecachedquery(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Cache backend + cfquery cache-key scheme (cfquery cache is unimplemented). Unit tests + tracker updates.

## Ease of implementation

Moderate. Depends on cfquery caching.
