# Research: SpanIncluding cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_spanincluding(const cfvariant *, const cfvariant *)` in `src/cf8.cpp:15881`.
- Symbol registered with correct signature at `src/compiler.cpp:5628`.
- Interpreter (`evalFunction`) dispatch at `src/cf8.cpp:5525`.
- No native JIT compile handler in `src/compiler.cpp`; calls fall through to the generic dynamic dispatch path.

## Implemented: 70%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation (returns chars from start while each char is in the set; case-sensitive) | `src/cf8.cpp:15881` |
| Compiler wiring | ⚠️ No native JIT handler — goes through generic `cfvariant_call_function` + JIT arg-delegation fallback (`src/compiler.cpp:2663`) | symbol at `src/compiler.cpp:5628` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:5525` |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*spanincluding*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:728` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SPANINCLUDING.md` |

Key detail: works via the dynamic fallback but does not meet the AGENTS.md rule of being compiled as a direct JIT call.

## What SpanIncluding does at the low C level

Per Adobe CF: scan `String` from index 1 and copy characters as long as each character is contained in `set` (case-sensitive); stop at the first character not in the set. If the first character is not in the set, returns an empty string.

- Arg 1: `String` (input).
- Arg 2: `set` (characters to include).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `String` (input). passed by value.
- Arg 2: `set` (characters to include). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_spanincluding(const cfvariant *str, const cfvariant *set);
```

Native JIT handler required (like `cf_writedump`, `src/compiler.cpp:1762`) instead of the generic fallback.

## Dependency

- Native JIT compile handler in `src/compiler.cpp` (currently only the generic fallback path exists).
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies.

## Ease of implementation

Easy. Logic is already written and correct; remaining work is wiring a direct JIT handler, adding tests, and updating trackers.
