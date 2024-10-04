# Research: SETENCODING cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_setencoding()` in `src/cf8.cpp:15767` throws `"Function SETENCODING is not implemented"`.
- Compiler: `SETENCODING` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5616`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15767` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5616` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*setencoding*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:714` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
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
