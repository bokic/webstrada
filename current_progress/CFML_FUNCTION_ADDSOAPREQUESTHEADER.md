# Research: ADDSOAPREQUESTHEADER cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_addsoaprequestheader()` in `src/cf8.cpp:11015` throws `"Function ADDSOAPREQUESTHEADER is not implemented"`.
- Compiler: `ADDSOAPREQUESTHEADER` is in the zero-arg not-implemented function list (`src/compiler.cpp:1732`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5235`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11015` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1732`, symbol at `src/compiler.cpp:5235` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*addsoaprequestheader*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:215` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (SOAP/Webservices) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ADDSOAPREQUESTHEADER.md` |

## What ADDSOAPREQUESTHEADER does at the low C level

Adds a SOAP header to a SOAP request; key/value pairs, or an XML node, are appended to the request sent by a webservice call.

- Arg 1: `namespace` (string).
- Arg 2: `name` (string).
- Arg 3: `value` (any).
- Arg 4: `mustUnderstand` (boolean).
- Arg 5: `actor` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `namespace` (string). passed by value.
- Arg 2: `name` (string). passed by value.
- Arg 3: `value` (any). passed by value.
- Arg 4: `mustUnderstand` (boolean). passed by value.
- Arg 5: `actor` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_addsoaprequestheader(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `5` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing (already present); SOAP/XML envelope construction and WS-Security header support.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
