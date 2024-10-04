# Research: REMatch cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_rematch()` in `src/cf8.cpp:15666` throws `"Function REMatch is not implemented"`.
- Compiler: `REMATCH` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5596`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15666` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5596` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*rematch*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:686` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Regex) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REMATCH.md` |

## What REMatch does at the low C level

Per Adobe CF: case-sensitive regex search returning an array of all matched substrings (not positions).

- Arg 1: `regex` (pattern).
- Arg 2: `string` (target).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `regex` (pattern). passed by value.
- Arg 2: `string` (target). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_rematch(const cfvariant *regex, const cfvariant *str);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- **Regex engine** — none currently exists in the codebase.
- Array construction (array support already exists in cfvariant).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

## Ease of implementation

Moderate-to-hard. Simpler than `REFind` (returns plain array of matches, no subexpression struct), but still needs a regex engine.
