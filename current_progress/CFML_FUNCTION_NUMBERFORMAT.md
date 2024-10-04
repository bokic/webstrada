# Research: NumberFormat cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_numberformat()` in `src/cf8.cpp:14810` throws `"Function NumberFormat is not implemented"`.
- Compiler: `NUMBERFORMAT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5549`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14810` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5549` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*numberformat*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:635` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_NUMBERFORMAT.md` |

## What NumberFormat does at the low C level

Per Adobe CF: create a custom-formatted number from a mask. Mask grammar: `_`/`9` digit placeholder, `.` decimal point, `0` zero-pad, `,` grouping, `-`/`+` literal, `()` for negatives, `C`/`D`/`E` etc. for other placeholders. Uses US separators (non-localized; use `LSNumberFormat` for locale).

- Arg 1: `number` (numeric).
- Arg 2: `mask` (optional, default `_9`).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `number` (numeric). passed by value.
- Arg 2: `mask` (optional, default `_9`). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_numberformat(const cfvariant *number, const cfvariant *mask);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation of the mask engine (shared with `LSNumberFormat`).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies.

## Ease of implementation

Moderate. The mask grammar (placeholders, zero-pad, grouping, negative handling) is the bulk of the work; a widely-used function.
