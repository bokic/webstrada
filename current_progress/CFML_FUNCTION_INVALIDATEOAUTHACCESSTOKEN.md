# Research: INVALIDATEOAUTHACCESSTOKEN cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_invalidateoauthaccesstoken()` in `src/cf8.cpp:14040` throws `"Function INVALIDATEOAUTHACCESSTOKEN is not implemented"`.
- Compiler: `INVALIDATEOAUTHACCESSTOKEN` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5467`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14040` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5467` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*invalidateoauthaccesstoken*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:523` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_INVALIDATEOAUTHACCESSTOKEN.md` |

## What INVALIDATEOAUTHACCESSTOKEN does at the low C level

Invalidate a cached OAuth access token for `type` (e.g. the native-app flow tokens cached by CF).

- Arg 1: `token` (string, required).
- Arg 2: `type` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `token` (string). passed by value.
- Arg 2: `type` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_invalidateoauthaccesstoken(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

OAuth token cache + provider config. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires OAuth client infrastructure.
