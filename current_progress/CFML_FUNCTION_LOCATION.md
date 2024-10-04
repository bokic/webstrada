# Research: LOCATION cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`
- Implemented: `2026-08-08` (Tier 1; see `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests)

## Current state

> Implemented (2026-08-08): `cf_location` in `src/cffunctions/`, a direct JIT call in
> `src/codegen/codegen_expr.cpp` plus `cfvariant_call_function` / `evaluateExpr` dispatch,
> byte-verified against CF 2025 in `tests/cfm/tier1_*.cfm` and `JitExpressionTest.Tier1*`.

- Runtime stub: `cfml::cf_location()` in `src/cf8.cpp:14701` throws `"Function LOCATION is not implemented"`.
- Compiler: `LOCATION` is in the zero-arg not-implemented function list (`src/compiler.cpp:1741`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5534`.

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
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LOCATION.md` |

## What LOCATION does at the low C level

Redirect the client to `url`. `addtoken=true` appends the session token (`CFID`/`CFTOKEN` or JSESSIONID) to the URL; `statuscode` sets the HTTP redirect code. Throws a redirect exception that the request loop converts to a `Location:` header.

- Arg 1: `url` (string, required).
- Arg 2: `addtoken` (boolean, optional, default true).
- Arg 3: `statuscode` (numeric, optional, default 302).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `url` (string). passed by value.
- Arg 2: `addtoken` (boolean). passed by value.
- Arg 3: `statuscode` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_location(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Request-loop redirect exception (like `abort_exception`) + session-token plumbing. Unit tests + tracker updates.

## Ease of implementation

Easy-to-moderate. Throw a redirect exception carrying URL/status; the HTTP layer writes the header.
