# Research: STREAMINGSPREADSHEETISXMLFORMAT cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: present — `cfml::cf_streamingspreadsheetisxmlformat()` in `src/cf8.cpp` throws `Function StreamingSpreadsheetIsXmlFormat is not implemented` (a "will not implement" stub, matching every other not-implemented function).
- Compiler: `STREAMINGSPREADSHEETISXMLFORMAT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1897`); the JIT call resolves via the registered `cf_streamingspreadsheetisxmlformat` symbol.
- Interpreter (`evalFunction`) dispatch: none directly; the name is in `kBuiltinFunctionNames` (`src/cf8.cpp:150`) so the interpreter treats it as a known function (`Unknown function call` on the cfoutput path) and the `cfvariant_call_function` JIT-delegation path routes to the stub.
- Symbol registered at `src/compiler.cpp:7012` (`DynamicLibrary::AddSymbol("cf_streamingspreadsheetisxmlformat", ...)`).

## Implemented: 0% (stub throws "not implemented")

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Stub present (throws `Function StreamingSpreadsheetIsXmlFormat is not implemented`) | `src/cf8.cpp` |
| Compiler wiring | ✅ In zero-arg not-implemented list + `AddSymbol` registered | `src/compiler.cpp:1897`, `:7012` |
| Interpreter dispatch | ⚠️ Known-function-name list entry; falls through to JIT delegation / `Unknown function call` | `src/cf8.cpp:150` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Unit: `JitExpressionTest.UnimplementedStubsThrow` (JIT path throws the standard exception) | `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` row exists (`❌ No`, stub note); in `UNIMPLEMENTED_FUNCTIONS.md` (Spreadsheet*, will-not-implement) | — |
| Docs/spec | ⚠️ No spec reference (docs are auto-generated placeholders) | — |

## What STREAMINGSPREADSHEETISXMLFORMAT does at the low C level

Returns true if the given file is in the XML (Excel 2003) spreadsheet format.

- Arg 1: `filepath` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `filepath` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_streamingspreadsheetisxmlformat(const cfvariant *a1);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `1` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Excel (xlsx) streaming reader/writer library (e.g. libxlsxwriter or a POI-equivalent); ZIP container handling; memory-efficient row streaming.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
