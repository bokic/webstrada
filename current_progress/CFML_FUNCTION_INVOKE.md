# Research: INVOKE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_invoke()` in `src/cf8.cpp:14044` throws `"Function INVOKE is not implemented"`.
- Compiler: `INVOKE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5468`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14044` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5468` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*invoke*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:524` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_INVOKE.md` |

## What INVOKE does at the low C level

Invokes a method of a component or object with an argument struct; used for dynamic method calls.

- Arg 1: `component` (string).
- Arg 2: `method` (string).
- Arg 3: `argStruct` (struct).
- Arg 4: `returnType` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `component` (string). passed by value.
- Arg 2: `method` (string). passed by value.
- Arg 3: `argStruct` (struct). passed by value.
- Arg 4: `returnType` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_invoke(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `4` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Component/method invocation support; object serialization (Java-serialization-compatible); client stub generation.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
