# Research: SENDSAMLLOGOUTRESPONSE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_sendsamllogoutresponse()` in `src/cf8.cpp:15721` throws `"Function SENDSAMLLOGOUTRESPONSE is not implemented"`.
- Compiler: `SENDSAMLLOGOUTRESPONSE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5608`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15721` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5608` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*sendsamllogoutresponse*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:706` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SENDSAMLLOGOUTRESPONSE.md` |

## What SENDSAMLLOGOUTRESPONSE does at the low C level

Sends a SAML logout response to the IDP using the given logout request and response settings.

- Arg 1: `logoutRequest` (string).
- Arg 2: `response` (string).
- Arg 3: `relayState` (string).
- Arg 4: `inResponseTo` (string).
- Arg 5: `issuer` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `logoutRequest` (string). passed by value.
- Arg 2: `response` (string). passed by value.
- Arg 3: `relayState` (string). passed by value.
- Arg 4: `inResponseTo` (string). passed by value.
- Arg 5: `issuer` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_sendsamllogoutresponse(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `5` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing; SAML logout-response document validation; HTTPS signing.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
