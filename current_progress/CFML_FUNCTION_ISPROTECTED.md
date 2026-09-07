# Research: ISPROTECTED cffunction implementation notes

## Current state

- Runtime implementation: `cfml::cf_isprotected()` in `src/cffunctions/fn_isprotected.cpp` throws `"Variable ISPROTECTED is undefined."` (byte-identical to Adobe ColdFusion 2025).
- Removed in ColdFusion MX with the removal of pre-MX Advanced Security.
- In Adobe ColdFusion 2025, calling `isProtected()` throws `Variable ISPROTECTED is undefined.` and defining a UDF with this name is allowed.
- JIT compiler resolves unqualified calls through `cfvariant_call_function`, allowing UDF definitions to take precedence and falling through to `Variable ISPROTECTED is undefined.` when undefined.
- Symbol registered via `AddSymbol` in `src/codegen/llvm_compiler.cpp`.

## Implemented: 100% (Not a CF 2025 function; byte-identical error reproduced)

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Reproduces CF 2025 error | `src/cffunctions/fn_isprotected.cpp` |
| Compiler wiring | ✅ Calls through `cfvariant_call_function` allowing UDFs | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ Throws variable-undefined error | `src/core/core_interp.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified against CF 2025 | `tests/cfm/is_protected_authenticated_authorized.cfm`, `JitExpressionTest.Tier2IsProtected` |
| Tracker status | ✅ Updated | `PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md` |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISPROTECTED.md` |
