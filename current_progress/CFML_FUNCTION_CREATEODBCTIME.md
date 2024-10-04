# Research: CREATEODBCTIME cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`
- Implemented: `2026-08-08` (Tier 1; see `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests)

## Current state

> Implemented (2026-08-08): `cf_createodbctime` in `src/cffunctions/`, a direct JIT call in
> `src/codegen/codegen_expr.cpp` plus `cfvariant_call_function` / `evaluateExpr` dispatch,
> byte-verified against CF 2025 in `tests/cfm/tier1_*.cfm` and `JitExpressionTest.Tier1*`.

- Runtime stub: `cfml::cf_createodbctime()` in `src/cf8.cpp:12005` throws `"Function CREATEODBCTIME is not implemented"`.
- Compiler: `CREATEODBCTIME` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5278`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/` |
| Compiler wiring | ✅ Native JIT handler | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ `evaluateExpr` + `cfvariant_call_function` | `src/core/` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CREATEODBCTIME.md` |

## What CREATEODBCTIME does at the low C level

Create an ODBC `time` object from a time value.

- Arg 1: `date` (date, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `date` (date). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_createodbctime(const cfvariant *a1);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

An ODBC time type in the variant model. Unit tests + tracker updates.

## Ease of implementation

Easy-to-moderate. A time-variant wrapper.
