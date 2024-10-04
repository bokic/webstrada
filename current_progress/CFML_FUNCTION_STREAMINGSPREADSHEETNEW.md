# Research: STREAMINGSPREADSHEETNEW cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_streamingspreadsheetnew()` in `src/cf8.cpp:16188` throws `"Function STREAMINGSPREADSHEETNEW is not implemented"`.
- Compiler: `STREAMINGSPREADSHEETNEW` is in the zero-arg not-implemented function list (`src/compiler.cpp:1747`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5702`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:16188` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1747`, symbol at `src/compiler.cpp:5702` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*streamingspreadsheetnew*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:803` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_STREAMINGSPREADSHEETNEW.md` |

## What STREAMINGSPREADSHEETNEW does at the low C level

Creates a streaming spreadsheet writer object; used with StreamingSpreadSheetProcess.

- Arg 1: `sheetname` (string).
- Arg 2: `xmlFormat` (boolean).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `sheetname` (string). passed by value.
- Arg 2: `xmlFormat` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_streamingspreadsheetnew(const cfvariant *a1, const cfvariant *a2);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `2` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) streaming reader/writer library (e.g. libxlsxwriter or a POI-equivalent); ZIP container handling; memory-efficient row streaming.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
