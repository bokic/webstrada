# Research: CSVWRITE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_csvwrite()` in `src/cf8.cpp:12209` throws `"Function CSVWRITE is not implemented"`.
- Compiler: `CSVWRITE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5285`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12209` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5285` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*csvwrite*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:310` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CSVWRITE.md` |

## What CSVWRITE does at the low C level

Serialize `data` (an array of arrays, or array of structs, or a query) into a CSV string with the given `delimiter`. Quoting rules: fields containing the delimiter, quotes, newlines are quoted, and embedded quotes doubled.

- Arg 1: `data` (array, required).
- Arg 2: `delimiter` (string, optional, default ,).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `data` (array). passed by value.
- Arg 2: `delimiter` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_csvwrite(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

CSV serializer mirroring the parser's quoting rules. Unit tests + tracker updates.

## Ease of implementation

Moderate. Straightforward string builder; quoting edge cases (embedded quotes/delimiters/CRLF) need tests.
