# Research: SPREADSHEETADDSPLITPANE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_spreadsheetaddsplitpane()` in `src/cf8.cpp:15932` throws `"Function SPREADSHEETADDSPLITPANE is not implemented"`.
- Compiler: `SPREADSHEETADDSPLITPANE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1745`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5638`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15932` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1745`, symbol at `src/compiler.cpp:5638` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spreadsheetaddsplitpane*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:738` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPREADSHEETADDSPLITPANE.md` |

## What SPREADSHEETADDSPLITPANE does at the low C level

Adds a split pane to a spreadsheet at the given row/column; a pane splits the window into scrollable areas.

- Arg 1: `spreadsheetObj` (any).
- Arg 2: `splitRow` (numeric).
- Arg 3: `splitColumn` (numeric).
- Arg 4: `topRow` (numeric).
- Arg 5: `leftmostColumn` (numeric).
- Arg 6: `activePane` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `spreadsheetObj` (any). passed by value.
- Arg 2: `splitRow` (numeric). passed by value.
- Arg 3: `splitColumn` (numeric). passed by value.
- Arg 4: `topRow` (numeric). passed by value.
- Arg 5: `leftmostColumn` (numeric). passed by value.
- Arg 6: `activePane` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spreadsheetaddsplitpane(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `6` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) streaming reader/writer library (e.g. libxlsxwriter or a POI-equivalent); ZIP container handling; memory-efficient row streaming.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
