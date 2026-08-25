# Research: GETBASETAGDATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime: `cfml::cf_getbasetagdata()` in `src/core/core_misc.cpp` — implemented.
- Compiler: `GETBASETAGDATA` compiled as a direct JIT call into `cf_getbasetagdata`.
- Interpreter (`evalFunction`) dispatch entry present.
- On 2026-08-25 the returned struct was fixed to mirror CF's `PageScope`: the base tag's variables are merged at the top level (so `data.<var>` resolves, e.g. `data.currentPage`), under the THISTAG / ATTRIBUTES / CALLER / VARIABLES scope keys. This fixed the Mango Blog "Element CURRENTPAGE is undefined in DATA." error. See PROGRESS.md.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/core/core_misc.cpp` `cf_getbasetagdata()` |
| Compiler wiring | ✅ Direct JIT call | `src/codegen/llvm_compiler.cpp` |
| Interpreter dispatch | ✅ Present | `src/core/core_interp.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/custom_tag_getbasetagdata_test.cfm` (byte-verified vs CF 2025) + `ComponentTest.GetBaseTagDataResolvesTagVariablesAtTopLevel` + `ComponentTest.GetBaseTagDataScopesStillExposed` | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETBASETAGDATA.md` |

## What GETBASETAGDATA does at the low C level

Return metadata about an enclosing `tagname` in the tag stack (a struct with `name`, `thisTag`, `parent`, `hastype` in older CFML; for `cftag`/`cffunction` contexts). `level` skips frames.

- Arg 1: `tagname` (string, required).
- Arg 2: `level` (numeric, optional, default 1).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `tagname` (string). passed by value.
- Arg 2: `level` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_getbasetagdata(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Tag-stack/metadata tracking does not exist; functions compile to JIT calls with no enclosing-tag frame info. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires a template/tag invocation-stack with per-frame tag metadata.
