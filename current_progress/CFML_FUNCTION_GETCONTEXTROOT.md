# Research: GETCONTEXTROOT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`
- Implemented: `2026-08-08` (Tier 1; see `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests)

## Current state

> Implemented (2026-08-08): `cf_getcontextroot` in `src/cffunctions/`, a direct JIT call in
> `src/codegen/codegen_expr.cpp` plus `cfvariant_call_function` / `evaluateExpr` dispatch,
> byte-verified against CF 2025 in `tests/cfm/tier1_*.cfm` and `JitExpressionTest.Tier1*`.

- Runtime stub: `cfml::cf_getcontextroot()` in `src/cf8.cpp:13491` throws `"Function GETCONTEXTROOT is not implemented"`.
- Compiler: `GETCONTEXTROOT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1736`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5361`.

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
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETCONTEXTROOT.md` |

## What GETCONTEXTROOT does at the low C level

Return the web-application context root (the path of the web root). In CF this reflects the deployed web root.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_getcontextroot();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

A notion of web-app root must be plumbed into the runtime (CLI runner has the template dir). Unit tests + tracker updates.

## Ease of implementation

Easy-to-moderate. Config value; needs the web-root concept in the runtime context.
