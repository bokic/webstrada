# Research: DUPLICATE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_duplicate()` in `src/cf8.cpp:12683` throws `"Function DUPLICATE is not implemented"`.
- Compiler: `DUPLICATE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1734`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5309`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12683` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1734`, symbol at `src/compiler.cpp:5309` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*duplicate*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:343` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_DUPLICATE.md` |

## What DUPLICATE does at the low C level

Return a deep copy of `object` (nested structs/arrays/strings/etc.). For queries, a new query object with copied data. `deepcopy` (Lucee-only) controls child cloning; CF always deep-copies.

- Arg 1: `object` (any, required).
- Arg 2: `deepcopy` (boolean, optional, default true).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `object` (any). passed by value.
- Arg 2: `deepcopy` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_duplicate(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Deep-copy walk over the variant model (recursive struct/array clone). The variant type system already models structs/arrays/queries. Unit tests + tracker updates.

## Ease of implementation

Moderate. Recursive clone of the variant tree; cycle handling and type fidelity need care.
