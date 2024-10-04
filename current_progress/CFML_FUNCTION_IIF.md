# Research: IIF cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_iif()` in `src/cf8.cpp:13778` throws `"Function IIF is not implemented"`.
- Compiler: `IIF` is in the zero-arg not-implemented function list (`src/compiler.cpp:1737`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5407`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13778` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1737`, symbol at `src/compiler.cpp:5407` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*iif*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:461` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Dynamic eval) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IIF.md` |

## What IIF does at the low C level

Evaluate `condition`; if true return the result of `Evaluate(expression1)`, otherwise `Evaluate(expression2)`. Both expressions are only evaluated lazily (the unchosen branch is NOT evaluated — the critical CF semantic difference from a plain ternary).

- Arg 1: `condition` (boolean, required).
- Arg 2: `expression1` (string, required).
- Arg 3: `expression2` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `condition` (boolean). passed by value.
- Arg 2: `expression1` (string). passed by value.
- Arg 3: `expression2` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_iif(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Runtime expression evaluation, same as `Evaluate`. Unit tests + tracker updates.

## Ease of implementation

Hard. Same dynamic-eval infra as `Evaluate`.
