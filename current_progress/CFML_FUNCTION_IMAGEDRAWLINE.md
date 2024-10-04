# Research: IMAGEDRAWLINE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagedrawline()` in `src/cf8.cpp:13818` throws `"Function IMAGEDRAWLINE is not implemented"`.
- Compiler: `IMAGEDRAWLINE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1738`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5417`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13818` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1738`, symbol at `src/compiler.cpp:5417` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagedrawline*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:471` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGEDRAWLINE.md` |

## What IMAGEDRAWLINE does at the low C level

Draw a line from (x1,y1) to (x2,y2) in the current drawing color (Bresenham).

- Arg 1: `image` (any, required).
- Arg 2: `x1` (numeric, required).
- Arg 3: `y1` (numeric, required).
- Arg 4: `x2` (numeric, required).
- Arg 5: `y2` (numeric, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `x1` (numeric). passed by value.
- Arg 3: `y1` (numeric). passed by value.
- Arg 4: `x2` (numeric). passed by value.
- Arg 5: `y2` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagedrawline(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + line rasterization. Unit tests + tracker updates.

## Ease of implementation

Easy with an image buffer.
