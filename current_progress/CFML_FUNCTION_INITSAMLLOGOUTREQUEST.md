# Research: INITSAMLLOGOUTREQUEST cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_initsamllogoutrequest()` in `src/cf8.cpp:14002` throws `"Function INITSAMLLOGOUTREQUEST is not implemented"`.
- Compiler: `INITSAMLLOGOUTREQUEST` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5463`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14002` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5463` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*initsamllogoutrequest*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:518` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_INITSAMLLOGOUTREQUEST.md` |

## What INITSAMLLOGOUTREQUEST does at the low C level

Initialize the SAML logout request used by `GetSAMLLogoutRequest`.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_initsamllogoutrequest();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

SAML logout state + builder. Unit tests + tracker updates.

## Ease of implementation

Hard. Same SAML infrastructure.
