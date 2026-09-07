# Research: GETPAGECONTEXT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime: `cfml::cf_getpagecontext()` implemented in `src/cffunctions/fn_getpagecontext.cpp`. Throws `Function GetPageContext is not supported: it returns a Java page context object.` per AGENTS.md rule (engine has no Java object interop; matches `CacheGetSession`).
- Compiler: JIT compiled direct call via `codegen_expr.cpp`.
- Interpreter: `core_interp.cpp` and `core_membermethods.cpp` support.
- Symbol registered in `llvm_compiler.cpp`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented (throws unsupported error) | `src/cffunctions/fn_getpagecontext.cpp` |
| Compiler wiring | ✅ JIT compiled direct call | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ Implemented | `src/core/core_interp.cpp`, `src/core/core_membermethods.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified via unit tests (`JitExpressionTest.Tier1GetPageContext`) | `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETPAGECONTEXT.md` |

## What GETPAGECONTEXT does at the low C level

Return the underlying PageContext object (Java) for the current request — a low-level CF internal escape hatch. Rarely used in real code.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_getpagecontext();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Requires a Java interop layer and a page-context object model — neither exists. Unit tests + tracker updates.

## Ease of implementation

Hard. Java object model needed; likely reasonable to leave unimplemented (stub throwing) in a native runtime.
