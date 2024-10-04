# Research: ParameterExists cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`
- Implemented: `2026-08-08` (Tier 1; see `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests)

## Current state

> Implemented (2026-08-08): `cf_parameterexists` in `src/cffunctions/`, a direct JIT call in
> `src/codegen/codegen_expr.cpp` plus `cfvariant_call_function` / `evaluateExpr` dispatch,
> byte-verified against CF 2025 in `tests/cfm/tier1_*.cfm` and `JitExpressionTest.Tier1*`.

- Runtime stub: `cfml::cf_parameterexists()` in `src/cf8.cpp:14894` throws `"Function ParameterExists is not implemented"`.
- Compiler: `PARAMETEREXISTS` is in the zero-arg not-implemented function list (`src/compiler.cpp:1743`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5570`.

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
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_PARAMETEREXISTS.md` |

## What ParameterExists does at the low C level

Per Adobe CF: determine whether a parameter (variable reference) exists. Key distinction: ColdFusion does **not** evaluate the argument — a bare name like `foo` is checked as a variable, not as an expression. Returns boolean. Deprecated in modern CF in favor of `IsDefined`.

- Arg 1: `parameter` (a variable reference, e.g. `variables.foo`).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `parameter` (a variable reference, e.g. `variables.foo`). Special case: the argument is an **unevaluated variable name** (ColdFusion does not resolve it before the call) — the name is read as text, so passing semantics for its value do not apply; the referenced variable is never modified.

## Proposed compiled form

```cpp
cfvariant *cf_parameterexists(const cfvariant *parameter);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list. The compiler must pass the argument as an unevaluated name (like `IsDefined`/`cfparam`), not as a resolved value.

## Dependency

- **Compiler support for passing an unevaluated name** — mirrors the `IsDefined` mechanism (also unimplemented).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

## Ease of implementation

Easy-to-moderate. Trivial lookup once the compiler can pass the bare variable name unevaluated.
