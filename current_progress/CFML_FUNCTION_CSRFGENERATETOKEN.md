# Research: CSRFGENERATETOKEN cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_csrfgeneratetoken()` in `src/cf8.cpp:12193` throws `"Function CSRFGENERATETOKEN is not implemented"`.
- Compiler: `CSRFGENERATETOKEN` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5281`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12193` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5281` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*csrfgeneratetoken*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:306` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CSRFGENERATETOKEN.md` |

## What CSRFGENERATETOKEN does at the low C level

Generate a random CSRF protection token tied to the current session (and optional `key`), storing it in the session scope. `forceNew=true` invalidates any existing token first. Returns the token string.

- Arg 1: `key` (string, optional).
- Arg 2: `forceNew` (boolean, optional, default false).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `key` (string). passed by value.
- Arg 2: `forceNew` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_csrfgeneratetoken(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Needs a secure random generator (session-scope storage already exists). Session must be enabled for tokens to persist. Unit tests + tracker updates.

## Ease of implementation

Moderate. Random token generation + session storage; the token lifecycle (`forceNew`, per-key tokens) needs CF-exact semantics.
