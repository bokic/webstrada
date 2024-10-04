# Research: ONWSAUTHENTICATE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_onwsauthenticate()` in `src/cf8.cpp:14826` throws `"Function ONWSAUTHENTICATE is not implemented"`.
- Compiler: `ONWSAUTHENTICATE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5553`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14826` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5553` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*onwsauthenticate*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:639` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (SOAP/Webservices) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ONWSAUTHENTICATE.md` |

## What ONWSAUTHENTICATE does at the low C level

Invokes an authentication callback; used with cfwsdl/webservices to validate credentials.

- Arg 1: `username` (string).
- Arg 2: `password` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `username` (string). passed by value.
- Arg 2: `password` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_onwsauthenticate(const cfvariant *a1, const cfvariant *a2);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `2` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing (already present); SOAP/XML envelope construction and WS-Security header support.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
