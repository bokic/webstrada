# Research: GETCOMPONENTMETADATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_getcomponentmetadata()` in `src/cf8.cpp:13487` throws `"Function GETCOMPONENTMETADATA is not implemented"`.
- Compiler: `GETCOMPONENTMETADATA` is in the zero-arg not-implemented function list (`src/compiler.cpp:1736`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5360`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13487` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1736`, symbol at `src/compiler.cpp:5360` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*getcomponentmetadata*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:405` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETCOMPONENTMETADATA.md` |

## What GETCOMPONENTMETADATA does at the low C level

Return a struct of component metadata (name, path, properties, functions, inheritance, etc.) for the CFC at `path`. Throws if the component cannot be found.

- Arg 1: `path` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `path` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_getcomponentmetadata(const cfvariant *a1);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Component/CFC subsystem does not exist. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires CFC parsing and a metadata model.
