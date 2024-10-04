# Research: LSNumberFormat cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_lsnumberformat()` in `src/cf8.cpp:14759` throws `"Function LSNumberFormat is not implemented"`.
- Compiler: `LSNUMBERFORMAT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5542`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14759` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5542` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*lsnumberformat*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:621` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LSNUMBERFORMAT.md` |

## What LSNumberFormat does at the low C level

Per Adobe CF: format a number using a locale-specific mask (`_,9` digit placeholder; `.` decimal point; `0` zero-pad). Same mask grammar as `NumberFormat` but with locale-specific separators.

- Arg 1: `number` (numeric).
- Arg 2: `mask` (optional, default `_9`).
- Arg 3: `locale` (optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `number` (numeric). passed by value.
- Arg 2: `mask` (optional, default `_9`). passed by value.
- Arg 3: `locale` (optional). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_lsnumberformat(const cfvariant *number, const cfvariant *mask, const cfvariant *locale);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation using the existing locale infrastructure; shares mask logic with `NumberFormat`.
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

Locale infrastructure exists.

## Ease of implementation

Moderate. Mask grammar is shared with `NumberFormat`; the locale part changes separators/grouping.
