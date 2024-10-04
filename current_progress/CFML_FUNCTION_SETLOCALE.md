# Research: SETLOCALE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_setlocale()` in `src/cf8.cpp:15785` throws `"Function SETLOCALE is not implemented"`.
- Compiler: `SETLOCALE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5618`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15785` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5618` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*setlocale*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:716` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SETLOCALE.md` |

## What SETLOCALE does at the low C level

Set the locale for the current request to `new_locale` (e.g. `en_US`), returning the previous locale. Affects LS functions for the rest of the request.

- Arg 1: `new_locale` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `new_locale` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_setlocale(const cfvariant *a1);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Locale state in the request context (LS functions must read it). The LS engine partially exists (LSIsNumeric/LSDateFormat implemented). Unit tests + tracker updates.

## Ease of implementation

Moderate. Request-context locale variable + validation against the locale table.
