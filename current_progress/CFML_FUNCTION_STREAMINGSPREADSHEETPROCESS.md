# Research: STREAMINGSPREADSHEETPROCESS cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_streamingspreadsheetprocess()` in `src/cf8.cpp:16192` throws `"Function STREAMINGSPREADSHEETPROCESS is not implemented"`.
- Compiler: `STREAMINGSPREADSHEETPROCESS` is in the zero-arg not-implemented function list (`src/compiler.cpp:1747`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5703`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:16192` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1747`, symbol at `src/compiler.cpp:5703` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*streamingspreadsheetprocess*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:804` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_STREAMINGSPREADSHEETPROCESS.md` |

## What STREAMINGSPREADSHEETPROCESS does at the low C level

Applies the given SQL to a streaming spreadsheet writer, or sets a sheet name.

- Arg 1: `streamingSpreadsheet` (any).
- Arg 2: `SQL` (string).
- Arg 3: `sheetname` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `streamingSpreadsheet` (any). passed by value.
- Arg 2: `SQL` (string). passed by value.
- Arg 3: `sheetname` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_streamingspreadsheetprocess(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `3` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) streaming reader/writer library (e.g. libxlsxwriter or a POI-equivalent); ZIP container handling; memory-efficient row streaming.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
