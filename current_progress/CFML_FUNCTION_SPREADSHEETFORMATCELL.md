# Research: SPREADSHEETFORMATCELL cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_spreadsheetformatcell()` in `src/cf8.cpp:15956` throws `"Function SPREADSHEETFORMATCELL is not implemented"`.
- Compiler: `SPREADSHEETFORMATCELL` is in the zero-arg not-implemented function list (`src/compiler.cpp:1745`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5644`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15956` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1745`, symbol at `src/compiler.cpp:5644` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spreadsheetformatcell*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:744` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPREADSHEETFORMATCELL.md` |

## What SPREADSHEETFORMATCELL does at the low C level

Formats a single cell (font, color, alignment, data format, etc.) of a spreadsheet.

- Arg 1: `spreadsheetObj` (any).
- Arg 2: `dataFormat` (string).
- Arg 3: `font` (string).
- Arg 4: `fontSize` (numeric).
- Arg 5: `fontBold` (boolean).
- Arg 6: `fontItalic` (boolean).
- Arg 7: `fontColor` (string).
- Arg 8: `alignment` (string).
- Arg 9: `verticalAlignment` (string).
- Arg 10: `textWrap` (boolean).
- Arg 11: `fill` (string).
- Arg 12: `border` (string).
- Arg 13: `borderColor` (string).
- Arg 14: `format` (struct).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `spreadsheetObj` (any). passed by value.
- Arg 2: `dataFormat` (string). passed by value.
- Arg 3: `font` (string). passed by value.
- Arg 4: `fontSize` (numeric). passed by value.
- Arg 5: `fontBold` (boolean). passed by value.
- Arg 6: `fontItalic` (boolean). passed by value.
- Arg 7: `fontColor` (string). passed by value.
- Arg 8: `alignment` (string). passed by value.
- Arg 9: `verticalAlignment` (string). passed by value.
- Arg 10: `textWrap` (boolean). passed by value.
- Arg 11: `fill` (string). passed by value.
- Arg 12: `border` (string). passed by value.
- Arg 13: `borderColor` (string). passed by value.
- Arg 14: `format` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spreadsheetformatcell(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7, const cfvariant *a8, const cfvariant *a9, const cfvariant *a10, const cfvariant *a11, const cfvariant *a12, const cfvariant *a13, const cfvariant *a14);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `14` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) read/write library (e.g. libxlsxwriter/libxl or a POI-equivalent); binary BIFF and XML formats; ZIP for xlsx container.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
