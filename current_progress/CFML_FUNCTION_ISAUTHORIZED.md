# Research: ISAUTHORIZED cffunction implementation notes

## Current state

- Runtime implementation: `cfml::cf_isauthorized()` in `src/cffunctions/fn_isauthorized.cpp` throws `"Variable ISAUTHORIZED is undefined."` (byte-identical to Adobe ColdFusion 2025).
- Removed in ColdFusion MX with the removal of pre-MX Advanced Security.
- In Adobe ColdFusion 2025, calling `isAuthorized()` throws `Variable ISAUTHORIZED is undefined.` and defining a UDF with this name is allowed.
- JIT compiler resolves unqualified calls through `cfvariant_call_function`, allowing UDF definitions to take precedence and falling through to `Variable ISAUTHORIZED is undefined.` when undefined.
- Symbol registered via `AddSymbol` in `src/codegen/llvm_compiler.cpp`.

## Implemented: 100% (Not a CF 2025 function; byte-identical error reproduced)

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Reproduces CF 2025 error | `src/cffunctions/fn_isauthorized.cpp` |
| Compiler wiring | ✅ Calls through `cfvariant_call_function` allowing UDFs | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ Throws variable-undefined error | `src/core/core_interp.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified against CF 2025 | `tests/cfm/is_protected_authenticated_authorized.cfm`, `JitExpressionTest.Tier2IsAuthorized` |
| Tracker status | ✅ Updated | `PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md` |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISAUTHORIZED.md` |
