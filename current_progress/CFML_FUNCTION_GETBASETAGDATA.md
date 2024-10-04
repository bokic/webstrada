# Research: GETBASETAGDATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_getbasetagdata()` in `src/cf8.cpp:13475` throws `"Function GETBASETAGDATA is not implemented"`.
- Compiler: `GETBASETAGDATA` is in the zero-arg not-implemented function list (`src/compiler.cpp:1736`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5357`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13475` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1736`, symbol at `src/compiler.cpp:5357` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*getbasetagdata*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:401` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
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
