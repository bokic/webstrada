# Research: ENTITYLOADBYPK cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_entityloadbypk()` in `src/cf8.cpp:12912` throws `"Function ENTITYLOADBYPK is not implemented"`.
- Compiler: `ENTITYLOADBYPK` is in the zero-arg not-implemented function list (`src/compiler.cpp:1735`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5325`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12912` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1735`, symbol at `src/compiler.cpp:5325` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*entityloadbypk*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:359` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (ORM/Entity) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ENTITYLOADBYPK.md` |

## What ENTITYLOADBYPK does at the low C level

Load a single entity by its primary-key value, or empty string if not found.

- Arg 1: `entityName` (string, required).
- Arg 2: `primaryKey` (any, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `entityName` (string). passed by value.
- Arg 2: `primaryKey` (any). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_entityloadbypk(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

ORM engine + PK mapping. Unit tests + tracker updates.

## Ease of implementation

Hard.
