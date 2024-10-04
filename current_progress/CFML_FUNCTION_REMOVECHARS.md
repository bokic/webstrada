# Research: RemoveChars cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_removechars(const cfvariant *, const cfvariant *, const cfvariant *)` in `src/cf8.cpp:15678`.
- Symbol registered with correct signature at `src/compiler.cpp:5599`.
- Interpreter (`evalFunction`) dispatch at `src/cf8.cpp:5512`.
- No native JIT compile handler in `src/compiler.cpp`; calls fall through to the generic dynamic dispatch path.

## Implemented: 70%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation (`res.remove(st - 1, cnt)`); ❌ throws on out-of-bounds start — Adobe CF returns the string unchanged instead | `src/cf8.cpp:15678` |
| Compiler wiring | ⚠️ No native JIT handler — goes through generic `cfvariant_call_function` + JIT arg-delegation fallback (`src/compiler.cpp:2663`) | symbol at `src/compiler.cpp:5599` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:5512` |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*removechars*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:689` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REMOVECHARS.md` |

Key detail: works via the dynamic fallback but does not meet the AGENTS.md rule of being compiled as a direct JIT call. Edge-case behavior (start out of range) needs verification against Adobe CF.

## What RemoveChars does at the low C level

Per Adobe CF: remove `count` characters starting at 1-based position `start`. If `start` is greater than the string length, the string is returned unchanged.

- Arg 1: `String` (input).
- Arg 2: `start` (1-based starting position).
- Arg 3: `count` (number of characters to remove).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `String` (input). passed by value.
- Arg 2: `start` (1-based starting position). passed by value.
- Arg 3: `count` (number of characters to remove). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_removechars(const cfvariant *str, const cfvariant *start, const cfvariant *count);
```

Native JIT handler required (like `cf_writedump`, `src/compiler.cpp:1762`) instead of the generic fallback.

## Dependency

- Native JIT compile handler in `src/compiler.cpp` (currently only the generic fallback path exists).
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification, including the out-of-range `start` edge case.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies.

## Ease of implementation

Easy. Logic is already written; remaining work is verifying/fixing the out-of-bounds behavior against ColdFusion, wiring a direct JIT handler, adding tests, and updating trackers.
