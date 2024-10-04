# Research: RESTDELETEAPPLICATION cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_restdeleteapplication()` in `src/cf8.cpp:15705` throws `"Function RESTDELETEAPPLICATION is not implemented"`.
- Compiler: `RESTDELETEAPPLICATION` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5603`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15705` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5603` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*restdeleteapplication*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:696` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (REST) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_RESTDELETEAPPLICATION.md` |

## What RESTDELETEAPPLICATION does at the low C level

Removes the REST application (from the given root path) so its settings are cleaned up from the REST registry.

- Arg 1: `rootPath` (string).
- Arg 2: `stopServer` (boolean).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `rootPath` (string). passed by value.
- Arg 2: `stopServer` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_restdeleteapplication(const cfvariant *a1, const cfvariant *a2);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `2` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

HTTP request handling; REST registry for root-path based applications.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
