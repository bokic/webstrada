# Research: LSEUROCURRENCYFORMAT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_lseurocurrencyformat()` in `src/cf8.cpp:14733` throws `"Function LSEUROCURRENCYFORMAT is not implemented"`.
- Compiler: `LSEUROCURRENCYFORMAT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5538`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14733` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5538` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*lseurocurrencyformat*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:617` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LSEUROCURRENCYFORMAT.md` |

## What LSEUROCURRENCYFORMAT does at the low C level

Format a number as a euro currency value for the current locale (or a given `locale`). Types: `none`/`local`/`international`. Uses locale-specific currency/grouping rules; the shared LS formatting engine is used.

- Arg 1: `number` (number, required).
- Arg 2: `type` (string, optional, default local).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `number` (number). passed by value.
- Arg 2: `type` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_lseurocurrencyformat(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Locale-aware number/currency formatter (the LS engine; `LSNumberFormat`/`LSCurrencyFormat` share it). Unit tests + tracker updates.

## Ease of implementation

Moderate. Same number-formatting engine as `LSNumberFormat`; needs euro symbol/placement per locale.
