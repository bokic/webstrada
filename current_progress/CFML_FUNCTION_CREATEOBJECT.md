# Research: CREATEOBJECT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_createobject()` in `src/cf8.cpp:11993` throws `"Function CREATEOBJECT is not implemented"`.
- Compiler: `CREATEOBJECT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5275`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11993` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5275` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*createobject*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:298` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CREATEOBJECT.md` |

## What CREATEOBJECT does at the low C level

Instantiate an object by `type`: `java`, `component`, `webservice`, `object`, or OSGi `bundle`. For a CFC this is `component` (with `componentName` in CF but `urltowsdl` here). Returns the object instance (for components, the CFC instance).

- Arg 1: `type` (string, required).
- Arg 2: `urltowsdl` (string, required).
- Arg 3: `portname` (string, optional).
- Arg 4: `bundleName` (string, optional).
- Arg 5: `bundleVersion` (string, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `type` (string). passed by value.
- Arg 2: `urltowsdl` (string). passed by value.
- Arg 3: `portname` (string). passed by value.
- Arg 4: `bundleName` (string). passed by value.
- Arg 5: `bundleVersion` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_createobject(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

A minimal object model: CFC instantiation currently does not exist (no component/CFC support), Java object interop does not exist. Unit tests + tracker updates.

## Ease of implementation

Hard. CFC/Java object support is a large missing subsystem; at minimum `component` requires the component loader + instance creation.
