# Research: SETENCODING cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime: `cfml::cf_setencoding(const cfvariant *scope, const cfvariant *encoding)` implemented in `src/cffunctions/fn_setencoding.cpp`.
- Helper: `encoding_reset()`, `set_form_encoding()`, `set_url_encoding()` in `src/cffunctions/common.cpp`.
- Compiler: JIT compiled direct call via `codegen_expr.cpp`.
- Interpreter: `core_interp.cpp` and `core_membermethods.cpp` support.
- Symbol registered in `llvm_compiler.cpp`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/fn_setencoding.cpp` |
| Compiler wiring | ✅ JIT compiled direct call | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ Implemented | `src/core/core_interp.cpp`, `src/core/core_membermethods.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified against CF 2025 (`tests/cfm/tier1_setencoding_test.cfm` + `JitExpressionTest.Tier1SetEncoding`) | `tests/cfm/tier1_setencoding_test.cfm`, `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SETENCODING.md` |

## What SETENCODING does at the low C level

Set the character encoding used for the given `scope` (`url`, `form`, or `request`) — e.g. `UTF-8`. Affects how request/form/url data is decoded.

- Arg 1: `encoding` (string, required).
- Arg 2: `scope` (string, optional, default url).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `encoding` (string). passed by value.
- Arg 2: `scope` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_setencoding(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Request-decoding layer with per-scope charset handling (form/url data is currently decoded with a fixed charset). Unit tests + tracker updates.

## Ease of implementation

Moderate. Per-scope charset state in the request context + decode pass.
