# Research: ISSAFEHTML cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime implementation: `cfml::cf_issafehtml()` in `src/cffunctions/fn_issafehtml.cpp`.
- Compiler: `ISSAFEHTML` JIT compilation in `src/codegen/codegen_expr.cpp`.
- Interpreter: `ISSAFEHTML` dispatch in `src/core/core_interp.cpp` and `src/core/core_membermethods.cpp`.
- String member method support: `str.isSafeHTML()` in `src/core/core_membermethods.cpp`.
- Symbol registered via `llvm::sys::DynamicLibrary::AddSymbol` in `src/codegen/llvm_compiler.cpp`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/fn_issafehtml.cpp` |
| Compiler wiring | ✅ JIT compiled | `src/codegen/codegen_expr.cpp`, symbol at `src/codegen/llvm_compiler.cpp` |
| Interpreter dispatch | ✅ Implemented | `src/core/core_interp.cpp`, `src/core/core_membermethods.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified | `tests/cfm/issafehtml_test.cfm` (100% match against CF 2025) |
| Tracker status | ✅ Updated | `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISSAFEHTML.md` |

## What ISSAFEHTML does at the low C level

Returns true if the given HTML is safe (free of malicious content) after sanitization checks.

- Arg 1: `value` (any).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `value` (any). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_issafehtml(const cfvariant *a1);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `1` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

XML parsing; SAML logout-response document validation; HTTPS signing.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
