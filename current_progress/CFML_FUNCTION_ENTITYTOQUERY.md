# Research: ENTITYTOQUERY cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_entitytoquery()` in `src/cf8.cpp:12932` throws `"Function ENTITYTOQUERY is not implemented"`.
- Compiler: `ENTITYTOQUERY` is in the zero-arg not-implemented function list (`src/compiler.cpp:1735`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5330`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12932` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1735`, symbol at `src/compiler.cpp:5330` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*entitytoquery*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:364` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (ORM/Entity) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ENTITYTOQUERY.md` |

## What ENTITYTOQUERY does at the low C level

Convert an entity or array of entities into a query object, using `propertyNames` (default: all properties).

- Arg 1: `entity` (any, required).
- Arg 2: `propertyNames` (array, optional).
- Arg 3: `timeout` (numeric, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `entity` (any). passed by value.
- Arg 2: `propertyNames` (array). passed by value.
- Arg 3: `timeout` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_entitytoquery(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

ORM engine + query-object construction (query objects exist for cfquery). Unit tests + tracker updates.

## Ease of implementation

Moderate-to-hard once ORM exists.
