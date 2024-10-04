# Research: ORMSEARCHOFFLINE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_ormsearchoffline()` in `src/cf8.cpp:14890` throws `"Function ORMSEARCHOFFLINE is not implemented"`.
- Compiler: `ORMSEARCHOFFLINE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1743`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5569`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14890` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1743`, symbol at `src/compiler.cpp:5569` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*ormsearchoffline*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:655` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (ORM/Entity) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ORMSEARCHOFFLINE.md` |

## What ORMSEARCHOFFLINE does at the low C level

Performs an offline search against the ORM index (for bulk/background indexing workflows).

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_ormsearchoffline();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

ORM search engine. Unit tests + tracker updates.

## Ease of implementation

Hard.
