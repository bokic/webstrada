# Research: ISVALID cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_isvalid()` in `src/cf8.cpp:14309` throws `"Function ISVALID is not implemented"`.
- Compiler: `ISVALID` is in the zero-arg not-implemented function list (`src/compiler.cpp:1741`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5512`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14309` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1741`, symbol at `src/compiler.cpp:5512` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*isvalid*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:570` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISVALID.md` |

## What ISVALID does at the low C level

Validate `value` against `type` (`range`, `integer`, `numeric`, `telephone`, `zipcode`, `email`, `creditcard`, `ssn`, `time`, `boolean`, `guid`, `eurodate`, `regex`, `url`, `uuid`, `date`, `usdate`, `variablename`). Returns true/false; `range` uses `min`/`max`, `regex` uses `pattern`.

- Arg 1: `type` (string, required).
- Arg 2: `value` (any, required).
- Arg 3: `min` (numeric, optional).
- Arg 4: `max` (numeric, optional).
- Arg 5: `pattern` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `type` (string). passed by value.
- Arg 2: `value` (any). passed by value.
- Arg 3: `min` (numeric). passed by value.
- Arg 4: `max` (numeric). passed by value.
- Arg 5: `pattern` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_isvalid(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

A battery of format validators (regex for email/uuid/url/creditcard, date/number parsing). No regex engine exists yet. Unit tests + tracker updates.

## Ease of implementation

Moderate-to-hard. Each validator is small; `email`/`creditcard`/`url`/`regex` need a regex engine that does not yet exist.
