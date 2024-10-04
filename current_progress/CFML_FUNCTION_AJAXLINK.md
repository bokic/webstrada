# Research: AJAXLINK cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_ajaxlink()` in `src/cf8.cpp:11023` throws `"Function AJAXLINK is not implemented"`.
- Compiler: `AJAXLINK` is in the zero-arg not-implemented function list (`src/compiler.cpp:1732`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5237`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11023` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1732`, symbol at `src/compiler.cpp:5237` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*ajaxlink*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:217` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_AJAXLINK.md` |

## What AJAXLINK does at the low C level

Generates JavaScript/AJAX link code that calls the given function on the client; used with cfajaximport.

- Arg 1: `href` (string).
- Arg 2: `method` (string).
- Arg 3: `key` (string).
- Arg 4: `form` (string).
- Arg 5: `bind` (string).
- Arg 6: `onSuccess` (string).
- Arg 7: `onError` (string).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `href` (string). passed by value.
- Arg 2: `method` (string). passed by value.
- Arg 3: `key` (string). passed by value.
- Arg 4: `form` (string). passed by value.
- Arg 5: `bind` (string). passed by value.
- Arg 6: `onSuccess` (string). passed by value.
- Arg 7: `onError` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_ajaxlink(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `7` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Component/method invocation support; object serialization (Java-serialization-compatible); client stub generation.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.
