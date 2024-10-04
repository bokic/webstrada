# Research: ENCODEFORXPATH cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_encodeforxpath()` in `src/cf8.cpp:12749` throws `"Function ENCODEFORXPATH is not implemented"`.
- Compiler: `ENCODEFORXPATH` is in the zero-arg not-implemented function list (`src/compiler.cpp:1734`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5319`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12749` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1734`, symbol at `src/compiler.cpp:5319` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*encodeforxpath*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:353` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Security/Encode) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ENCODEFORXPATH.md` |

## What ENCODEFORXPATH does at the low C level

Encode `string` for safe inclusion in XPath expressions per OWASP ESAPI (escapes XPath-special characters with `\xHH` hex escapes).

- Arg 1: `string` (string, required).
- Arg 2: `canonicalize` (boolean, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `string` (string). passed by value.
- Arg 2: `canonicalize` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_encodeforxpath(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

ESAPI encoder family infra. Unit tests + tracker updates.

## Ease of implementation

Moderate. XPath escape table; shares infra with family.
