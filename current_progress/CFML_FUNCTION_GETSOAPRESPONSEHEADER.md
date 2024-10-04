# Research: GETSOAPRESPONSEHEADER cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_getsoapresponseheader()` in `src/cf8.cpp:13632` throws `"Function GETSOAPRESPONSEHEADER is not implemented"`.
- Compiler: `GETSOAPRESPONSEHEADER` is in the zero-arg not-implemented function list (`src/compiler.cpp:1737`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5392`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13632` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1737`, symbol at `src/compiler.cpp:5392` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*getsoapresponseheader*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:442` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (SOAP/Webservices) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETSOAPRESPONSEHEADER.md` |

## What GETSOAPRESPONSEHEADER does at the low C level

Returns an XML object containing the SOAP response header of a webservice call.

- Arg 1: `webservice` (any).
- Arg 2: `name` (string).
- Arg 3: `namespace` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `webservice` (any). passed by value.
- Arg 2: `name` (string). passed by value.
- Arg 3: `namespace` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_getsoapresponseheader(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `3` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing (already present); SOAP/XML envelope construction and WS-Security header support.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
