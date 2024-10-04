# Research: PROCESSSAMLLOGOUTREQUEST cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_processsamllogoutrequest()` in `src/cf8.cpp:14919` throws `"Function PROCESSSAMLLOGOUTREQUEST is not implemented"`.
- Compiler: `PROCESSSAMLLOGOUTREQUEST` is in the zero-arg not-implemented function list (`src/compiler.cpp:1743`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5574`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14919` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1743`, symbol at `src/compiler.cpp:5574` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*processsamllogoutrequest*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:662` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_PROCESSSAMLLOGOUTREQUEST.md` |

## What PROCESSSAMLLOGOUTREQUEST does at the low C level

Process an incoming SAML logout request (base64-encoded) received in the request data and perform the logout against the SP session.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_processsamllogoutrequest();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

SAML message parsing + session management. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires SAML parsing and session/single-signout infrastructure.
