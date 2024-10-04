# Research: ReEscape cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_reescape(const cfvariant *)` in `src/cf8.cpp:12101`.
- Symbol registered with correct signature at `src/compiler.cpp:5592`.
- Interpreter (`evalFunction`) dispatch at `src/cf8.cpp:5485`.
- No native JIT compile handler in `src/compiler.cpp`; calls fall through to the generic dynamic dispatch path.

## Implemented: 70%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation (escapes `\.^$*+?()[]{}|` with backslash) | `src/cf8.cpp:12101` |
| Compiler wiring | ⚠️ No native JIT handler — goes through generic `cfvariant_call_function` + JIT arg-delegation fallback (`src/compiler.cpp:2663`) | symbol at `src/compiler.cpp:5592` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:5485` |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*reescape*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:682` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Regex) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REESCAPE.md` |

Key detail: works via the dynamic fallback but does not meet the AGENTS.md rule of being compiled as a direct JIT call. Full escape set needs verification against Adobe CF.

## What ReEscape does at the low C level

Per Adobe CF: prefix regex metacharacters with a backslash so a string can be used as a literal regex pattern. Current impl escapes `\ . ^ $ * + ? ( ) [ ] { } |`.

- Arg 1: `String` (input).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `String` (input). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_reescape(const cfvariant *str);
```

Native JIT handler required (like `cf_writedump`, `src/compiler.cpp:1762`) instead of the generic fallback.

## Dependency

- Native JIT compile handler in `src/compiler.cpp` (currently only the generic fallback path exists).
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification (confirm full metacharacter set).
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies (does not itself need a regex engine).

## Ease of implementation

Easy. Logic is already written; remaining work is wiring a direct JIT handler, adding tests, and updating trackers.
