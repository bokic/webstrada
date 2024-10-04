# Research: SPREADSHEETSETCELLCOMMENT cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_spreadsheetsetcellcomment()` in `src/cf8.cpp:16088` throws `"Function SPREADSHEETSETCELLCOMMENT is not implemented"`.
- Compiler: `SPREADSHEETSETCELLCOMMENT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1746`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5677`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:16088` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1746`, symbol at `src/compiler.cpp:5677` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spreadsheetsetcellcomment*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:777` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPREADSHEETSETCELLCOMMENT.md` |

## What SPREADSHEETSETCELLCOMMENT does at the low C level

Adds a comment to a spreadsheet cell.

- Arg 1: `spreadsheetObj` (any).
- Arg 2: `cell` (string).
- Arg 3: `comment` (string).
- Arg 4: `commentAuthor` (string).
- Arg 5: `bold` (boolean).
- Arg 6: `italic` (boolean).
- Arg 7: `fontColor` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `spreadsheetObj` (any). passed by value.
- Arg 2: `cell` (string). passed by value.
- Arg 3: `comment` (string). passed by value.
- Arg 4: `commentAuthor` (string). passed by value.
- Arg 5: `bold` (boolean). passed by value.
- Arg 6: `italic` (boolean). passed by value.
- Arg 7: `fontColor` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spreadsheetsetcellcomment(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `7` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) read/write library (e.g. libxlsxwriter/libxl or a POI-equivalent); binary BIFF and XML formats; ZIP for xlsx container.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
