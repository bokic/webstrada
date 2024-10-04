# Research: IMAGERESIZE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imageresize()` in `src/cf8.cpp:13930` throws `"Function IMAGERESIZE is not implemented"`.
- Compiler: `IMAGERESIZE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5445`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13930` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5445` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imageresize*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:499` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGERESIZE.md` |

## What IMAGERESIZE does at the low C level

Resize the image to `width`x`height` (may scale), using `interpolation` (`nearest`, `bilinear`, `bicubic`, ...) and an optional `blurFactor`. In-place.

- Arg 1: `image` (any, required).
- Arg 2: `width` (numeric, required).
- Arg 3: `height` (numeric, required).
- Arg 4: `interpolation` (string, optional).
- Arg 5: `blurFactor` (numeric, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `width` (numeric). passed by value.
- Arg 3: `height` (numeric). passed by value.
- Arg 4: `interpolation` (string). passed by value.
- Arg 5: `blurFactor` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imageresize(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + scaling/interpolation. Unit tests + tracker updates.

## Ease of implementation

Moderate with an image library.
