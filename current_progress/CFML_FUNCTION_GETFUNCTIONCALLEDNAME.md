# Research: GETFUNCTIONCALLEDNAME cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime implementation: `cfml::cf_getfunctioncalledname()` in `src/cffunctions/fn_getfunctioncalledname.cpp`.
- Compiler: JIT emits direct call to `cf_getfunctioncalledname()` via `src/codegen/codegen_expr.cpp`.
- Interpreter: `evalFunction` in `src/core/core_interp.cpp` verifies 0 args and calls `cf_getfunctioncalledname()`.
- Symbol registered in `kBuiltinFunctionNames` in `src/core/core_udf.cpp` and `AddSymbol` in `src/codegen/llvm_compiler.cpp`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/fn_getfunctioncalledname.cpp` |
| Compiler wiring | ✅ Direct JIT call (0 args) | `src/codegen/codegen_expr.cpp`, `src/codegen/llvm_compiler.cpp` |
| Interpreter dispatch | ✅ Implemented | `src/core/core_interp.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Comprehensive CFM + GTest coverage | `tests/cfm/getfunctioncalledname.cfm`, `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Supported | `cfml_docs/CFML_FUNCTION_GETFUNCTIONCALLEDNAME.md` |

## What GETFUNCTIONCALLEDNAME does at the low C level

Returns the name of the currently executing function (the enclosing cffunction).

- No arguments (zero-arg function).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- No arguments to pass.

## Proposed compiled form

```cpp
cfvariant *cf_getfunctioncalledname();
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `0` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Function call stack / current invocation metadata (enclosing cffunction name).

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
