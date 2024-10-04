# Research: GETSAFEHTML cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_getsafehtml()` in `src/cf8.cpp:13608` throws `"Function GETSAFEHTML is not implemented"`.
- Compiler: `GETSAFEHTML` is in the zero-arg not-implemented function list (`src/compiler.cpp:1737`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5386`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13608` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1737`, symbol at `src/compiler.cpp:5386` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*getsafehtml*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:436` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETSAFEHTML.md` |

## What GETSAFEHTML does at the low C level

Sanitize `inputString` HTML against an OWASP AntiSamy policy, returning only the safe HTML (removing scripts, event handlers, etc.). `PolicyFile` selects a policy XML.

- Arg 1: `inputString` (string, required).
- Arg 2: `PolicyFile` (string, optional).
- Arg 3: `throwOnError` (boolean, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `inputString` (string). passed by value.
- Arg 2: `PolicyFile` (string). passed by value.
- Arg 3: `throwOnError` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_getsafehtml(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

OWASP AntiSamy-style HTML sanitizer (HTML parser + policy engine). No HTML parsing exists. Unit tests + tracker updates.

## Ease of implementation

Hard. A full HTML parser and policy engine is a large subsystem.
