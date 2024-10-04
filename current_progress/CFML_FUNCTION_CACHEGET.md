# Research: CACHEGET cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_cacheget()` in `src/cf8.cpp:11911` throws `"Function CACHEGET is not implemented"`.
- Compiler: `CACHEGET` is in the zero-arg not-implemented function list (`src/compiler.cpp:1732`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5253`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11911` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1732`, symbol at `src/compiler.cpp:5253` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*cacheget*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:269` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Cache*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CACHEGET.md` |

## What CACHEGET does at the low C level

Return the cached object for `id` in the default region (or `region`), or empty string if not cached. Implemented as the JCache `CacheGet`.

- Arg 1: `id` (string, required).
- Arg 2: `region` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `id` (string). passed by value.
- Arg 2: `region` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_cacheget(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

A cache backend (JCache-like key/value with regions and expiry) — none exists. Unit tests + tracker updates.

## Ease of implementation

Moderate. In-memory TTL map with region namespaces; the region model is the main design work.
