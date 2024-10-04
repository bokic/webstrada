# Research: ISPDFARCHIVE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_ispdfarchive()` in `src/cf8.cpp:14236` throws `"Function ISPDFARCHIVE is not implemented"`.
- Compiler: `ISPDFARCHIVE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1740`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5496`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14236` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1740`, symbol at `src/compiler.cpp:5496` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*ispdfarchive*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:554` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISPDFARCHIVE.md` |

## What ISPDFARCHIVE does at the low C level

Returns true if the value is a PDF/A (PDF archive) file or binary.

- Arg 1: `value` (any).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `value` (any). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_ispdfarchive(const cfvariant *a1);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `1` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

File-format sniffing (PDF magic bytes, ZIP/OLE container detection); for PDF/A a PDF library or parser.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
