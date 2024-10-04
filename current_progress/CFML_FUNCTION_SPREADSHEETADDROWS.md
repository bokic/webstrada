# Research: SPREADSHEETADDROWS cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_spreadsheetaddrows()` in `src/cf8.cpp:15928` throws `"Function SPREADSHEETADDROWS is not implemented"`.
- Compiler: `SPREADSHEETADDROWS` is in the zero-arg not-implemented function list (`src/compiler.cpp:1745`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5637`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15928` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1745`, symbol at `src/compiler.cpp:5637` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spreadsheetaddrows*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:737` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPREADSHEETADDROWS.md` |

## What SPREADSHEETADDROWS does at the low C level

Adds multiple rows of data to a spreadsheet from a query, array, or comma-delimited list.

- Arg 1: `spreadsheetObj` (any).
- Arg 2: `data` (query).
- Arg 3: `rowNumber` (numeric).
- Arg 4: `columnNumber` (numeric).
- Arg 5: `includeQueryColumns` (boolean).
- Arg 6: `includeColumnNames` (boolean).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `spreadsheetObj` (any). passed by value.
- Arg 2: `data` (query). passed by value.
- Arg 3: `rowNumber` (numeric). passed by value.
- Arg 4: `columnNumber` (numeric). passed by value.
- Arg 5: `includeQueryColumns` (boolean). passed by value.
- Arg 6: `includeColumnNames` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spreadsheetaddrows(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `6` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) read/write library (e.g. libxlsxwriter/libxl or a POI-equivalent); binary BIFF and XML formats; ZIP for xlsx container.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
