# Research: IMAGESHEARDRAWINGAXIS cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagesheardrawingaxis()` in `src/cf8.cpp:13974` throws `"Function IMAGESHEARDRAWINGAXIS is not implemented"`.
- Compiler: `IMAGESHEARDRAWINGAXIS` is in the zero-arg not-implemented function list (`src/compiler.cpp:1739`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5456`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13974` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1739`, symbol at `src/compiler.cpp:5456` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagesheardrawingaxis*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:510` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGESHEARDRAWINGAXIS.md` |

## What IMAGESHEARDRAWINGAXIS does at the low C level

Set the drawing-axis shear by `sx`/`sy` and `angle` for subsequent drawing.

- Arg 1: `image` (any, required).
- Arg 2: `sx` (numeric, required).
- Arg 3: `sy` (numeric, required).
- Arg 4: `angle` (numeric, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `sx` (numeric). passed by value.
- Arg 3: `sy` (numeric). passed by value.
- Arg 4: `angle` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagesheardrawingaxis(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + transform state. Unit tests + tracker updates.

## Ease of implementation

Moderate. Transform-state plumbing.
