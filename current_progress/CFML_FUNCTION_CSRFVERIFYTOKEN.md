# Research: CSRFVERIFYTOKEN cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_csrfverifytoken()` in `src/cf8.cpp:12197` throws `"Function CSRFVERIFYTOKEN is not implemented"`.
- Compiler: `CSRFVERIFYTOKEN` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5282`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12197` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5282` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*csrfverifytoken*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:307` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CSRFVERIFYTOKEN.md` |

## What CSRFVERIFYTOKEN does at the low C level

Check `token` against the stored CSRF token for this session/`key`. Returns true when they match, false otherwise.

- Arg 1: `token` (string, required).
- Arg 2: `key` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `token` (string). passed by value.
- Arg 2: `key` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_csrfverifytoken(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Same session-token store as `CSRFGenerateToken`. Unit tests + tracker updates.

## Ease of implementation

Easy once `CSRFGenerateToken` exists — a comparison against the stored token.
