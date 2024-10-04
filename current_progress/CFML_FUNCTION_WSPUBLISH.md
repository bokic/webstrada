# Research: WSPUBLISH cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_wspublish()` in `src/cf8.cpp:17087` throws `"Function WSPUBLISH is not implemented"`.
- Compiler: `WSPUBLISH` is in the zero-arg not-implemented function list (`src/compiler.cpp:1748`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5743`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:17087` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1748`, symbol at `src/compiler.cpp:5743` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*wspublish*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:860` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (SOAP/Webservices) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_WSPUBLISH.md` |

## What WSPUBLISH does at the low C level

Publishes a message to a WebSocket channel, optionally targeting specific subscriber IDs.

- Arg 1: `channel` (string).
- Arg 2: `message` (any).
- Arg 3: `subscriberIds` (array).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `channel` (string). passed by value.
- Arg 2: `message` (any). passed by value.
- Arg 3: `subscriberIds` (array). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_wspublish(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `3` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing (already present); SOAP/XML envelope construction and WS-Security header support.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
