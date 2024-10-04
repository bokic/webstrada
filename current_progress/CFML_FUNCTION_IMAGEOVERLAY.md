# Research: IMAGEOVERLAY cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imageoverlay()` in `src/cf8.cpp:13914` throws `"Function IMAGEOVERLAY is not implemented"`.
- Compiler: `IMAGEOVERLAY` is in the zero-arg not-implemented function list (`src/compiler.cpp:1738`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5441`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13914` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1738`, symbol at `src/compiler.cpp:5441` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imageoverlay*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:495` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGEOVERLAY.md` |

## What IMAGEOVERLAY does at the low C level

Composite `topImage` onto `baseImage` at (x,y) with `opacity` (default 50) and `composite` mode (`over`/`in`/`out`/`xor`/`multiply`/...). Returns the base image.

- Arg 1: `baseImage` (any, required).
- Arg 2: `topImage` (any, required).
- Arg 3: `x` (numeric, optional).
- Arg 4: `y` (numeric, optional).
- Arg 5: `opacity` (numeric, optional).
- Arg 6: `composite` (string, optional, default over).
- Arg 7: `crop` (boolean, optional, default false).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `baseImage` (any). passed by value.
- Arg 2: `topImage` (any). passed by value.
- Arg 3: `x` (numeric). passed by value.
- Arg 4: `y` (numeric). passed by value.
- Arg 5: `opacity` (numeric). passed by value.
- Arg 6: `composite` (string). passed by value.
- Arg 7: `crop` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imageoverlay(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4, const cfvariant *a5, const cfvariant *a6, const cfvariant *a7);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + alpha compositing. Unit tests + tracker updates.

## Ease of implementation

Moderate-to-hard with alpha-channel compositing.
