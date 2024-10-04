# Research: IMAGEDRAWCUBICCURVE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagedrawcubiccurve()` in `src/cf8.cpp:13814` throws `"Function IMAGEDRAWCUBICCURVE is not implemented"`.
- Compiler: `IMAGEDRAWCUBICCURVE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1738`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5416`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13814` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1738`, symbol at `src/compiler.cpp:5416` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagedrawcubiccurve*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:470` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGEDRAWCUBICCURVE.md` |

## What IMAGEDRAWCUBICCURVE does at the low C level

Draw a cubic Bezier curve from the current drawing position to (x,y) using two control points.

- Arg 1: `image` (any, required).
- Arg 2: `control1x` (numeric, required).
- Arg 3: `control1y` (numeric, required).
- Arg 4: `control2x` (numeric, required).
- Arg 5: `control2y` (numeric, required).
- Arg 6: `x` (numeric, required).
- Arg 7: `y` (numeric, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `control1x` (numeric). passed by value.
- Arg 3: `control1y` (numeric). passed by value.
- Arg 4: `control2x` (numeric). passed by value.
- Arg 5: `control2y` (numeric). passed by value.
- Arg 6: `x` (numeric). passed by value.
- Arg 7: `y` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagedrawcubiccurve(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + Bezier rasterization. Unit tests + tracker updates.

## Ease of implementation

Moderate with an image library.
