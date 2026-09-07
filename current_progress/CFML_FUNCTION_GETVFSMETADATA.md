# Research: GETVFSMETADATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime: `cfml::cf_getvfsmetadata(const cfvariant *fileSystemType)` implemented in `src/cffunctions/fn_getvfsmetadata.cpp`.
- Compiler: JIT compiled direct call via `codegen_expr.cpp`.
- Interpreter: `core_interp.cpp` and `core_membermethods.cpp` support.
- Symbol registered in `llvm_compiler.cpp`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/fn_getvfsmetadata.cpp` |
| Compiler wiring | ✅ JIT compiled direct call | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ Implemented | `src/core/core_interp.cpp`, `src/core/core_membermethods.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ Verified against CF 2025 (`tests/cfm/tier1_getvfsmetadata_test.cfm` + `JitExpressionTest.Tier1GetVFSMetaData`) | `tests/cfm/tier1_getvfsmetadata_test.cfm`, `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETVFSMETADATA.md` |

## What GETVFSMETADATA does at the low C level

Return a struct of metadata about the virtual file system (available RAM VFS storage, size, etc.) — mostly relevant to the bundled virtual filesystem.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_getvfsmetadata();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

VFS subsystem does not exist. Unit tests + tracker updates.

## Ease of implementation

Hard (or unimplementable in a native runtime — likely a stub).
