# Research: SESSIONINVALIDATE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_sessioninvalidate()` in `src/cf8.cpp:15745` throws `"Function SESSIONINVALIDATE is not implemented"`.
- Compiler: `SESSIONINVALIDATE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5613`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15745` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5613` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*sessioninvalidate*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:711` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SESSIONINVALIDATE.md` |

## What SESSIONINVALIDATE does at the low C level

Invalidate (clear) the current user's session scope for the rest of the request (the session data is discarded and a new session is started on the next request).

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_sessioninvalidate();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Session-scope clear + invalidation flag. Unit tests + tracker updates.

## Ease of implementation

Easy-to-moderate. Clear the session map and set an invalidated flag.
