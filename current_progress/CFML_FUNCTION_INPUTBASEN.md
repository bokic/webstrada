# Research: InputBaseN cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_inputbasen(const cfvariant *, const cfvariant *)` in `src/cf8.cpp:14006`.
- Symbol registered with correct signature at `src/compiler.cpp:5464`.
- Interpreter (`evalFunction`) dispatch at `src/cf8.cpp:5501`.
- No native JIT compile handler in `src/compiler.cpp`; calls fall through to the generic dynamic dispatch path.

## Implemented: 70%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation (uses `strtoll` with radix; validates radix 2–36; returns `int`) | `src/cf8.cpp:14006` |
| Compiler wiring | ⚠️ No native JIT handler — goes through generic `cfvariant_call_function` + JIT arg-delegation fallback (`src/compiler.cpp:2663`) | symbol at `src/compiler.cpp:5464` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:5501` |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*inputbasen*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:519` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_INPUTBASEN.md` |

Key detail: works via the dynamic fallback but does not meet the AGENTS.md rule of being compiled as a direct JIT call. `strtoll` accepts case-insensitive digits and signed input; error/overflow behavior needs verification against Adobe CF.

## What InputBaseN does at the low C level

Per Adobe CF: convert `String` (digits in the given `radix`, 2–36) to a decimal integer. Returns the numeric result.

- Arg 1: `String` (number literal in base `radix`).
- Arg 2: `radix` (2–36).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `String` (number literal in base `radix`). passed by value.
- Arg 2: `radix` (2–36). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_inputbasen(const cfvariant *str, const cfvariant *radix);
```

Native JIT handler required (like `cf_writedump`, `src/compiler.cpp:1762`) instead of the generic fallback.

## Dependency

- Native JIT compile handler in `src/compiler.cpp` (currently only the generic fallback path exists).
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification (valid/invalid input, overflow, radix bounds).
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies.

## Ease of implementation

Easy. Logic is already written; remaining work is verifying edge cases against ColdFusion, wiring a direct JIT handler, adding tests, and updating trackers.
