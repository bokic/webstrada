# Research: IMAGECREATECAPTCHA cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_imagecreatecaptcha()` in `src/cf8.cpp:13798` throws `"Function IMAGECREATECAPTCHA is not implemented"`.
- Compiler: `IMAGECREATECAPTCHA` is in the zero-arg not-implemented function list (`src/compiler.cpp:1737`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5412`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13798` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1737`, symbol at `src/compiler.cpp:5412` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*imagecreatecaptcha*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:466` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Image*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_IMAGECREATECAPTCHA.md` |

## What IMAGECREATECAPTCHA does at the low C level

Draw a CAPTCHA (random distorted text from the `text` option, default 4-char) into `image`, sized `width`x`height`. `options` carries `text`, `fonts`, `fontsize`, `difficulty`, `noise`, etc. Returns the image.

- Arg 1: `image` (any, required).
- Arg 2: `width` (numeric, required).
- Arg 3: `height` (numeric, required).
- Arg 4: `options` (struct, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `image` (any). passed by value.
- Arg 2: `width` (numeric). passed by value.
- Arg 3: `height` (numeric). passed by value.
- Arg 4: `options` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_imagecreatecaptcha(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Image model + font rendering (text drawing) + random distortion — font rasterization does not exist. Unit tests + tracker updates.

## Ease of implementation

Hard. Needs text/font rendering, not just pixel ops.
