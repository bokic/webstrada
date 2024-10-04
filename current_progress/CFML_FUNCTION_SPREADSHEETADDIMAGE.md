# Research: SPREADSHEETADDIMAGE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_spreadsheetaddimage()` in `src/cf8.cpp:15908` throws `"Function SPREADSHEETADDIMAGE is not implemented"`.
- Compiler: `SPREADSHEETADDIMAGE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1745`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5632`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15908` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1745`, symbol at `src/compiler.cpp:5632` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spreadsheetaddimage*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:732` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPREADSHEETADDIMAGE.md` |

## What SPREADSHEETADDIMAGE does at the low C level

Adds an image to a spreadsheet at the given cell/range, optionally with anchor and scaling.

- Arg 1: `spreadsheetObj` (any).
- Arg 2: `image` (string).
- Arg 3: `startRow` (numeric).
- Arg 4: `startColumn` (numeric).
- Arg 5: `endRow` (numeric).
- Arg 6: `endColumn` (numeric).
- Arg 7: `anchorType` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `spreadsheetObj` (any). passed by value.
- Arg 2: `image` (string). passed by value.
- Arg 3: `startRow` (numeric). passed by value.
- Arg 4: `startColumn` (numeric). passed by value.
- Arg 5: `endRow` (numeric). passed by value.
- Arg 6: `endColumn` (numeric). passed by value.
- Arg 7: `anchorType` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spreadsheetaddimage(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `7` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) read/write library (e.g. libxlsxwriter/libxl or a POI-equivalent); binary BIFF and XML formats; ZIP for xlsx container.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
