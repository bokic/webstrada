# Research: IMAGESETDRAWINGSTROKE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagesetdrawingstroke()` in `src/cf8.cpp:13958` throws `"Function IMAGESETDRAWINGSTROKE is not implemented"`.
- Compiler: `IMAGESETDRAWINGSTROKE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5452`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13958` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5452` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagesetdrawingstroke*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:506` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGESETDRAWINGSTROKE.md` |

## What IMAGESETDRAWINGSTROKE does at the low C level

Set the stroke attributes (`width`, `lineJoin`, `miterLimit`, `dashArray`, `dashPhase`, `cap`).

- Arg 1: `image` (any, required).
- Arg 2: `attributes` (struct, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `attributes` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagesetdrawingstroke(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + stroke state. Unit tests + tracker updates.

## Ease of implementation

Moderate. Stroke plumbing into rasterizer.
