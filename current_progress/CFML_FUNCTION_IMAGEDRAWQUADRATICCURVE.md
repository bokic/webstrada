# Research: IMAGEDRAWQUADRATICCURVE cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagedrawquadraticcurve()` in `src/cf8.cpp:13834` throws `"Function IMAGEDRAWQUADRATICCURVE is not implemented"`.
- Compiler: `IMAGEDRAWQUADRATICCURVE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1738`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5421`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13834` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1738`, symbol at `src/compiler.cpp:5421` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagedrawquadraticcurve*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:475` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGEDRAWQUADRATICCURVE.md` |

## What IMAGEDRAWQUADRATICCURVE does at the low C level

Draw a quadratic Bezier curve from the current position to (x,y) with one control point.

- Arg 1: `image` (any, required).
- Arg 2: `controlx` (numeric, required).
- Arg 3: `controly` (numeric, required).
- Arg 4: `x` (numeric, required).
- Arg 5: `y` (numeric, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `controlx` (numeric). passed by value.
- Arg 3: `controly` (numeric). passed by value.
- Arg 4: `x` (numeric). passed by value.
- Arg 5: `y` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagedrawquadraticcurve(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + Bezier rasterization. Unit tests + tracker updates.

## Ease of implementation

Moderate with an image library.
